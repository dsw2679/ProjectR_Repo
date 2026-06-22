// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (강화 등급별 무기 공격력 실시간 연산 구현)
// Author: 배유찬 (무기 런타임 탄약 상태 및 업그레이드 스탯 보존 구현)
// Author: 이건주 (무기 Mod 슬롯 상태 동기화 및 3D 모델 프리뷰 구현)
#include "PRItemInstance_Weapon.h"

#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Utils/PRGameplayStatics.h"


void UPRItemInstance_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRItemInstance_Weapon, ModData);
	DOREPLIFETIME(UPRItemInstance_Weapon, EquippedModItem);
	DOREPLIFETIME(UPRItemInstance_Weapon, UpgradeLevel);
}

bool UPRItemInstance_Weapon::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 인벤토리 UI 선택이나 장비 상호작용은 InventoryComponent 요청을 거쳐 이 함수로 도착함
	// 여기서는 무기 장착 방식을 결정하지 않고 플레이어의 WeaponManager에 실제 장착 처리를 맡김
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// WeaponManager는 PlayerState에 있으며 슬롯 소유권 검증, 무기 Actor 갱신, AbilitySet 부여를 함께 처리함
	// ItemInstance가 직접 슬롯 상태를 바꾸면 장착 표시, 복제 상태, 어빌리티 부여 순서가 갈라질 수 있음
	UPRWeaponManagerComponent* WeaponManager = UPRGameplayStatics::GetWeaponManagerComponent(ActivationContext.UserActor);
	if (!IsValid(WeaponManager))
	{
		return false;
	}

	return WeaponManager->EquipWeapon(this);
}

bool UPRItemInstance_Weapon::DeactivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 인벤토리 UI의 무기 해제 항목은 표시만 명령처럼 보이고 실제로는 이 무기 ItemInstance를 비활성화함
	// 따라서 UI가 WeaponManager를 직접 호출하지 않아도 장착 무기 클릭과 해제 항목 클릭이 같은 경로를 사용함
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// 해제할 슬롯은 UI가 마지막으로 연 목록이 아니라 무기 데이터가 가진 슬롯 타입으로 결정함
	// 목록 상태가 갱신되거나 닫혀도 ItemInstance 자기 데이터만으로 해제 대상을 다시 찾을 수 있음
	const UPRWeaponDataAsset* WeaponData = GetWeaponData();
	if (!IsValid(WeaponData))
	{
		return false;
	}

	// WeaponManager의 해제 처리에는 활성 슬롯 전환, 무기 Actor 갱신, AbilitySet 회수가 묶여 있음
	// 이 순서를 한 곳에 모아 두기 위해 ItemInstance는 해제 대상 슬롯만 알려 줌
	UPRWeaponManagerComponent* WeaponManager = UPRGameplayStatics::GetWeaponManagerComponent(ActivationContext.UserActor);
	if (!IsValid(WeaponManager))
	{
		return false;
	}

	return WeaponManager->UnequipWeaponFromSlot(WeaponData->SlotType);
}

void UPRItemInstance_Weapon::InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount)
{
	Super::InitializeItem(InItemData, InitialStackCount);
}

void UPRItemInstance_Weapon::InitializeMod(UPRWeaponModDataAsset* InModData)
{
	ModData = InModData;
	ClearEquippedModItem();
	bIsEquippedWeaponSlot = false;
}

UPRWeaponDataAsset* UPRItemInstance_Weapon::GetWeaponData() const
{
	return Cast<UPRWeaponDataAsset>(ItemData);
}

void UPRItemInstance_Weapon::FillSaveEntry(FPRWeaponItemSaveEntry& OutEntry) const
{
	// 저장 엔트리 기본값
	OutEntry = FPRWeaponItemSaveEntry();
	OutEntry.WeaponData = GetWeaponData();
	OutEntry.StackCount = GetStackCount();
	OutEntry.State.EnhancementLevel = UpgradeLevel;
}

void UPRItemInstance_Weapon::ApplySaveEntry(const FPRWeaponItemSaveEntry& InEntry)
{
	// v1 인스턴스 상태 복원
	StackCount = FMath::Max(InEntry.StackCount, 0);
	UpgradeLevel = FMath::Max(InEntry.State.EnhancementLevel, 0);
}

