// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (장비 상세 속성 빌더 구현)
// Author: 배유찬 (아이템 능력치 수치 뷰 데이터 생성 구현)
#include "PRItemTooltipViewDataBuilder.h"

#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/PRGameplayEffectUIDescriptionComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/ItemSystem/PRWeaponStatStatics.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"

namespace
{
	FText GetItemTypeText(EPRItemType ItemType)
	{
		switch (ItemType)
		{
		case EPRItemType::Weapon:
		case EPRItemType::PrimaryWeapon:
		case EPRItemType::SecondaryWeapon:
			return FText::FromString(TEXT("무기"));

		case EPRItemType::Mod:
			return FText::FromString(TEXT("모드"));

		case EPRItemType::Consumable:
			return FText::FromString(TEXT("소비"));

		case EPRItemType::Material:
			return FText::FromString(TEXT("재료"));

		case EPRItemType::Equipment:
			return FText::FromString(TEXT("장비"));

		case EPRItemType::None:
		default:
			return FText::GetEmpty();
		}
	}

	void AddDetailLine(FPRItemTooltipViewData& ViewData, const FText& Label, const FText& Value)
	{
		FPRItemTooltipDetailLineViewData DetailLine;
		DetailLine.Label = Label;
		DetailLine.Value = Value;
		ViewData.DetailLines.Add(DetailLine);
	}

	FText BuildSignedFloatText(float Value, int32 MaximumFractionalDigits = 0)
	{
		const FText ValueText = UPRUserInterfaceStatics::ConvertFloatToText(FMath::Abs(Value), MaximumFractionalDigits);
		const TCHAR* SignFormat = Value >= 0.0f ? TEXT("+{0}") : TEXT("-{0}");
		return FText::Format(FText::FromString(SignFormat), ValueText);
	}

	void AddEquipmentStatLine(FPRItemTooltipViewData& ViewData, const FText& Label, float Value, int32 MaximumFractionalDigits = 0)
	{
		if (FMath::IsNearlyZero(Value))
		{
			return;
		}

		AddDetailLine(ViewData, Label, BuildSignedFloatText(Value, MaximumFractionalDigits));
	}

	void AppendGameplayEffectDescriptionLines(FPRItemTooltipViewData& ViewData, const TArray<FPREffectEntry>& EffectEntries)
	{
		for (const FPREffectEntry& EffectEntry : EffectEntries)
		{
			if (!IsValid(EffectEntry.EffectClass))
			{
				continue;
			}

			const UGameplayEffect* EffectCDO = EffectEntry.EffectClass->GetDefaultObject<UGameplayEffect>();
			if (!IsValid(EffectCDO))
			{
				continue;
			}

			const UPRGameplayEffectUIDescriptionComponent* DescriptionComponent =
				EffectCDO->FindComponent<UPRGameplayEffectUIDescriptionComponent>();
			if (!IsValid(DescriptionComponent))
			{
				continue;
			}

			for (const FPRGameplayEffectUIDescriptionLine& DescriptionLine : DescriptionComponent->GetDescriptionLines())
			{
				AddDetailLine(ViewData, DescriptionLine.Label, DescriptionLine.Value);
			}
		}
	}

	FText BuildWeaponDisplayName(const UPRWeaponDataAsset* WeaponData, const UPRItemInstance_Weapon* WeaponItem)
	{
		if (!IsValid(WeaponData))
		{
			return FText::GetEmpty();
		}

		if (!IsValid(WeaponItem))
		{
			return WeaponData->GetDisplayName();
		}

		return FText::Format(
			FText::FromString(TEXT("{0}(+{1})")),
			WeaponData->GetDisplayName(),
			FText::AsNumber(WeaponItem->GetUpgradeLevel()));
	}

	bool TryResolveOwnedStackCount(const FPRInventoryItemSlotViewData& SlotViewData, int32& OutOwnedStackCount)
	{
		if (IsValid(SlotViewData.ItemInstance))
		{
			OutOwnedStackCount = SlotViewData.ItemInstance->GetStackCount();
			return true;
		}

		if (SlotViewData.bHasOwnedStackCount)
		{
			OutOwnedStackCount = SlotViewData.OwnedStackCount;
			return true;
		}

		OutOwnedStackCount = 0;
		return false;
	}
}

FPRItemTooltipViewData UPRItemTooltipViewDataBuilder::BuildTooltipViewData(const FPRInventoryItemSlotViewData& SlotViewData)
{
	FPRItemTooltipViewData ViewData;

	UPRItemDataAsset* ItemData = SlotViewData.ItemData.Get();
	if (!IsValid(ItemData))
	{
		return ViewData;
	}

	ViewData.bHasItem = true;
	ViewData.DisplayName = ItemData->GetDisplayName();
	ViewData.ItemTypeText = GetItemTypeText(ItemData->GetItemType());
	ViewData.Description = ItemData->GetDescription();

	if (const UPRWeaponDataAsset* WeaponData = Cast<UPRWeaponDataAsset>(ItemData))
	{
		const UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(SlotViewData.ItemInstance);
		ViewData.DisplayName = BuildWeaponDisplayName(WeaponData, WeaponItem);

		const float BaseDamage = IsValid(WeaponItem)
			? UPRWeaponStatStatics::CalculateWeaponItemBaseDamage(WeaponItem)
			: UPRWeaponStatStatics::CalculateUpgradedBaseDamage(WeaponData, 0);
		AddDetailLine(ViewData, FText::FromString(TEXT("공격력")), UPRUserInterfaceStatics::ConvertFloatToText(BaseDamage));
		AddDetailLine(ViewData, FText::FromString(TEXT("탄창")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->MaxMagazineAmmo));
		AddDetailLine(ViewData, FText::FromString(TEXT("예비탄 배율")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->ReserveAmmoMultiplier, 1));
		AddDetailLine(
			ViewData,
			FText::FromString(TEXT("약점 배율")),
			FText::Format(FText::FromString(TEXT("{0}%")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->AdditiveWeakpointMultiplier * 100.0f)));
		return ViewData;
	}

	if (const UPREquipmentDataAsset* EquipmentData = Cast<UPREquipmentDataAsset>(ItemData))
	{
		AddDetailLine(
			ViewData,
			FText::FromString(TEXT("장비 슬롯")),
			UPRInventoryItemSlotViewDataBuilder::GetEquipmentSlotDisplayName(EquipmentData->GetSlotType()));
		AddEquipmentStatLine(ViewData, FText::FromString(TEXT("최대 체력")), EquipmentData->GetMaxHealthBonus());
		AddEquipmentStatLine(ViewData, FText::FromString(TEXT("방어력")), EquipmentData->GetArmorBonus());
		AddEquipmentStatLine(ViewData, FText::FromString(TEXT("공격력")), EquipmentData->GetAttackPowerBonus());
		AddEquipmentStatLine(ViewData, FText::FromString(TEXT("최대 스태미너")), EquipmentData->GetMaxStaminaBonus());
		AppendGameplayEffectDescriptionLines(ViewData, EquipmentData->GetEquippedEffects());
		return ViewData;
	}

	if (Cast<UPRConsumableDataAsset>(ItemData) || Cast<UPRMaterialDataAsset>(ItemData))
	{
		int32 OwnedStackCount = 0;
		if (TryResolveOwnedStackCount(SlotViewData, OwnedStackCount))
		{
			AddDetailLine(ViewData, FText::FromString(TEXT("보유 수량")), FText::AsNumber(OwnedStackCount));
		}

		AddDetailLine(ViewData, FText::FromString(TEXT("최대 스택")), FText::AsNumber(ItemData->MaxStackCount));
	}

	return ViewData;
}
