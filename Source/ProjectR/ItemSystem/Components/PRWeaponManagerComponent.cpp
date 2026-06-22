// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기별 조준 반동 애니메이션 및 줌 제어 연동 구현)
// Author: 배유찬 (무기 교체(Swap) 메커니즘, 장전/탄약 관리 및 Mod 게이지 제어 로직 구현)
// Author: 이건주 (배리어 모드 장착 흐름 및 무기 상태 HUD 실시간 동기화 구현)
#include "PRWeaponManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/ItemSystem/PRWeaponStatStatics.h"

namespace
{
	// 무기와 Mod의 태그 호환 여부 판정
	bool IsModCompatible(const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
	{
		// 무기 데이터나 Mod 데이터가 없는 경우
		if (!IsValid(WeaponData) || !IsValid(ModData))
		{
			// 호환성 판정 불가
			return false;
		}

		// Mod가 호환 판정에 사용할 태그를 갖지 않은 경우
		if (ModData->ModTags.IsEmpty())
		{
			// 호환 가능한 대상 없음
			return false;
		}

		// 무기가 지원 가능한 Mod 태그를 갖지 않은 경우
		if (WeaponData->SupportedModTags.IsEmpty())
		{
			// 허용 가능한 Mod 범위 없음
			return false;
		}

		// 무기 지원 태그와 Mod 태그가 하나라도 겹치면 호환으로 판정
		return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
	}

	FString GetNetModeLogName(const UObject* WorldContextObject)
	{
		// 월드 컨텍스트를 얻을 수 없는 경우
		if (!IsValid(WorldContextObject) || !IsValid(WorldContextObject->GetWorld()))
		{
			// 로그에서 월드 없음 상태로 표기
			return TEXT("NoWorld");
		}

		// 로그 가독성을 위해 엔진 NetMode를 문자열로 변환
		switch (WorldContextObject->GetWorld()->GetNetMode())
		{
		case NM_Standalone:
			return TEXT("Standalone");
		case NM_DedicatedServer:
			return TEXT("DedicatedServer");
		case NM_ListenServer:
			return TEXT("ListenServer");
		case NM_Client:
			return TEXT("Client");
		default:
			return TEXT("Unknown");
		}
	}

	// ASC 상태 태그 추가
	void AddASCStateTag(UPRAbilitySystemComponent* ASC, AActor* OwnerActor, const FGameplayTag& Tag)
	{
		if (!IsValid(ASC) || !Tag.IsValid())
		{
			return;
		}

		ASC->AddLooseGameplayTag(Tag);
		if (IsValid(OwnerActor) && OwnerActor->HasAuthority())
		{
			ASC->AddReplicatedLooseGameplayTag(Tag);
		}
	}

	// ASC 상태 태그 제거
	void RemoveASCStateTag(UPRAbilitySystemComponent* ASC, AActor* OwnerActor, const FGameplayTag& Tag)
	{
		if (!IsValid(ASC) || !Tag.IsValid())
		{
			return;
		}

		ASC->RemoveLooseGameplayTag(Tag);
		if (IsValid(OwnerActor) && OwnerActor->HasAuthority())
		{
			ASC->RemoveReplicatedLooseGameplayTag(Tag);
		}
	}

	// 슬롯 태그 조회
	FGameplayTag ResolveCurrentSlotTag(EPRWeaponSlotType SlotType)
	{
		if (SlotType == EPRWeaponSlotType::Primary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Primary;
		}

		if (SlotType == EPRWeaponSlotType::Secondary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Secondary;
		}

		return FGameplayTag();
	}

	// 기본 사격 태그 조회
	FGameplayTag ResolveCurrentSlotBaseFireTag(EPRWeaponSlotType SlotType)
	{
		if (SlotType == EPRWeaponSlotType::Primary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Primary_Base;
		}

		if (SlotType == EPRWeaponSlotType::Secondary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Secondary_Base;
		}

		return FGameplayTag();
	}

	// Mod 사격 태그 조회
	FGameplayTag ResolveCurrentSlotModFireTag(EPRWeaponSlotType SlotType)
	{
		if (SlotType == EPRWeaponSlotType::Primary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Primary_Mod;
		}

		if (SlotType == EPRWeaponSlotType::Secondary)
		{
			return PRGameplayTags::State_CurrentWeaponSlot_Secondary_Mod;
		}

		return FGameplayTag();
	}

}

UPRWeaponManagerComponent::UPRWeaponManagerComponent()
{
	// 무기 관리는 명시 요청과 동기화으로 갱신하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;

	// 슬롯 공개 상태와 무장 상태를 네트워크로 전달하기 위해 기본 복제 활성화
	SetIsReplicatedByDefault(true);
}

void UPRWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트 종료 전에 주무기 Item의 AbilitySet 회수
	if (IsValid(PrimaryWeaponInstance))
	{
		PrimaryWeaponInstance->OnUnequipped(GetOwner());
	}

	// 컴포넌트 종료 전에 보조무기 Item의 AbilitySet 회수
	if (IsValid(SecondaryWeaponInstance))
	{
		SecondaryWeaponInstance->OnUnequipped(GetOwner());
	}

	// PlayerState ASC에 남은 장착 지속 효과를 모두 회수
	UpdateCurrentWeaponSlotTags(CurrentWeaponSlot, EPRWeaponSlotType::None);
	RemoveAllEquipEffects();

	// 컴포넌트 종료 시 남아 있는 주무기 Actor 정리
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);

	// 컴포넌트 종료 시 남아 있는 보조무기 Actor 정리
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	Super::EndPlay(EndPlayReason);
}

void UPRWeaponManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 무장 상태는 클라이언트 부착 소켓과 공개 비주얼 상태를 결정하므로 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, ArmedState);

	// 활성 무기 슬롯은 클라이언트가 손에 든 무기와 수납 무기를 구분하도록 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, CurrentWeaponSlot);

	// 주무기 공개 비주얼 정보는 owner-only 원본 포인터 없이 클라이언트 표시를 갱신하기 위해 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, PrimaryVisualInfo);

	// 보조무기 공개 비주얼 정보는 owner-only 원본 포인터 없이 클라이언트 표시를 갱신하기 위해 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, SecondaryVisualInfo);

	// 주무기 원본 Item 참조는 소유자 UI와 서버 내부 장착 판단에만 필요하므로 owner-only로 복제
	DOREPLIFETIME_CONDITION(UPRWeaponManagerComponent, PrimaryWeaponInstance, COND_OwnerOnly);

	// 보조무기 원본 Item 참조는 소유자 UI와 서버 내부 장착 판단에만 필요하므로 owner-only로 복제
	DOREPLIFETIME_CONDITION(UPRWeaponManagerComponent, SecondaryWeaponInstance, COND_OwnerOnly);
}

void UPRWeaponManagerComponent::InitializeRuntimeLinks()
{
	// PlayerState 재연결 시 이전 캐시가 남지 않도록 먼저 초기화
	CachedASC = nullptr;
	CachedWeaponSet = nullptr;
	CachedInventory = nullptr;

	const APRPlayerState* PlayerState = Cast<APRPlayerState>(GetOwner());
	if (!IsValid(PlayerState))
	{
		// 함수 조기 종료. PlayerState가 아직 준비되지 않은 시점이므로 캐시를 비운 상태로 유지
		return;
	}

	// 서버 권위 장착 처리에서 사용할 런타임 의존성 캐시
	CachedASC = PlayerState->GetPRAbilitySystemComponent();
	CachedWeaponSet = PlayerState->GetWeaponSet();
	CachedInventory = PlayerState->GetInventoryComponent();
}

void UPRWeaponManagerComponent::InitializeWithPawn(APRCharacterBase* InPawn)
{
	CachedPawnOwner = InPawn;
	RefreshAllWeaponActors();
	RefreshAnimLayer();
}

void UPRWeaponManagerComponent::ResetSystem()
{
	// 기존 Pawn 무기 Actor 제거
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	// 무장 표현 상태 초기화
	ArmedState = EPRArmedState::Unarmed;
	CurrentLinkedAnimLayerClass = nullptr;

	if (APRPlayerState* PlayerState = GetOwnerPlayerState())
	{
		if (UPRAbilitySystemComponent* ASC = PlayerState->GetPRAbilitySystemComponent())
		{
			// 무장 태그 잔류 제거
			ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Armed);
			ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Armed);
		}
	}

	// Pawn 의존 캐시 초기화
	CachedPawnOwner = nullptr;
	CachedASC = nullptr;
	CachedWeaponSet = nullptr;
	CachedInventory = nullptr;

	if (IsValid(GetOwner()))
	{
		// 무장 상태 복제 갱신
		GetOwner()->ForceNetUpdate();
	}
}


FPRWeaponManagerSaveData UPRWeaponManagerComponent::MakeSaveData() const
{
	FPRWeaponManagerSaveData SaveData;
	const APRPlayerState* PlayerState = GetOwnerPlayerState();
	const UPRInventoryComponent* Inventory = IsValid(PlayerState) ? PlayerState->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		return SaveData;
	}
	SaveData.PrimaryWeaponIndex = Inventory->GetItemIndexByType(PrimaryWeaponInstance, EPRItemType::Weapon);
	SaveData.SecondaryWeaponIndex = Inventory->GetItemIndexByType(SecondaryWeaponInstance, EPRItemType::Weapon);
	SaveData.CurrentWeaponSlot = CurrentWeaponSlot;
	
	const UPRAbilitySystemComponent* SaveASC = CachedASC.IsValid()
		? CachedASC.Get()
		: PlayerState->GetPRAbilitySystemComponent();
	if (IsValid(SaveASC))
	{
		// 슬롯별 현재 자원 저장
		SaveData.PrimaryMagazineAmmo = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute());
		SaveData.PrimaryReserveAmmo = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute());
		SaveData.PrimaryModGauge = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute());
		SaveData.PrimaryModStack = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetPrimaryModStackAttribute());
		SaveData.SecondaryMagazineAmmo = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute());
		SaveData.SecondaryReserveAmmo = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute());
		SaveData.SecondaryModGauge = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute());
		SaveData.SecondaryModStack = SaveASC->GetNumericAttribute(UPRAttributeSet_Weapon::GetSecondaryModStackAttribute());
	}
	return SaveData;
}

