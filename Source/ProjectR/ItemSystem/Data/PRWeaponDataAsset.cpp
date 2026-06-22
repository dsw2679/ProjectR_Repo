// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 반동 애니메이션 및 조준 카메라 데이터 정의)
// Author: 배유찬 (무기 기본 사격/장전 속도 및 탄약 스탯 데이터 정의)
// Author: 이건주 (Mod 장착 슬롯 및 인벤토리 연동 무기 능력치 정의)
#include "PRWeaponDataAsset.h"

#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Fire.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/UI/Crosshair/PRCrosshairConfig.h"

UPRWeaponDataAsset::UPRWeaponDataAsset()
{
	ItemType = EPRItemType::Weapon;
	ItemInstanceClass = UPRItemInstance_Weapon::StaticClass();
}

const UPRCrosshairConfig* UPRWeaponDataAsset::GetCrosshairConfig() const
{
	if (IsValid(CrosshairConfig))
	{
		return CrosshairConfig;
	}

	const UPRDeveloperSettings* DeveloperSettings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(DeveloperSettings))
	{
		return nullptr;
	}

	// 무기 전용 설정이 비어 있는 경우 프로젝트 기본 크로스헤어 설정 사용
	return DeveloperSettings->GetDefaultCrosshairConfigSync();
}

void UPRWeaponDataAsset::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,
	FPRAbilitySetHandles& OutHandles,
	UObject* InSourceObject,
	const FGameplayTagContainer* AdditionalDynamicTags,
	const FGameplayTagContainer* BaseFireAdditionalDynamicTags)
{
	for (const FPRAbilityEntry& Entry :EquippedAbilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		FGameplayTagContainer EntryAdditionalDynamicTags;
		if (AdditionalDynamicTags != nullptr)
		{
			// 슬롯 차단 태그
			EntryAdditionalDynamicTags.AppendTags(*AdditionalDynamicTags);
		}

		if (BaseFireAdditionalDynamicTags != nullptr
			&& Entry.AbilityClass->IsChildOf(UPRGA_Fire::StaticClass()))
		{
			// 기본 사격 모드 차단 태그
			EntryAdditionalDynamicTags.AppendTags(*BaseFireAdditionalDynamicTags);
		}

		Entry.GiveToAbilitySystem(
			TargetASC,
			OutHandles,
			InSourceObject,
			EntryAdditionalDynamicTags.IsEmpty() ? nullptr : &EntryAdditionalDynamicTags);
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