bool UPRItemInstance_Weapon::MatchesWeaponData(const UPRWeaponDataAsset* InWeaponData) const
{
	return GetWeaponData() == InWeaponData;
}

bool UPRItemInstance_Weapon::HasEquippedModItem() const
{
	return IsValid(EquippedModItem);
}

void UPRItemInstance_Weapon::SetModData(UPRWeaponModDataAsset* NewModData)
{
	ModData = NewModData;
}

void UPRItemInstance_Weapon::SetEquippedModItem(UPRItemInstance_Mod* NewModItem)
{
	EquippedModItem = NewModItem;
}

void UPRItemInstance_Weapon::ClearEquippedModItem()
{
	EquippedModItem = nullptr;
}

void UPRItemInstance_Weapon::OnEquipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (bIsEquippedWeaponSlot)
	{
		return;
	}

	// 슬롯 장착 상태 기록
	bIsEquippedWeaponSlot = true;
	GrantEquippedAbilitySets(OwnerActor);
}

void UPRItemInstance_Weapon::OnUnequipped(AActor* OwnerActor)
{
	// 슬롯 장착 상태 해제
	bIsEquippedWeaponSlot = false;

	if (!IsValid(OwnerActor))
	{
		return;
	}

	ClearEquippedAbilitySets(OwnerActor);
	ResetTransientRuntimeOnDeactivate();
}


void UPRItemInstance_Weapon::OnModChanged(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData)
{
	// Mod 교체 생명주기에서는 기존 핸들을 정리한 뒤 새 Mod 기준으로 재구성한다
	if (!IsValid(OwnerActor))
	{
		return;
	}

	RebuildModAbility(OwnerActor, NewModData);
}

void UPRItemInstance_Weapon::GrantEquippedAbilitySets(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(OwnerActor));
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (UPRWeaponDataAsset* WeaponData = GetWeaponData())
	{
		const FGameplayTagContainer SlotBlockTags = BuildOppositeSlotBlockTags(WeaponData);
		const FGameplayTagContainer BaseFireBlockTags = BuildCurrentSlotFireModeBlockTags(WeaponData, EPRWeaponFireModeState::ModFire);
		WeaponData->GiveToAbilitySystem(ASC, WeaponAbilityHandles, this, &SlotBlockTags, &BaseFireBlockTags);
	}

	if (IsValid(ModData))
	{
		const FGameplayTagContainer SlotBlockTags = BuildOppositeSlotBlockTags(GetWeaponData());
		ModData->GiveToAbilitySystem(ASC, ModAbilityHandles, this, &SlotBlockTags);
	}

	LastWeaponFailReason = EPRWeaponActionFailReason::None;
	LastModFailReason = EPRWeaponModFailReason::None;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("무기 아이템의 어빌리티 셋 부여. Owner=%s Weapon=%s Mod=%s WeaponAbilities=%d WeaponEffects=%d ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		WeaponAbilityHandles.AbilityHandles.Num(),
		WeaponAbilityHandles.EffectHandles.Num(),
		ModAbilityHandles.AbilityHandles.Num(),
		ModAbilityHandles.EffectHandles.Num());
}

void UPRItemInstance_Weapon::ClearEquippedAbilitySets(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(OwnerActor));
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	ASC->ClearAbilitySetByHandles(WeaponAbilityHandles);
	ASC->ClearAbilitySetByHandles(ModAbilityHandles);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("무기 아이템의 어빌리티 셋 회수. Owner=%s Weapon=%s Mod=%s"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData));
}