void UPRWeaponManagerComponent::ApplySaveData(const FPRWeaponManagerSaveData& InSaveData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}
	InitializeRuntimeLinks();
	if (!CachedInventory.IsValid())
	{
		return;
	}
	if (IsValid(PrimaryWeaponInstance))
	{
		// 기존 주무기 슬롯 장착 상태 정리
		PrimaryWeaponInstance->OnUnequipped(GetOwner());
	}
	if (IsValid(SecondaryWeaponInstance))
	{
		// 기존 보조무기 슬롯 장착 상태 정리
		SecondaryWeaponInstance->OnUnequipped(GetOwner());
	}
	RemoveAllEquipEffects();
	UpdateCurrentWeaponSlotTags(CurrentWeaponSlot, EPRWeaponSlotType::None);
	PrimaryWeaponInstance = nullptr;
	SecondaryWeaponInstance = nullptr;
	CurrentWeaponSlot = EPRWeaponSlotType::None;
	ArmedState = EPRArmedState::Unarmed;
	UPRItemInstance_Weapon* RestoredPrimaryItem = nullptr;
	UPRItemInstance_Weapon* RestoredSecondaryItem = nullptr;
	if (UPRItemInstance_Weapon* PrimaryItem = CachedInventory->GetItemAtIndexByType<UPRItemInstance_Weapon>(EPRItemType::Weapon, InSaveData.PrimaryWeaponIndex))
	{
		RestoredPrimaryItem = PrimaryItem;
		EquipWeaponInternal(RestoredPrimaryItem);
	}
	if (UPRItemInstance_Weapon* SecondaryItem = CachedInventory->GetItemAtIndexByType<UPRItemInstance_Weapon>(EPRItemType::Weapon, InSaveData.SecondaryWeaponIndex))
	{
		RestoredSecondaryItem = SecondaryItem;
		EquipWeaponInternal(RestoredSecondaryItem);
	}
	if (IsSupportedSlot(InSaveData.CurrentWeaponSlot))
	{
		SetCurrentWeaponSlotInternal(InSaveData.CurrentWeaponSlot);
	}
	
	if (IsValid(RestoredPrimaryItem))
	{
		// 주무기 현재 자원 복원
		ApplyOverrideAmmoGE(
			EPRWeaponSlotType::Primary,
			InSaveData.PrimaryMagazineAmmo,
			InSaveData.PrimaryReserveAmmo,
			RestoredPrimaryItem);
		ApplyOverrideModResourceGE(
			EPRWeaponSlotType::Primary,
			InSaveData.PrimaryModGauge,
			InSaveData.PrimaryModStack,
			RestoredPrimaryItem);
	}
	if (IsValid(RestoredSecondaryItem))
	{
		// 보조무기 현재 자원 복원
		ApplyOverrideAmmoGE(
			EPRWeaponSlotType::Secondary,
			InSaveData.SecondaryMagazineAmmo,
			InSaveData.SecondaryReserveAmmo,
			RestoredSecondaryItem);
		ApplyOverrideModResourceGE(
			EPRWeaponSlotType::Secondary,
			InSaveData.SecondaryModGauge,
			InSaveData.SecondaryModStack,
			RestoredSecondaryItem);
	}
	RefreshVisualInfosFromCurrentState();
	RefreshAllWeaponActors();
	RefreshAnimLayer();
}

UPRItemInstance_Weapon* UPRWeaponManagerComponent::GetWeaponInstanceBySlotType(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponInstance;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponInstance;
	}

	// 지원하지 않는 슬롯은 원본 Item 없음으로 nullptr 리턴
	return nullptr;
}

UPRWeaponDataAsset* UPRWeaponManagerComponent::GetWeaponDataBySlotType(EPRWeaponSlotType SlotType) const
{
	// 서버나 소유자 경로에서 원본 Item을 조회할 수 있는 경우
	if (UPRItemInstance_Weapon* WeaponInstance = GetWeaponInstanceBySlotType(SlotType))
	{
		// 원본 Item의 최신 무기 데이터를 리턴
		return WeaponInstance->GetWeaponData();
	}

	// 원본 Item이 없는 클라이언트는 공개 비주얼 정보를 기준으로 무기 데이터 리턴
	const FPRWeaponVisualInfo& VisualInfo = GetVisualInfoBySlotType(SlotType);
	return VisualInfo.WeaponData;
}

float UPRWeaponManagerComponent::GetCurrentWeaponBaseDamage() const
{
	const UPRItemInstance_Weapon* CurrentWeapon = GetWeaponInstanceBySlotType(CurrentWeaponSlot);
	return UPRWeaponStatStatics::CalculateWeaponItemBaseDamage(CurrentWeapon);
}

const FPRWeaponVisualInfo& UPRWeaponManagerComponent::GetCurrentWeaponVisualInfo() const
{
	// 현재 활성 슬롯에 대응하는 공개 비주얼 정보 리턴
	return GetVisualInfoBySlotType(CurrentWeaponSlot);
}

const UPRCrosshairConfig* UPRWeaponManagerComponent::GetActiveCrosshairConfig() const
{
	const FPRWeaponVisualInfo& CurrentVisualInfo = GetCurrentWeaponVisualInfo();
	const UPRWeaponDataAsset* WeaponData = CurrentVisualInfo.WeaponData;
	if (!IsValid(WeaponData))
	{
		return nullptr;
	}

	// 무기 데이터의 전용 설정 또는 프로젝트 기본 설정 반환
	return WeaponData->GetCrosshairConfig();
}

const FPRWeaponVisualInfo& UPRWeaponManagerComponent::GetVisualInfoBySlotType(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryVisualInfo;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryVisualInfo;
	}

	// 지원하지 않는 슬롯은 변경되지 않는 빈 비주얼 정보 리턴
	static FPRWeaponVisualInfo EmptyVisualInfo;
	return EmptyVisualInfo;
}

bool UPRWeaponManagerComponent::EquipInventoryWeaponAtIndex(int32 InventoryIndex)
{
	// 인벤토리 캐시가 비어 있을 수 있으므로 장착 요청마다 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 인벤토리 컴포넌트를 찾지 못한 경우
	if (!CachedInventory.IsValid())
	{
		// 장착 실패. 장착 대상 조회 불가
		return false;
	}

	// 인덱스 기반 UI 요청을 실제 Item 참조 요청으로 변환하기 위한 Item 조회
	UPRItemInstance_Weapon* WeaponItem = CachedInventory->GetItemAtIndexByType<UPRItemInstance_Weapon>(EPRItemType::Weapon, InventoryIndex);

	// 인덱스에 대응하는 무기 Item이 없는 경우
	if (!IsValid(WeaponItem))
	{
		// 장착 실패 상황을 Owner, Index 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][%s] 무기 장착 실패. EquipInventoryWeaponAtIndex() | Owner = %s | Index = %d"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			InventoryIndex);

		// 장착 실패. Item 조회 실패
		return false;
	}

	// 조회된 Item 참조로 장착 요청을 이어간다
	return EquipWeapon(WeaponItem);
}

bool UPRWeaponManagerComponent::EquipWeapon(UPRItemInstance_Weapon* WeaponItem)
{
	// 무기 Item이 없는 경우
	if (!IsValid(WeaponItem))
	{
		// 장착 실패. 요청 검증 실패
		return false;
	}

	// 원본 포인터 기반 장착은 서버 권위에서만 직접 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 내부 처리에서 인벤토리 소유 여부를 검증할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 장착 결과 리턴
		return EquipWeaponInternal(WeaponItem);
	}

	// 클라이언트 요청은 서버에서 Item 참조와 인벤토리 소유권을 다시 검증하도록 RPC 전송
	Server_EquipWeapon(WeaponItem);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

bool UPRWeaponManagerComponent::UnequipWeaponFromSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단
	if (!IsSupportedSlot(TargetSlot))
	{
		// 해제 실패. 요청 검증 실패
		return false;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 내부 해제 경로로 즉시 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 서버 권위 해제 처리에서 GE를 적용할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 해제 결과 리턴
		return UnequipWeaponFromSlotInternal(TargetSlot);
	}

	// 클라이언트 요청은 서버 권위에서 슬롯 해제를 수행하도록 RPC 전송
	Server_UnequipWeaponSlot(TargetSlot);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

bool UPRWeaponManagerComponent::AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(TargetSlot))
	{
		// Mod 장착 실패. 요청 검증 실패
		return false;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 내부 Mod 장착 경로로 즉시 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 내부 처리에서 플레이어 자원을 갱신할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 Mod 장착 결과 리턴
		return AttachModToSlotInternal(TargetSlot, NewModData);
	}

	// 클라이언트 요청은 서버 권위에서 Mod 호환성을 검증하도록 RPC 전송
	Server_AttachModToSlot(TargetSlot, NewModData);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

