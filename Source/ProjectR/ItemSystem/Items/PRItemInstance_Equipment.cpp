// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Item 인스턴스 Equipment 구현)
#include "PRItemInstance_Equipment.h"

#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

namespace
{
	// 0이 아닌 장비 보너스 존재 여부
	bool HasAnyCommonStatBonus(const UPREquipmentDataAsset* EquipmentData)
	{
		return IsValid(EquipmentData)
			&& (!FMath::IsNearlyZero(EquipmentData->GetMaxHealthBonus())
				|| !FMath::IsNearlyZero(EquipmentData->GetArmorBonus())
				|| !FMath::IsNearlyZero(EquipmentData->GetAttackPowerBonus())
				|| !FMath::IsNearlyZero(EquipmentData->GetMaxStaminaBonus()));
	}
}

void UPRItemInstance_Equipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UPRItemInstance_Equipment::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 인벤토리 UI의 장비 목록 선택은 InventoryComponent 요청을 거쳐 서버에서 이 함수로 도착
	// 장비 슬롯 교체, 기존 장비 해제, 복제용 외형 정보 갱신은 EquipmentManager가 한 번에 처리
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// UserActor는 로컬 UI에서는 PlayerController로 넘어오며 서버에서는 PlayerState의 EquipmentManager로 해석
	// 장비 ItemInstance는 자기 슬롯 데이터만 알고 실제 장착 상태 배열은 EquipmentManager가 보관
	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->EquipItem(this);
}

bool UPRItemInstance_Equipment::DeactivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 장비 슬롯의 해제 항목과 이미 장착 중인 장비 항목은 모두 이 ItemInstance 비활성화 요청으로 처리
	// UI가 별도 해제 함수를 직접 부르지 않아 장착과 해제가 같은 InventoryComponent 검증 경로를 사용
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// 해제 대상 슬롯은 목록이 열렸던 UI 상태가 아니라 장비 데이터의 SlotType으로 결정
	// 목록이 갱신되거나 닫힌 뒤에도 ItemInstance 데이터만으로 같은 슬롯을 다시 찾는 구조
	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->UnequipSlot(GetSlotType());
}

UPREquipmentDataAsset* UPRItemInstance_Equipment::GetEquipmentData() const
{
	return Cast<UPREquipmentDataAsset>(ItemData);
}

EPREquipmentSlotType UPRItemInstance_Equipment::GetSlotType() const
{
	const UPREquipmentDataAsset* Data = GetEquipmentData();
	return IsValid(Data) ? Data->GetSlotType() : EPREquipmentSlotType::None;
}

void UPRItemInstance_Equipment::OnEquipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (bIsEquippedEquipmentSlot)
	{
		return;
	}

	// 장비 슬롯 장착 상태 기록
	bIsEquippedEquipmentSlot = true;
	GrantEquippedEffects(OwnerActor);

	// EquipmentManager 장착 확정 이후의 장비별 후처리 지점
	// 캐릭터 파츠 교체와 장비 수치 적용 이후 추적 로그
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 장착: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));
}

void UPRItemInstance_Equipment::OnUnequipped(AActor* OwnerActor)
{
	// 장비 슬롯 장착 상태 해제
	bIsEquippedEquipmentSlot = false;

	if (!IsValid(OwnerActor))
	{
		return;
	}

	ClearEquippedEffects(OwnerActor);

	// EquipmentManager 해제 확정 이후의 장비별 후처리 지점
	// 캐릭터 파츠 복원과 장비 수치 회수 이후 추적 로그
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 해제: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));
}

void UPRItemInstance_Equipment::FillSaveEntry(FPREquipmentSlotSaveEntry& OutEntry) const
{
	OutEntry = FPREquipmentSlotSaveEntry();
	OutEntry.SlotType = GetSlotType();
	// ItemIndex는 EquipmentManager 혹은 InventoryComponent에서 채운다.
}

void UPRItemInstance_Equipment::ApplySaveEntry(const FPREquipmentSlotSaveEntry& InEntry)
{
	// 인스턴스의 내부 상태가 필요해지면 복원
}

void UPRItemInstance_Equipment::GrantEquippedEffects(AActor* OwnerActor)
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

	ApplyCommonStatEffect(OwnerActor);

	if (const UPREquipmentDataAsset* EquipmentData = GetEquipmentData())
	{
		EquipmentData->GiveAdditionalEffectsToAbilitySystem(ASC, EquipmentAbilityHandles, this);
	}
}

void UPRItemInstance_Equipment::ClearEquippedEffects(AActor* OwnerActor)
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

	RemoveCommonStatEffect(OwnerActor);
	ASC->ClearAbilitySetByHandles(EquipmentAbilityHandles);
}

void UPRItemInstance_Equipment::ApplyCommonStatEffect(AActor* OwnerActor)
{
	UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(OwnerActor));
	const UPREquipmentDataAsset* EquipmentData = GetEquipmentData();
	if (!IsValid(ASC) || !IsValid(EquipmentData) || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	RemoveCommonStatEffect(OwnerActor);

	if (!HasAnyCommonStatBonus(EquipmentData))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	TSubclassOf<UGameplayEffect> CommonStatGE;
	if (IsValid(Registry))
	{
		CommonStatGE = Registry->EquipGE_EquipmentCommonStats;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CommonStatGE, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRGameplayTags::SetByCaller_Equipment_MaxHealth, EquipmentData->GetMaxHealthBonus());
	SpecHandle.Data->SetSetByCallerMagnitude(PRGameplayTags::SetByCaller_Equipment_Armor, EquipmentData->GetArmorBonus());
	SpecHandle.Data->SetSetByCallerMagnitude(PRGameplayTags::SetByCaller_Equipment_PlayerAttackPower, EquipmentData->GetAttackPowerBonus());
	SpecHandle.Data->SetSetByCallerMagnitude(PRGameplayTags::SetByCaller_Equipment_MaxStamina, EquipmentData->GetMaxStaminaBonus());

	CommonStatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UPRItemInstance_Equipment::RemoveCommonStatEffect(AActor* OwnerActor)
{
	if (!CommonStatEffectHandle.IsValid())
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(OwnerActor));
	if (IsValid(ASC) && ASC->IsOwnerActorAuthoritative())
	{
		ASC->RemoveActiveGameplayEffect(CommonStatEffectHandle);
	}

	CommonStatEffectHandle.Invalidate();
}