void UPRItemInstance_Weapon::RebuildModAbility(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(OwnerActor));
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		SetModData(NewModData);
		return;
	}

	const UPRWeaponModDataAsset* PreviousModData = ModData;
	const bool bWasEquipped = IsEquippedWeaponSlot();

	ASC->ClearAbilitySetByHandles(ModAbilityHandles);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon item mod rebuild start. Owner=%s Weapon=%s PrevMod=%s NewMod=%s WasEquipped=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(PreviousModData),
		*GetNameSafe(NewModData),
		bWasEquipped ? 1 : 0);

	SetModData(NewModData);

	if (!IsValid(ModData))
	{
		FireModeState = EPRWeaponFireModeState::BaseFire;
		LastModFailReason = EPRWeaponModFailReason::MissingMod;
		ModEffectEndServerWorldTimeSeconds = 0.0f;
		return;
	}

	LastModFailReason = EPRWeaponModFailReason::None;

	if (bWasEquipped)
	{
		const FGameplayTagContainer SlotBlockTags = BuildOppositeSlotBlockTags(GetWeaponData());
		NewModData->GiveToAbilitySystem(ASC, ModAbilityHandles, this, &SlotBlockTags);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon item mod rebuild complete. Owner=%s Weapon=%s CurrentMod=%s ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		ModAbilityHandles.AbilityHandles.Num(),
		ModAbilityHandles.EffectHandles.Num());
}

void UPRItemInstance_Weapon::ResetTransientRuntimeOnDeactivate()
{
	FireModeState = EPRWeaponFireModeState::BaseFire;
	ModEffectEndServerWorldTimeSeconds = 0.0f;

	LastWeaponFailReason = EPRWeaponActionFailReason::None;
	LastModFailReason = EPRWeaponModFailReason::None;
}

// 같은 슬롯 사격 모드 차단 태그 구성
FGameplayTagContainer UPRItemInstance_Weapon::BuildCurrentSlotFireModeBlockTags(const UPRWeaponDataAsset* WeaponData, EPRWeaponFireModeState BlockedFireModeState)
{
	FGameplayTagContainer BlockTags;
	if (!IsValid(WeaponData))
	{
		return BlockTags;
	}

	if (WeaponData->SlotType == EPRWeaponSlotType::Primary)
	{
		BlockTags.AddTag(BlockedFireModeState == EPRWeaponFireModeState::ModFire
			? PRGameplayTags::State_CurrentWeaponSlot_Primary_Mod
			: PRGameplayTags::State_CurrentWeaponSlot_Primary_Base);
	}
	else if (WeaponData->SlotType == EPRWeaponSlotType::Secondary)
	{
		BlockTags.AddTag(BlockedFireModeState == EPRWeaponFireModeState::ModFire
			? PRGameplayTags::State_CurrentWeaponSlot_Secondary_Mod
			: PRGameplayTags::State_CurrentWeaponSlot_Secondary_Base);
	}

	return BlockTags;
}

FGameplayTagContainer UPRItemInstance_Weapon::BuildOppositeSlotBlockTags(const UPRWeaponDataAsset* WeaponData)
{
	FGameplayTagContainer BlockTags;
	if (!IsValid(WeaponData))
	{
		return BlockTags;
	}

	if (WeaponData->SlotType == EPRWeaponSlotType::Primary)
	{
		BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Secondary);
	}
	else if (WeaponData->SlotType == EPRWeaponSlotType::Secondary)
	{
		BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Primary);
	}

	return BlockTags;
}

float UPRItemInstance_Weapon::GetRemainingModDurationSeconds(float ServerWorldTimeSeconds) const
{
	return FMath::Max(ModEffectEndServerWorldTimeSeconds - ServerWorldTimeSeconds, 0.0f);
}

bool UPRItemInstance_Weapon::SetUpgradeLevel(int32 NewLevel)
{
	if (NewLevel < 0)
	{
		return false;
	}

	if (UpgradeLevel == NewLevel)
	{
		return false;
	}

	UpgradeLevel = NewLevel;
	NotifyInventoryChanged(EPRInventoryChangeReason::WeaponUpgradeChanged);
	return true;
}

bool UPRItemInstance_Weapon::IncreaseUpgradeLevel()
{
	return SetUpgradeLevel(UpgradeLevel + 1);
}

void UPRItemInstance_Weapon::OnRep_ModData()
{
	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}

void UPRItemInstance_Weapon::OnRep_EquippedModItem()
{
	// 클라이언트에서 장착 Mod Item 참조만 갱신되는 상황을 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Weapon item mod item replicated. Item = %s | Weapon = %s | Mod = %s | ModItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		*GetNameSafe(EquippedModItem.Get()));

	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}

void UPRItemInstance_Weapon::OnRep_UpgradeLevel()
{
	NotifyInventoryChanged(EPRInventoryChangeReason::WeaponUpgradeChanged);
}