void UPRWeaponManagerComponent::SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단
	if (!IsSupportedSlot(TargetSlot))
	{
		// 함수 조기 종료. 요청 검증 실패
		return;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 활성 슬롯을 즉시 전환
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 서버 권위 슬롯 전환에서 GE를 적용할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		SetCurrentWeaponSlotInternal(TargetSlot);

		// 함수 조기 종료. 서버 전환 처리 마침
		return;
	}

	// 클라이언트 요청은 서버 권위에서 활성 슬롯을 전환하도록 RPC 전송
	Server_SetCurrentWeaponSlot(TargetSlot);
}

void UPRWeaponManagerComponent::SetWeaponArmedState(EPRArmedState NewArmedState)
{
	// 서버 권위가 있으면 RPC를 거치지 않고 무장 상태를 즉시 변경
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SetWeaponArmedStateInternal(NewArmedState);
		
		// 함수 조기 종료. 서버 무장 상태 변경 마침
		return;
	}
	
	// 26.05.12, Yuchan, 권위가 없어도 먼저 상태 변경을 예측.
	SetWeaponArmedStateInternal(NewArmedState);

	// 서버 권위에서 무장 상태를 변경하도록 RPC 전송
	Server_SetWeaponArmedState(NewArmedState);
}

void UPRWeaponManagerComponent::RefreshCurrentWeaponFireModeTags()
{
	UPRAbilitySystemComponent* ASC = CachedASC.IsValid() ? CachedASC.Get() : nullptr;
	if (!IsValid(ASC))
	{
		if (APRPlayerState* PlayerState = GetOwnerPlayerState())
		{
			ASC = PlayerState->GetPRAbilitySystemComponent();
		}
	}

	if (!IsValid(ASC) || !IsSupportedSlot(CurrentWeaponSlot))
	{
		return;
	}

	const FGameplayTag BaseFireTag = ResolveCurrentSlotBaseFireTag(CurrentWeaponSlot);
	const FGameplayTag ModFireTag = ResolveCurrentSlotModFireTag(CurrentWeaponSlot);
	AActor* OwnerActor = GetOwner();

	// 기존 사격 모드 태그 제거
	RemoveASCStateTag(ASC, OwnerActor, BaseFireTag);
	RemoveASCStateTag(ASC, OwnerActor, ModFireTag);

	const UPRItemInstance_Weapon* CurrentWeapon = GetWeaponInstanceBySlotType(CurrentWeaponSlot);
	const FGameplayTag NewFireModeTag = IsValid(CurrentWeapon) && CurrentWeapon->FireModeState == EPRWeaponFireModeState::ModFire
		? ModFireTag
		: BaseFireTag;

	// 현재 사격 모드 태그 추가
	AddASCStateTag(ASC, OwnerActor, NewFireModeTag);
}

APRWeaponActor* UPRWeaponManagerComponent::GetActiveWeaponActor() const
{
	// 현재 활성 슬롯에 대응하는 로컬 무기 Actor 리턴
	return GetWeaponActorBySlot(CurrentWeaponSlot);
}
//
// EPRWeaponSlotType UPRWeaponManagerComponent::GetAimOffsetWeaponSlot() const
// {
// 	// 비무장 상태에서는 맨손 AimOffset을 사용하도록 빈 슬롯을 반환
// 	if (ArmedState != EPRArmedState::Armed)
// 	{
// 		return EPRWeaponSlotType::None;
// 	}
//
// 	// 주무기와 보조무기 외 슬롯은 AimOffset 대상으로 사용하지 않는다
// 	if (!IsSupportedSlot(CurrentWeaponSlot))
// 	{
// 		return EPRWeaponSlotType::None;
// 	}
//
// 	const FPRWeaponVisualInfo& CurrentVisualInfo = GetCurrentWeaponVisualInfo();
//
// 	// 현재 활성 슬롯에 공개 무기 데이터가 없으면 맨손 AimOffset으로 처리
// 	if (CurrentVisualInfo.IsEmpty())
// 	{
// 		return EPRWeaponSlotType::None;
// 	}
//
// 	return CurrentWeaponSlot;
// }

bool UPRWeaponManagerComponent::IsManagingWeaponItem(const UPRItemInstance_Weapon* WeaponItem) const
{
	return ResolveWeaponItemSlot(WeaponItem) != EPRWeaponSlotType::None;
}

void UPRWeaponManagerComponent::HandleInventoryWeaponModChanged(UPRItemInstance_Weapon* WeaponItem)
{
	// PlayerState 기반 런타임 캐시가 늦게 연결된 경우를 대비해 반응 처리 전에 갱신
	InitializeRuntimeLinks();

	// 인벤토리 변경 반응은 현재 매니저가 슬롯 원본으로 들고 있는 무기만 처리
	const EPRWeaponSlotType TargetSlot = ResolveWeaponItemSlot(WeaponItem);
	if (TargetSlot == EPRWeaponSlotType::None)
	{
		// 함수 조기 종료. 현재 캐릭터가 장착 중인 무기 Item이 아님
		return;
	}

	// 슬롯 자원 재초기화와 어빌리티 리빌드에는 유효한 무기 데이터가 필요하다
	if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
	{
		// 함수 조기 종료. 슬롯 원본 데이터가 유효하지 않다
		return;
	}

	// 현재 활성 중인 무기라면 기존 Mod 어빌리티를 회수하고 새 Mod 어빌리티를 부여
	WeaponItem->OnModChanged(GetOwner(), WeaponItem->GetModData());
	ApplyEquipModGE(TargetSlot, WeaponItem->GetModData(), WeaponItem);
	ApplyOverrideModResourceGE(TargetSlot, 0.0f, 0.0f, WeaponItem);

	// Mod 장착 여부에 맞춰 최근 Mod 실패 사유를 정리
	WeaponItem->LastModFailReason = IsValid(WeaponItem->GetModData()) ? EPRWeaponModFailReason::None : EPRWeaponModFailReason::MissingMod;

	// 인벤토리 정본 변경 결과를 복제 공개 비주얼 정보에 반영
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬 Actor도 Mod 변경 결과에 맞춰 즉시 최신화
	RefreshAllWeaponActors();

	// 현재 활성 무기의 애니메이션 레이어 캐시를 재확인
	RefreshAnimLayer();

	// 장착 중인 무기의 Mod 표시 상태가 바뀌었음을 HUD와 인벤토리 UI에 알린다
	OnWeaponEquipmentChanged.Broadcast(this, TargetSlot);

	// 서버 Mod 변경 반응이 끝난 뒤 Owner, 슬롯, Weapon Item, Mod 상태를 남겨 인벤토리 연동 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] 인벤토리 Mod 변경 반영. HandleInventoryWeaponModChanged() | Owner = %s | Slot = %s | WeaponItem = %s | Mod = %s | ModItem = %s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(TargetSlot),
		*GetNameSafe(WeaponItem),
		*GetNameSafe(WeaponItem->GetModData()),
		*GetNameSafe(WeaponItem->GetEquippedModItem()));
}

void UPRWeaponManagerComponent::RefreshCurrentWeaponUpgradeState(UPRItemInstance_Weapon* WeaponItem)
{
	// 강화 반영은 현재 활성 무기 원본에 대해서만 즉시 전투 GE를 갱신
	if (!IsValid(WeaponItem) || GetWeaponInstanceBySlotType(CurrentWeaponSlot) != WeaponItem)
	{
		return;
	}

	InitializeRuntimeLinks();
	ApplyCurrentWeaponGE(WeaponItem);
	OnWeaponEquipmentChanged.Broadcast(this, CurrentWeaponSlot);
}

void UPRWeaponManagerComponent::RequestWeaponAnimation(EPRWeaponAnimationState AnimationState)
{
	APRWeaponActor* ActiveWeaponActor = (CurrentWeaponSlot == EPRWeaponSlotType::Primary)
		? PrimaryWeaponActor
		: SecondaryWeaponActor;
	if (IsValid(ActiveWeaponActor))
	{
		ActiveWeaponActor->RequestWeaponAnimation(AnimationState);
	}
}

void UPRWeaponManagerComponent::Multicast_RequestWeaponAnimation_Reliable_Implementation(EPRWeaponAnimationState AnimationState)
{
	if (GetOwner()->HasAuthority())
	{
		return;
	}

	if (APawn* TargetPawn = GetPawnOwner())
	{
		if (TargetPawn->IsLocallyControlled())
		{
			return;
		}
	}

	RequestWeaponAnimation(AnimationState);
}

void UPRWeaponManagerComponent::Multicast_RequestWeaponAnimation_Unreliable_Implementation(EPRWeaponAnimationState AnimationState)
{
	if (GetOwner()->HasAuthority())
	{
		return;
	}

	if (APawn* TargetPawn = GetPawnOwner())
	{
		if (TargetPawn->IsLocallyControlled())
		{
			return;
		}
	}

	RequestWeaponAnimation(AnimationState);
}

