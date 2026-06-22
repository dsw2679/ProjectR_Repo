// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Mod 충전 게이지 요구량 및 액티브 어빌리티 데이터 정의)
// Author: 이건주 (드론 및 배리어 Mod 전용 비주얼/스탯 데이터 정의)
#include "PRWeaponModDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"

UPRWeaponModDataAsset::UPRWeaponModDataAsset()
{
	ItemType =EPRItemType::Mod;
	ItemInstanceClass = UPRItemInstance_Mod::StaticClass();
}

void UPRWeaponModDataAsset::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,
	FPRAbilitySetHandles& OutHandles,
	UObject* InSourceObject,
	const FGameplayTagContainer* AdditionalDynamicTags)
{
	for (const FPRAbilityEntry& Entry :EquippedAbilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject, AdditionalDynamicTags);
	}

	for (const FPREffectEntry& Entry :EquippedEffects)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		
		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject);
	}
}
