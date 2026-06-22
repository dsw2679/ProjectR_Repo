// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (방어구 파츠 스켈레탈 메시 및 능력치 데이터 정의)
// Author: 손승우 (보스전용 특수 장착 에셋 데이터 연동)
#include "PREquipmentDataAsset.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"

UPREquipmentDataAsset::UPREquipmentDataAsset()
{
	ItemType = EPRItemType::Equipment;
	ItemInstanceClass = UPRItemInstance_Equipment::StaticClass();
	MaxStackCount = 1; // 장비는 겹쳐지지 않음
}

void UPREquipmentDataAsset::GiveAdditionalEffectsToAbilitySystem(
	UAbilitySystemComponent* TargetASC,
	FPRAbilitySetHandles& OutHandles,
	UObject* InSourceObject) const
{
	if (!IsValid(TargetASC))
	{
		return;
	}

	for (const FPRAbilityEntry& Entry : EquippedAbilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject);
	}

	for (const FPREffectEntry& Entry : EquippedEffects)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject);
	}
}