bool UPRWeaponManagerComponent::EquipWeaponInternal(UPRItemInstance_Weapon* WeaponItem)
{
	const UPRWeaponDataAsset* WeaponData = IsValid(WeaponItem) ? WeaponItem->GetWeaponData() : nullptr;
	const EPRWeaponSlotType WeaponSlot = IsValid(WeaponData) ? WeaponData->SlotType : EPRWeaponSlotType::None;

	// 잘못된 슬롯, 비소유 Item, 무기 데이터 누락인 경우
	if (!IsSupportedSlot(WeaponSlot)
		|| !IsValid(WeaponItem)
		|| !CachedInventory.IsValid()
		|| !CachedInventory->OwnsItem(WeaponItem)
		|| !IsValid(WeaponData))
	{
		// 장착 실패. 장착 전 검증 실패
		return false;
	}

	// 현재 장착 슬롯에 연결된 원본 Item. 중복 장착 차단과 교체 해제 알림 대상 판단에 사용
	UPRItemInstance_Weapon* CurrentWeaponInstance = GetWeaponInstanceBySlotType(WeaponSlot);

	// 같은 Item이 이미 장착 슬롯에 연결된 경우
	if (CurrentWeaponInstance == WeaponItem)
	{
		// 장착 실패. 중복 장착 요청
		return false;
	}

	// 장착 슬롯이 활성 슬롯이었는지. 기존 무기 해제 알림 필요 여부를 결정
	const bool bWasWeaponSlotCurrent = CurrentWeaponSlot == WeaponSlot;

	if (IsValid(CurrentWeaponInstance))
	{
		CacheAmmoRatiosForSlot(WeaponSlot);
	}

	if (IsValid(CurrentWeaponInstance))
	{
		// 기존 슬롯 무기 해제
		CurrentWeaponInstance->OnUnequipped(GetOwner());
	}

	// 새 무기 장착 전에 기존 슬롯 지속 효과를 회수해 최대치 누적을 방지
	RemoveSlotEquipEffects(WeaponSlot);

	// 장착 검증이 끝난 뒤 장착 슬롯 원본을 새 무기로 확정
	GetMutableWeaponInstanceBySlot(WeaponSlot) = WeaponItem;

	// 무기의 어빌리티셋을 플레이어에게 부여
	WeaponItem->OnEquipped(GetOwner());

	// 현재 활성 슬롯이 비어 있거나 이미 활성 중인 슬롯에 다시 장착하는 경우
	if (CurrentWeaponSlot == EPRWeaponSlotType::None || CurrentWeaponSlot == WeaponSlot)
	{
		const EPRWeaponSlotType OldWeaponSlot = CurrentWeaponSlot;

		// 장착 슬롯을 활성 슬롯으로 지정
		CurrentWeaponSlot = WeaponSlot;

		// 현재 슬롯 태그 갱신
		UpdateCurrentWeaponSlotTags(OldWeaponSlot, CurrentWeaponSlot);
	}

	// 무기 슬롯 최대 자원과 현재 자원을 GE로 적용
	ApplyEquipAmmoGE(WeaponData, WeaponItem);
	ApplyEquipModGE(WeaponSlot, WeaponItem->GetModData(), WeaponItem);
	ApplyOverrideModResourceGE(WeaponSlot, 0.0f, 0.0f, WeaponItem);
	if (CurrentWeaponSlot == WeaponSlot)
	{
		// 활성 슬롯에 새 무기를 장착한 경우에만 현재 무기 전투 스탯을 갱신
		ApplyCurrentWeaponGE(WeaponItem);
	}

	// 원본 슬롯 데이터를 기준으로 두 슬롯의 공개 비주얼 정보를 다시 구성
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬도 복제 콜백과 같은 경로로 슬롯별 Actor를 즉시 최신화
	RefreshAllWeaponActors();

	// 활성 무기 변화에 맞춰 애니메이션 레이어 갱신
	RefreshAnimLayer();

	// 서버 장착 처리가 확정된 뒤 Owner, 슬롯, Weapon Item, 무기 데이터, 활성 슬롯을 남겨 장착 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] 무기 장착 완료. EquipWeaponInternal | Owner = %s | Slot = %s | WeaponItem = %s | Weapon = %s | CurrentWeaponSlot = %s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(WeaponSlot),
		*GetNameSafe(WeaponItem),
		*GetNameSafe(WeaponData),
		*UEnum::GetValueAsString(CurrentWeaponSlot));

	// 장착 성공. 원본 슬롯, 활성 슬롯, 공개 비주얼, 로컬 Actor 갱신 마침
	OnWeaponEquipmentChanged.Broadcast(this, WeaponSlot);
	return true;
}

void UPRWeaponManagerComponent::ApplyEquipAmmoGE(const UPRWeaponDataAsset* WeaponData, UObject* SourceObject)
{
	if (!IsValid(WeaponData) || !CachedASC.IsValid() || !IsSupportedSlot(WeaponData->SlotType))
	{
		return;
	}

	// 현재 적용된 Ammo Equip GE를 제거
	FPREquipSlotEffectHandles& SlotHandles = GetMutableEquipEffectHandlesBySlot(WeaponData->SlotType);
	RemoveEquipEffectHandle(SlotHandles.AmmoMaxHandle);

	// 슬롯 타입에 맞는 Equip GE를 Get
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> EquipGE = WeaponData->SlotType == EPRWeaponSlotType::Primary
		? Registry->EquipGE_PrimaryWeapon
		: Registry->EquipGE_SecondaryWeapon;

	if (!IsValid(EquipGE))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponManager] EquipGE가 유효하지 않습니다. | ApplyEquipAmmoGE()"));
		return;
	}

	// 이펙트 컨텍스트 생성
	FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
	if (IsValid(SourceObject))
	{
		// 컨텍스트에 무기 아이템을 소스 오브젝트로 추가
		Context.AddSourceObject(SourceObject);
	}

	// Equip GE 스펙 핸들 생성 
	const FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(EquipGE, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 무기 데이터 에셋에서 원본 값 불러오기
	const float MaxMagazineAmmo = FMath::Max(WeaponData->MaxMagazineAmmo, 0.0f);
	const float MaxReserveAmmo = MaxMagazineAmmo * FMath::Max(WeaponData->ReserveAmmoMultiplier, 0.0f);

	// 슬롯 타입에 맞게 SetSetByCallerMagnitude로 주입
	if (WeaponData->SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryMaxMagazineAmmo, MaxMagazineAmmo);
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryMaxReserveAmmo, MaxReserveAmmo);
	}
	else
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryMaxMagazineAmmo, MaxMagazineAmmo);
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryMaxReserveAmmo, MaxReserveAmmo);
	}

	// 이펙트 적용과 슬롯 핸들에 등록을 동시 수행
	SlotHandles.AmmoMaxHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	
	// 기록된 CurrentAmmo 값 복구
	RestoreAmmoFromCachedRatios(WeaponData->SlotType, SourceObject);
}

void UPRWeaponManagerComponent::ApplyCurrentWeaponGE(UObject* SourceObject)
{
	if (!CachedASC.IsValid())
	{
		return;
	}

	RemoveEquipEffectHandle(CurrentWeaponEffectHandle);

	const UPRWeaponDataAsset* CurrentWeaponData = nullptr;
	if (const UPRItemInstance_Weapon* CurrentWeapon = GetWeaponInstanceBySlotType(CurrentWeaponSlot))
	{
		CurrentWeaponData = CurrentWeapon->GetWeaponData();
	}

	if (!IsValid(CurrentWeaponData))
	{
		return;
	}

	FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
	if (IsValid(SourceObject))
	{
		Context.AddSourceObject(SourceObject);
	}
	
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> EquipGE = Registry->EquipGE_CurrentWeapon;
	
	if (!IsValid(EquipGE))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponManager] EquipGE가 유효하지 않습니다. | ApplyCurrentWeaponGE()"));
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(EquipGE, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	const UPRItemInstance_Weapon* CurrentWeapon = GetWeaponInstanceBySlotType(CurrentWeaponSlot);
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage, UPRWeaponStatStatics::CalculateWeaponItemBaseDamage(CurrentWeapon));
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_CurrentWeapon_ArmorPenetration, FMath::Max(CurrentWeaponData->AdditiveArmorPenetration, 0.0f));
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_CurrentWeapon_WeakpointMultiplier, FMath::Max(CurrentWeaponData->AdditiveWeakpointMultiplier, 0.0f));
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_CurrentWeapon_GroggyDamageMultiplier, FMath::Max(CurrentWeaponData->AdditiveGroggyDamageMultiplier, 0.0f));

	CurrentWeaponEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UPRWeaponManagerComponent::ApplyEquipModGE(EPRWeaponSlotType SlotType, const UPRWeaponModDataAsset* ModData, UObject* SourceObject)
{
	if (!CachedASC.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	FPREquipSlotEffectHandles& SlotHandles = GetMutableEquipEffectHandlesBySlot(SlotType);
	RemoveEquipEffectHandle(SlotHandles.ModMaxHandle);

	if (!IsValid(ModData))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> EquipGE = SlotType == EPRWeaponSlotType::Primary
		? Registry->EquipGE_PrimaryMod
		: Registry->EquipGE_SecondaryMod;
	
	if (!IsValid(EquipGE))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponManager] EquipGE가 유효하지 않습니다. | ApplyEquipModGE()"));
		return;
	}

	FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
	if (IsValid(SourceObject))
	{
		Context.AddSourceObject(SourceObject);
	}

	const FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(EquipGE, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	const float MaxGauge = FMath::Max(ModData->MaxModGauge, 0.0f);
	const float MaxStack = FMath::Max(ModData->MaxModStack, 0.0f);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryMaxModGauge, MaxGauge);
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryMaxModStack, MaxStack);
	}
	else
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryMaxModGauge, MaxGauge);
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryMaxModStack, MaxStack);
	}

	SlotHandles.ModMaxHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UPRWeaponManagerComponent::ApplyOverrideAmmoGE(EPRWeaponSlotType SlotType, float MagazineAmmo, float ReserveAmmo, UObject* SourceObject) const
{
	if (!CachedASC.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry))
	{
		return;
	}

	// 슬롯 타입에 맞는 오버라이드 Ammo GE를 불러온다
	const TSubclassOf<UGameplayEffect> OverrideGE = SlotType == EPRWeaponSlotType::Primary
		? Registry->EquipGE_Override_PrimaryAmmo
		: Registry->EquipGE_Override_SecondaryAmmo;
	if (!IsValid(OverrideGE))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponManager] OverrideGE가 유효하지 않습니다. | ApplyOverrideAmmoGE()"));
		return;
	}

	// 무기 아이템을 소스 오브젝트로 콘텍스트에 추가
	FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
	if (IsValid(SourceObject))
	{
		Context.AddSourceObject(SourceObject);
	}

	const FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(OverrideGE, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 슬롯에 맞는 SetSetByCallerMagnitude값 주입
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryMagazineAmmo, FMath::Max(MagazineAmmo, 0.0f));
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryReserveAmmo, FMath::Max(ReserveAmmo, 0.0f));
	}
	else
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryMagazineAmmo, FMath::Max(MagazineAmmo, 0.0f));
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryReserveAmmo, FMath::Max(ReserveAmmo, 0.0f));
	}

	// GE 적용
	CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UPRWeaponManagerComponent::ApplyOverrideModResourceGE(EPRWeaponSlotType SlotType, float ModGauge, float ModStack, UObject* SourceObject) const
{
	if (!CachedASC.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry))
	{
		return;
	}

	// 슬롯 타입에 맞는 오버라이드 GE를 불러온다
	const TSubclassOf<UGameplayEffect> OverrideGE = SlotType == EPRWeaponSlotType::Primary
		? Registry->EquipGE_Override_PrimaryModResource
		: Registry->EquipGE_Override_SecondaryModResource;
	if (!IsValid(OverrideGE))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponManager] OverrideGE가 유효하지 않습니다. | ApplyOverrideModResourceGE()"));
		return;
	}

	// 무기 아이템을 소스 오브젝트로 콘텍스트에 추가
	FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
	if (IsValid(SourceObject))
	{
		Context.AddSourceObject(SourceObject);
	}

	const FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(OverrideGE, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 슬롯에 맞는 SetSetByCallerMagnitude값 주입
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryModGauge, FMath::Max(ModGauge, 0.0f));
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_PrimaryModStack, FMath::Max(ModStack, 0.0f));
	}
	else
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryModGauge, FMath::Max(ModGauge, 0.0f));
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Equip_SecondaryModStack, FMath::Max(ModStack, 0.0f));
	}
	
	// GE 적용
	CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

bool UPRWeaponManagerComponent::UnequipWeaponFromSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 해제 타겟 슬롯의 원본 Item. 해제 알림과 로그 기록에 사용
	UPRItemInstance_Weapon* CurrentWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 지원하지 않는 슬롯이거나 비어 있는 슬롯인 경우
	if (!IsSupportedSlot(TargetSlot) || !IsValid(CurrentWeaponInstance))
	{
		// 해제 실패. 검증 실패
		return false;
	}

	// 해제 전 활성 슬롯. 해제 이후 다음 활성 슬롯 선정 기준으로 사용
	const EPRWeaponSlotType PreWeaponSlot = CurrentWeaponSlot;

	// 해제 타겟 슬롯이 활성 슬롯이었는지. 활성 무기 해제 알림 필요 여부를 결정
	const bool bWasTargetSlotCurrent = PreWeaponSlot == TargetSlot;

	CacheAmmoRatiosForSlot(TargetSlot);

	// 해제 대상 무기 AbilitySet 회수
	CurrentWeaponInstance->OnUnequipped(GetOwner());

	// 슬롯 해제를 서버 원본 상태에 먼저 반영
	GetMutableWeaponInstanceBySlot(TargetSlot) = nullptr;

	// 활성 슬롯을 해제한 경우
	if (bWasTargetSlotCurrent)
	{
		// 남은 슬롯 중 다음 활성 슬롯 선정
		CurrentWeaponSlot = ResolveNextWeaponSlotAfterUnequip(TargetSlot);

		// 현재 슬롯 태그 갱신
		UpdateCurrentWeaponSlotTags(PreWeaponSlot, CurrentWeaponSlot);

		// 다음 활성 슬롯이 없는
		if (CurrentWeaponSlot == EPRWeaponSlotType::None)
		{
			// 활성 슬롯 후보가 없으므로 비무장 상태로 변경
			ArmedState = EPRArmedState::Unarmed;
		}
	}

	// 해제 이후 대상 슬롯 지속 효과와 현재 자원을 정리
	RemoveSlotEquipEffects(TargetSlot);
	ClearAmmoAttributesForSlot(TargetSlot, CurrentWeaponInstance);
	ApplyOverrideModResourceGE(TargetSlot, 0.0f, 0.0f, CurrentWeaponInstance);
	if (bWasTargetSlotCurrent)
	{
		// 활성 슬롯이 바뀐 경우 현재 무기 전투 스탯도 새 활성 슬롯 기준으로 갱신
		ApplyCurrentWeaponGE(GetWeaponInstanceBySlotType(CurrentWeaponSlot));
	}

	// 해제 결과를 공개 비주얼 정보에 반영
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬 Actor도 해제 결과에 맞춰 즉시 최신화
	RefreshAllWeaponActors();

	// 활성 슬롯 해제 결과에 맞춰 애니메이션 레이어 갱신
	RefreshAnimLayer();

	// 서버 해제 처리가 확정된 뒤 Owner, 해제 슬롯, Weapon Item, 무기 데이터, 다음 활성 슬롯을 남겨 해제 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] 무기 해제 완료. UnequipWeaponFromSlotInternal | Owner = %s | Slot = %s | WeaponItem = %s | Weapon = %s | NextWeaponSlot = %s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(TargetSlot),
		*GetNameSafe(CurrentWeaponInstance),
		*GetNameSafe(CurrentWeaponInstance->GetWeaponData()),
		*UEnum::GetValueAsString(CurrentWeaponSlot));
	
	// 해제 성공. 원본 슬롯, 활성 슬롯, 공개 비주얼, 로컬 Actor 갱신 마침
	OnWeaponEquipmentChanged.Broadcast(this, TargetSlot);
	return true;
}

bool UPRWeaponManagerComponent::AttachModToSlotInternal(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// Mod 장착 타겟 슬롯의 원본 Item. 호환성 검증과 자원 재초기화 기준으로 사용
	UPRItemInstance_Weapon* TargetWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 지원하지 않는 슬롯이거나 무기 원본 또는 무기 데이터가 없는 경우
	if (!IsSupportedSlot(TargetSlot) || !IsValid(TargetWeaponInstance) || !IsValid(TargetWeaponInstance->GetWeaponData()))
	{
		// Mod 장착 실패. 검증 실패
		return false;
	}

	// 이미 같은 Mod가 장착된 경우
	if (TargetWeaponInstance->GetModData() == NewModData)
	{
		// 중복 장착 실패 사유 기록 // TODO: 이건주, 실패 사유 기록보다 로그 추가 필요
		TargetWeaponInstance->LastModFailReason = EPRWeaponModFailReason::AlreadyActive;

		// Mod 장착 실패
		return false;
	}

	// 새 Mod가 있고 무기와 태그 호환되지 않는 경우
	if (IsValid(NewModData) && !IsModCompatible(TargetWeaponInstance->GetWeaponData(), NewModData))
	{
		// 상태 차단 실패 사유 기록
		TargetWeaponInstance->LastModFailReason = EPRWeaponModFailReason::BlockedByState;

		// Mod 장착 실패
		return false;
	}

	// DataAsset 직접 장착 경로는 인벤토리 Mod Item을 거치지 않으므로 Item 참조 연결을 비운다
	TargetWeaponInstance->ClearEquippedModItem();

	// 무기 Item에 Mod 변경을 반영하고 장착 효과 갱신
	TargetWeaponInstance->OnModChanged(GetOwner(), NewModData);
	ApplyEquipModGE(TargetSlot, NewModData, TargetWeaponInstance);
	ApplyOverrideModResourceGE(TargetSlot, 0.0f, 0.0f, TargetWeaponInstance);

	// Mod 장착 또는 빈 Mod 상태를 마지막 처리 결과로 기록
	TargetWeaponInstance->LastModFailReason = IsValid(NewModData) ? EPRWeaponModFailReason::None : EPRWeaponModFailReason::MissingMod;

	// Mod 변경 결과를 공개 비주얼 정보에 반영
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬 Actor도 Mod 변경 결과에 맞춰 즉시 최신화
	RefreshAllWeaponActors();

	// 현재 활성 무기의 애니메이션 레이어 캐시 재확인
	RefreshAnimLayer();

	// 슬롯의 Mod 표시 상태가 바뀌었음을 HUD와 인벤토리 UI에 알린다
	OnWeaponEquipmentChanged.Broadcast(this, TargetSlot);

	// 서버 Mod 장착 처리가 확정된 뒤 Owner, 슬롯, 무기 데이터, Mod 데이터를 남겨 Mod 변경 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] Mod 장착 완료 | Owner = %s | Slot = %d | Weapon = %s | Mod = %s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(TargetSlot),
		*GetNameSafe(TargetWeaponInstance->GetWeaponData()),
		*GetNameSafe(NewModData));

	// Mod 상태, 슬롯 자원, 공개 비주얼, 로컬 Actor 갱신 마침
	// Mod 장착 성공
	return true;
}

void UPRWeaponManagerComponent::SetWeaponArmedStateInternal(EPRArmedState NewArmedState)
{
	// 무장 상태가 이미 같은 경우
	if (ArmedState == NewArmedState)
	{
		// 함수 조기 종료. 부착 갱신 불필요
		return;
	}

	// 서버 기준 무장 상태 갱신
	ArmedState = NewArmedState;

	// 서버 로컬 Actor도 무장 상태에 맞춰 즉시 최신화
	RefreshAllWeaponActors();
	RefreshAnimLayer();
}

void UPRWeaponManagerComponent::SetCurrentWeaponSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(TargetSlot))
	{
		// 활성 슬롯 전환 불가
		// 함수 조기 종료
		return;
	}

	// 전환 타겟 슬롯의 원본 Item. 활성 슬롯 구성과 장착 알림 대상 판단에 사용
	UPRItemInstance_Weapon* TargetWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 전환 대상 무기 Item 또는 무기 데이터가 없는 경우
	if (!IsValid(TargetWeaponInstance) || !IsValid(TargetWeaponInstance->GetWeaponData()))
	{
		// 빈 슬롯으로 전환 불가
		// 함수 조기 종료
		return;
	}

	// 이미 같은 슬롯이 활성 상태인 경우
	if (CurrentWeaponSlot == TargetSlot)
	{
		// 중복 전환 요청
		// 함수 조기 종료
		return;
	}

	// 전환 전 활성 슬롯. 이전 활성 무기 해제 알림 대상 판단에 사용
	const EPRWeaponSlotType PreWeaponSlot = CurrentWeaponSlot;

	// 활성 무기 사용 흐름을 타겟 슬롯으로 전환
	// 타겟 슬롯을 활성 슬롯으로 지정
	CurrentWeaponSlot = TargetSlot;

	// 현재 슬롯 태그 갱신
	UpdateCurrentWeaponSlotTags(PreWeaponSlot, CurrentWeaponSlot);

	// 활성 슬롯 기준 전투 값을 새 무기 데이터로 갱신
	ApplyCurrentWeaponGE(TargetWeaponInstance);

	// 슬롯 전환은 Actor를 재생성하지 않고 소켓 부착만 최신화하는 경로 유지
	RefreshAllWeaponActors();

	// 활성 슬롯 전환에 맞춰 애니메이션 레이어 갱신
	RefreshAnimLayer();

	OnWeaponEquipmentChanged.Broadcast(this, TargetSlot);
}

void UPRWeaponManagerComponent::RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(SlotType))
	{
		// 함수 조기 종료. 로컬 Actor 갱신 불가
		return;
	}

	// 슬롯 공개 비주얼 정보. Actor 생성, 제거, 부착 위치 판단에 사용
	const FPRWeaponVisualInfo& VisualInfo = GetVisualInfoBySlotType(SlotType);

	// 슬롯별 로컬 Actor 참조. 필요 시 제거 또는 재생성 대상으로 사용
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);

	// 비주얼 정보가 비어 있는 경우
	if (VisualInfo.IsEmpty())
	{
		// 해당 슬롯 Actor 제거
		DestroyWeaponActorForSlot(SlotType);

		// 함수 조기 종료. 빈 슬롯 비주얼 갱신 마침
		return;
	}

	// 월드, 소유자, 무기 데이터, Actor 클래스 중 하나라도 준비되지 않은 경우
	if (!IsValid(GetWorld()) || !IsValid(GetOwner()) || !IsValid(VisualInfo.WeaponData) || !IsValid(VisualInfo.WeaponData->WeaponActorClass))
	{
		// Actor 생성 보류 사유를 Owner, Slot, Weapon, ActorClass 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][%s] 무기 Actor 생성 보류 | Owner = %s | Slot = %s | Weapon = %s | ActorClass = %s"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(SlotType),
			*GetNameSafe(VisualInfo.WeaponData.Get()),
			IsValid(VisualInfo.WeaponData) ? *GetNameSafe(VisualInfo.WeaponData->WeaponActorClass.Get()) : TEXT("None"));

		// 공개 Actor 생성 조건 미충족
		// 함수 조기 종료
		return;
	}

	// Actor가 없거나 공개 무기 데이터의 Actor 클래스와 다르면 재생성 필요
	const bool bNeedsRespawn = !IsValid(WeaponActor)
		|| WeaponActor->GetClass() != VisualInfo.WeaponData->WeaponActorClass;

	// 같은 슬롯에 다른 무기를 다시 장착했거나 Actor가 없는 경우
	if (bNeedsRespawn)
	{
		// 기존 Actor가 있으면 새 Actor 생성 전에 제거
		DestroyWeaponActorForSlot(SlotType);

		// 소유자와 충돌 정책을 고정해 장착 비주얼 Actor를 항상 생성
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 공개 무기 데이터의 Actor 클래스로 슬롯 Actor 생성
		WeaponActor = GetWorld()->SpawnActor<APRWeaponActor>(
			VisualInfo.WeaponData->WeaponActorClass,
			FTransform::Identity,
			SpawnParameters);

		// 슬롯 Actor 생성 결과를 Owner, Slot, Weapon, Actor, Class 기준으로 추적
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[WeaponManagerComponent][%s] 무기 Actor 생성 완료 | Owner = %s | Slot = %s | Weapon = %s | Actor = %s | Class = %s"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(SlotType),
			*GetNameSafe(VisualInfo.WeaponData.Get()),
			*GetNameSafe(WeaponActor.Get()),
			*GetNameSafe(VisualInfo.WeaponData->WeaponActorClass.Get()));
	}

	// Actor 생성에 실패한 경우
	if (!IsValid(WeaponActor))
	{
		// 부착 갱신 불가
		// 함수 조기 종료
		return;
	}

	// 생성되었거나 재사용 가능한 Actor를 현재 carry state에 맞춰 부착
	RefreshWeaponAttachmentForSlot(SlotType);
	
	// 26.05.08, Yuchan, State_Armed 추가
	if (CachedASC.IsValid())
	{
		if (ArmedState == EPRArmedState::Armed)
		{
			CachedASC->AddLooseGameplayTag(PRGameplayTags::State_Armed);	
		}
		else
		{
			CachedASC->RemoveLooseGameplayTag(PRGameplayTags::State_Armed);
		}
	}
}

void UPRWeaponManagerComponent::RefreshAllWeaponActors()
{
	// 주무기 슬롯 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);

	// 보조무기 슬롯 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(SlotType))
	{
		// 함수 조기 종료. 부착 갱신 불가
		return;
	}

	// 부착 갱신 대상 Actor와 캐릭터 소유자. 둘 다 있어야 Mesh 소켓 부착 가능
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawnOwner());

	// 장착된 슬롯 Actor 또는 캐릭터 소유자가 없는 경우
	if (!IsValid(WeaponActor) || !IsValid(OwnerCharacter))
	{
		// 함수 조기 종료. 부착 대상 조건 미충족
		return;
	}

	// 현재 무장 상태와 활성 슬롯을 기준으로 비무장 또는 무장 부착 소켓 결정
	const EPRArmedState SlotArmedState = GetSlotWeaponArmedState(SlotType);
	const FName AttachSocketName = GetAttachTargetSocketName(SlotType, SlotArmedState);

	// 슬롯 Actor 부착 요청을 Owner, Slot, Actor, Socket, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무기 Actor 부착 요청 | Owner = %s | Slot = %s | Actor = %s | Socket = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(SlotType),
		*GetNameSafe(WeaponActor),
		*AttachSocketName.ToString(),
		*UEnum::GetValueAsString(SlotArmedState));

	// 결정된 소켓으로 무기 Actor를 캐릭터 Mesh에 부착
	WeaponActor->AttachToOwnerMesh(OwnerCharacter, AttachSocketName);
}

void UPRWeaponManagerComponent::OnRep_CurrentWeaponSlot(EPRWeaponSlotType OldCurrentWeaponSlot)
{
	// 활성 슬롯 복제 값이 이전 값과 같은 경우
	if (OldCurrentWeaponSlot == CurrentWeaponSlot)
	{
		// 함수 조기 종료. 로컬 Actor 갱신 필요없음
		return;
	}

	const FPRWeaponVisualInfo& CurrentVisualInfo = GetCurrentWeaponVisualInfo();

	// 활성 슬롯 동기화를 Owner, 이전 슬롯, 새 슬롯, 무기 데이터 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 활성 슬롯 동기화 | Owner = %s | OldSlot = %s | NewSlot = %s | Weapon = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(OldCurrentWeaponSlot),
		*UEnum::GetValueAsString(CurrentWeaponSlot),
		*GetNameSafe(CurrentVisualInfo.WeaponData.Get()));

	// 활성 슬롯 변화에 맞춰 두 슬롯의 Actor와 부착 상태 갱신
	RefreshAllWeaponActors();

	// 활성 슬롯 변화에 맞춰 애니메이션 레이어 갱신
	RefreshAnimLayer();

	OnWeaponEquipmentChanged.Broadcast(this, CurrentWeaponSlot);
}

void UPRWeaponManagerComponent::OnRep_PrimaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo)
{
	// 주무기 공개 비주얼 복제 값이 이전 값과 같은 경우
	if (OldVisualInfo == PrimaryVisualInfo)
	{
		// 함수 조기 종료. 주무기 Actor 갱신 불필요
		return;
	}

	// 주무기 공개 비주얼 동기화를 Owner, 이전 무기, 새 무기, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 주무기 비주얼 동기화 | Owner = %s | OldWeapon = %s | NewWeapon = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OldVisualInfo.WeaponData.Get()),
		*GetNameSafe(PrimaryVisualInfo.WeaponData.Get()),
		*UEnum::GetValueAsString(GetSlotWeaponArmedState(EPRWeaponSlotType::Primary)));

	// 주무기 공개 비주얼 변화에 맞춰 주무기 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);

	// 주무기 공개 비주얼의 무기 또는 Mod 표시 상태 변경을 UI에 알린다
	OnWeaponEquipmentChanged.Broadcast(this, EPRWeaponSlotType::Primary);

	// 현재 활성 주무기 비주얼에 맞춰 애니메이션 레이어 갱신
	if (CurrentWeaponSlot == EPRWeaponSlotType::Primary)
	{
		RefreshAnimLayer();
	}
}

void UPRWeaponManagerComponent::OnRep_SecondaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo)
{
	// 보조무기 공개 비주얼 복제 값이 이전 값과 같은 경우
	if (OldVisualInfo == SecondaryVisualInfo)
	{
		// 함수 조기 종료. 보조무기 Actor 갱신 불필요
		return;
	}

	// 보조무기 공개 비주얼 동기화를 Owner, 이전 무기, 새 무기, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 보조무기 비주얼 동기화 | Owner = %s | OldWeapon = %s | NewWeapon = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OldVisualInfo.WeaponData.Get()),
		*GetNameSafe(SecondaryVisualInfo.WeaponData.Get()),
		*UEnum::GetValueAsString(GetSlotWeaponArmedState(EPRWeaponSlotType::Secondary)));

	// 보조무기 공개 비주얼 변화에 맞춰 보조무기 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	// 보조무기 공개 비주얼의 무기 또는 Mod 표시 상태 변경을 UI에 알린다
	OnWeaponEquipmentChanged.Broadcast(this, EPRWeaponSlotType::Secondary);

	// 현재 활성 보조무기 비주얼에 맞춰 애니메이션 레이어 갱신
	if (CurrentWeaponSlot == EPRWeaponSlotType::Secondary)
	{
		RefreshAnimLayer();
	}
}

void UPRWeaponManagerComponent::OnRep_ArmedState(EPRArmedState OldArmedState)
{
	// 무장 상태 복제 값이 이전 값과 같은 경우
	if (OldArmedState == ArmedState)
	{
		// 함수 조기 종료. 로컬 Actor 부착 갱신 불필요
		return;
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,1.0f,FColor::Yellow,FString::Printf(
			TEXT("%s ArmedState: %s"),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(ArmedState)));
	}

	// 무장 상태 동기화를 Owner, 이전 상태, 새 상태 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무장 상태 동기화 | Owner = %s | OldState = %s | NewState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(OldArmedState),
		*UEnum::GetValueAsString(ArmedState));

	// 무장 상태 변화에 맞춰 두 슬롯의 Actor와 부착 상태 갱신
	RefreshAllWeaponActors();
	RefreshAnimLayer();
}

void UPRWeaponManagerComponent::OnRep_PrimaryWeaponInstance()
{
	OnWeaponEquipmentChanged.Broadcast(this, EPRWeaponSlotType::Primary);
}

void UPRWeaponManagerComponent::OnRep_SecondaryWeaponInstance()
{
	OnWeaponEquipmentChanged.Broadcast(this, EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::Server_EquipWeapon_Implementation(UPRItemInstance_Weapon* WeaponItem)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 Item 참조 기반 장착 처리
	EquipWeaponInternal(WeaponItem);
}

void UPRWeaponManagerComponent::Server_UnequipWeaponSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 슬롯 해제 처리
	UnequipWeaponFromSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SetCurrentWeaponSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 활성 슬롯 전환 처리
	SetCurrentWeaponSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SetWeaponArmedState_Implementation(EPRArmedState NewArmedState)
{
	// 서버 권위에서 무장 상태 변경 처리
	SetWeaponArmedStateInternal(NewArmedState);
}

void UPRWeaponManagerComponent::Server_AttachModToSlot_Implementation(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 Mod 장착 처리
	AttachModToSlotInternal(TargetSlot, NewModData);
}

FPRWeaponVisualInfo UPRWeaponManagerComponent::BuildVisualInfo(EPRWeaponSlotType SlotType, const UPRItemInstance_Weapon* WeaponItem) const
{
	// 기본 공개 비주얼 정보는 슬롯 타입을 먼저 반영
	FPRWeaponVisualInfo NewVisualInfo;
	NewVisualInfo.SlotType = SlotType;

	// 무기 Item 또는 무기 데이터가 없는 경우
	if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
	{
		// 슬롯 타입은 유지하되 무기와 Mod 공개 데이터 초기화
		NewVisualInfo.Reset(SlotType);

		// 빈 공개 비주얼 정보 리턴
		return NewVisualInfo;
	}

	// 원본 Item 기준으로 공개 가능한 무기와 Mod 데이터를 구성
	NewVisualInfo.WeaponData = WeaponItem->GetWeaponData();
	NewVisualInfo.ModData = WeaponItem->GetModData();

	// 공개 비주얼 정보 구성 마침
	return NewVisualInfo;
}

EPRArmedState UPRWeaponManagerComponent::GetSlotWeaponArmedState(EPRWeaponSlotType SlotType) const
{
	// 현재 무장 상태가 Armed이고 활성 슬롯과 일치하는 슬롯인 경우
	if (ArmedState == EPRArmedState::Armed
		&& CurrentWeaponSlot == SlotType)
	{
		// 무장 상태로 리턴
		return EPRArmedState::Armed;
	}

	// 활성 슬롯이 아니거나 비무장 상태인 슬롯은 비무장 상태로 리턴
	return EPRArmedState::Unarmed;
}

FName UPRWeaponManagerComponent::GetAttachTargetSocketName(EPRWeaponSlotType SlotType, EPRArmedState SlotArmedState) const
{	
	// 소켓 네임을 얻기 위해 무기슬롯의 무기 데이터를 가져온다 
	UPRWeaponDataAsset* Data = GetWeaponDataBySlotType(SlotType);
	if (!IsValid(Data))
	{
		return NAME_None;
	}
	
	// 무장 상태에 맞는 소켓 이름을 가져온다 
	FName TargetSocketName = (SlotArmedState == EPRArmedState::Armed) 
	? FName(PREnumHelper::EnumToFName(Data->ArmedSocketName))
	: FName(PREnumHelper::EnumToFName(Data->StowedSocketName));
	return TargetSocketName;
}

void UPRWeaponManagerComponent::RefreshVisualInfosFromCurrentState()
{
	// 서버 원본 주무기 상태를 공개 비주얼 정보로 재구성
	PrimaryVisualInfo = BuildVisualInfo(EPRWeaponSlotType::Primary, PrimaryWeaponInstance);

	// 서버 원본 보조무기 상태를 공개 비주얼 정보로 재구성
	SecondaryVisualInfo = BuildVisualInfo(EPRWeaponSlotType::Secondary, SecondaryWeaponInstance);
}

EPRWeaponSlotType UPRWeaponManagerComponent::ResolveNextWeaponSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const
{
	// 기본값은 빈 활성 슬롯. 남은 무기가 없으면 그대로 리턴
	EPRWeaponSlotType NextWeaponSlot = EPRWeaponSlotType::None;

	// 해제된 슬롯이 주무기가 아니고 주무기 원본이 남아 있는 경우
	if (UnequippedSlot != EPRWeaponSlotType::Primary && IsValid(PrimaryWeaponInstance))
	{
		// 주무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Primary;
	}

	// 해제된 슬롯이 보조무기가 아니고 보조무기 원본이 남아 있는 경우
	if (UnequippedSlot != EPRWeaponSlotType::Secondary && IsValid(SecondaryWeaponInstance))
	{
		// 보조무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Secondary;
	}

	// 빈 활성 슬롯 리턴. 남은 장착 무기 없음
	return NextWeaponSlot;
}

bool UPRWeaponManagerComponent::IsSupportedSlot(EPRWeaponSlotType SlotType) const
{
	// 무기 매니저가 관리하는 주무기와 보조무기 슬롯만 지원
	return SlotType == EPRWeaponSlotType::Primary || SlotType == EPRWeaponSlotType::Secondary;
}

EPRWeaponSlotType UPRWeaponManagerComponent::ResolveWeaponItemSlot(const UPRItemInstance_Weapon* WeaponItem) const
{
	// 유효하지 않은 Item은 어떤 슬롯의 원본으로도 인정하지 않는다
	if (!IsValid(WeaponItem))
	{
		return EPRWeaponSlotType::None;
	}

	if (PrimaryWeaponInstance == WeaponItem)
	{
		return EPRWeaponSlotType::Primary;
	}

	if (SecondaryWeaponInstance == WeaponItem)
	{
		return EPRWeaponSlotType::Secondary;
	}

	return EPRWeaponSlotType::None;
}

void UPRWeaponManagerComponent::UpdateCurrentWeaponSlotTags(EPRWeaponSlotType OldSlot, EPRWeaponSlotType NewSlot)
{
	if (OldSlot == NewSlot)
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = CachedASC.IsValid() ? CachedASC.Get() : nullptr;
	if (!IsValid(ASC))
	{
		if (APRPlayerState* PlayerState = GetOwnerPlayerState())
		{
			ASC = PlayerState->GetPRAbilitySystemComponent();
		}
	}

	if (!IsValid(ASC))
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	const FGameplayTag OldTag = ResolveCurrentSlotTag(OldSlot);
	if (OldTag.IsValid())
	{
		// 이전 슬롯 태그 제거
		RemoveASCStateTag(ASC, OwnerActor, OldTag);
		RemoveASCStateTag(ASC, OwnerActor, ResolveCurrentSlotBaseFireTag(OldSlot));
		RemoveASCStateTag(ASC, OwnerActor, ResolveCurrentSlotModFireTag(OldSlot));
	}

	const FGameplayTag NewTag = ResolveCurrentSlotTag(NewSlot);
	if (NewTag.IsValid())
	{
		// 현재 슬롯 태그 추가
		AddASCStateTag(ASC, OwnerActor, NewTag);
		RefreshCurrentWeaponFireModeTags();
	}
}

APRPlayerState* UPRWeaponManagerComponent::GetOwnerPlayerState() const
{
	return Cast<APRPlayerState>(GetOwner());
}

FPREquipSlotEffectHandles& UPRWeaponManagerComponent::GetMutableEquipEffectHandlesBySlot(EPRWeaponSlotType SlotType)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryEquipEffectHandles;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryEquipEffectHandles;
	}

	checkNoEntry();
	return PrimaryEquipEffectHandles;
}

void UPRWeaponManagerComponent::RemoveEquipEffectHandle(FActiveGameplayEffectHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	if (!CachedASC.IsValid())
	{
		return;
	}

	CachedASC->RemoveActiveGameplayEffect(Handle);
	Handle = FActiveGameplayEffectHandle();
}

void UPRWeaponManagerComponent::RemoveSlotEquipEffects(EPRWeaponSlotType SlotType)
{
	if (!IsSupportedSlot(SlotType))
	{
		return;
	}

	// 타겟 슬롯 GE 제거
	FPREquipSlotEffectHandles& SlotHandles = GetMutableEquipEffectHandlesBySlot(SlotType);
	RemoveEquipEffectHandle(SlotHandles.AmmoMaxHandle);
	RemoveEquipEffectHandle(SlotHandles.ModMaxHandle);
}


void UPRWeaponManagerComponent::RemoveAllEquipEffects()
{
	// 모든 EquipEffects 제거
	RemoveSlotEquipEffects(EPRWeaponSlotType::Primary);
	RemoveSlotEquipEffects(EPRWeaponSlotType::Secondary);
	RemoveEquipEffectHandle(CurrentWeaponEffectHandle);
}


void UPRWeaponManagerComponent::CacheAmmoRatiosForSlot(EPRWeaponSlotType SlotType) const
{
	if (!CachedWeaponSet.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	APRPlayerState* PlayerState = GetOwnerPlayerState();
	if (!IsValid(PlayerState))
	{
		return;
	}

	const EPRAmmoType AmmoType = SlotType == EPRWeaponSlotType::Primary ? EPRAmmoType::Primary : EPRAmmoType::Secondary;
	const float MaxMagazineAmmo = CachedWeaponSet->GetMaxMagazineAmmoByType(AmmoType);
	const float MaxReserveAmmo = CachedWeaponSet->GetMaxReserveAmmoByType(AmmoType);
	if (MaxMagazineAmmo <= KINDA_SMALL_NUMBER || MaxReserveAmmo <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float MagazineRatio = CachedWeaponSet->GetMagazineAmmoByType(AmmoType) / MaxMagazineAmmo;
	const float ReserveRatio = CachedWeaponSet->GetReserveAmmoByType(AmmoType) / MaxReserveAmmo;
	PlayerState->SetCachedAmmoRatios(SlotType, MagazineRatio, ReserveRatio);
}

void UPRWeaponManagerComponent::RestoreAmmoFromCachedRatios(EPRWeaponSlotType SlotType, UObject* SourceObject) const
{
	if (!CachedASC.IsValid() || !CachedWeaponSet.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	const APRPlayerState* PlayerState = GetOwnerPlayerState();
	if (!IsValid(PlayerState))
	{
		return;
	}

	// 플레이어 스테이트에 기록된 슬롯 AmmoRatios를 불러온다
	float MagazineRatio = 1.0f;
	float ReserveRatio = 1.0f;
	PlayerState->GetCachedAmmoRatios(SlotType, MagazineRatio, ReserveRatio);

	// 적용할 슬롯의 최대 Ammo 값을 불러온다
	const EPRAmmoType AmmoType = SlotType == EPRWeaponSlotType::Primary ? EPRAmmoType::Primary : EPRAmmoType::Secondary;
	const float MaxMagazineAmmo = CachedWeaponSet->GetMaxMagazineAmmoByType(AmmoType);
	const float MaxReserveAmmo = CachedWeaponSet->GetMaxReserveAmmoByType(AmmoType);

	// 비율에 맞는 CurrentAmmo 값을 오버라이드
	ApplyOverrideAmmoGE(
		SlotType,
		FMath::Clamp(MaxMagazineAmmo * MagazineRatio, 0.0f, MaxMagazineAmmo),
		FMath::Clamp(MaxReserveAmmo * ReserveRatio, 0.0f, MaxReserveAmmo),
		SourceObject);
}

void UPRWeaponManagerComponent::ClearAmmoAttributesForSlot(EPRWeaponSlotType SlotType, UObject* SourceObject) const
{
	if (!CachedASC.IsValid() || !IsSupportedSlot(SlotType))
	{
		return;
	}

	ApplyOverrideAmmoGE(SlotType, 0.0f, 0.0f, SourceObject);
}

void UPRWeaponManagerComponent::RefreshAnimLayer()
{
	// 현재 애니메이션 레이어를 갱신할 대상 캐릭터와 Mesh
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawnOwner());
	if (!IsValid(OwnerCharacter))
	{
		// 함수 조기 종료. 애니메이션 레이어 갱신 대상 없음
		return;
	}

	USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh();
	if (!IsValid(OwnerMesh))
	{
		// 함수 조기 종료. 메시 컴포넌트 없음
		return;
	}

	// 현재 활성 무기에 연결해야 할 애니메이션 레이어 클래스를 결정
	TSubclassOf<UAnimInstance> TargetAnimLayerClass = nullptr;

	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = GetCurrentWeaponVisualInfo();
	if (ArmedState == EPRArmedState::Armed
		&& !CurrentWeaponVisualInfo.IsEmpty()
		&& IsValid(CurrentWeaponVisualInfo.WeaponData))
	{
		TargetAnimLayerClass = CurrentWeaponVisualInfo.WeaponData->WeaponAnimLayerClass;
	}

	// 무기 레이어가 지정되지 않은 상태(무기 미장착 또는 무기에 레이어 미지정)면 캐릭터 기본 레이어로 폴백
	if (!IsValid(TargetAnimLayerClass))
	{
		if (const APRCharacterBase* PRCharacter = Cast<APRCharacterBase>(OwnerCharacter))
		{
			TargetAnimLayerClass = PRCharacter->GetDefaultAnimLayerClass();
		}
	}

	// 이미 목표 레이어가 연결된 경우
	if (CurrentLinkedAnimLayerClass == TargetAnimLayerClass)
	{
		// 함수 조기 종료. 애니메이션 레이어 갱신 불필요
		return;
	}

	// 기존에 연결된 레이어가 있다면 먼저 해제
	if (IsValid(CurrentLinkedAnimLayerClass))
	{
		OwnerMesh->UnlinkAnimClassLayers(CurrentLinkedAnimLayerClass);
		CurrentLinkedAnimLayerClass = nullptr;
	}

	// 새 목표 레이어(무기 레이어 또는 기본 레이어)가 있으면 메시 컴포넌트에 연결
	if (IsValid(TargetAnimLayerClass))
	{
		OwnerMesh->LinkAnimClassLayers(TargetAnimLayerClass);
		CurrentLinkedAnimLayerClass = TargetAnimLayerClass;
	}
}

TObjectPtr<UPRItemInstance_Weapon>& UPRWeaponManagerComponent::GetMutableWeaponInstanceBySlot(EPRWeaponSlotType SlotType)
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 원본 Item 참조 리턴
		return PrimaryWeaponInstance;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 원본 Item 참조 리턴
		return SecondaryWeaponInstance;
	}

	// 지원하지 않는 슬롯은 호출 경로 오류로 간주
	checkNoEntry();

	// 빌드 구성을 위해 주무기 원본 참조 리턴
	return PrimaryWeaponInstance;
}

APRWeaponActor* UPRWeaponManagerComponent::GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 Actor 리턴
		return PrimaryWeaponActor;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 Actor 리턴
		return SecondaryWeaponActor;
	}

	// 지원하지 않는 슬롯은 Actor 없음으로 리턴
	return nullptr;
}

TObjectPtr<APRWeaponActor>& UPRWeaponManagerComponent::GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType)
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 Actor 참조 리턴
		return PrimaryWeaponActor;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 Actor 참조 리턴
		return SecondaryWeaponActor;
	}

	// 지원하지 않는 슬롯은 호출 경로 오류로 간주
	checkNoEntry();

	// 빌드 구성을 위해 주무기 Actor 참조 리턴
	return PrimaryWeaponActor;
}

void UPRWeaponManagerComponent::DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	// 슬롯별 로컬 Actor 참조. 제거 후 참조 초기화에 사용
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);

	// 제거할 Actor가 없는 경우
	if (!IsValid(WeaponActor))
	{
		// 함수 조기 종료. Actor 제거 불필요
		return;
	}

	// 슬롯 Actor 제거를 Owner, Slot, Actor 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무기 Actor 제거. DestroyWeaponActorForSlot | Owner = %s | Slot = %s | Actor = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(SlotType),
		*GetNameSafe(WeaponActor.Get()));

	// 로컬 무기 Actor 제거
	WeaponActor->Destroy();

	// 제거된 Actor 참조 초기화
	WeaponActor = nullptr;
}
