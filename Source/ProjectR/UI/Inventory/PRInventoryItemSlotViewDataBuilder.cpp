// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 인벤토리 Item 슬롯 View 데이터 Builder 구현)
#include "PRInventoryItemSlotViewDataBuilder.h"

#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/ItemSystem/Types/PRQuickSlotTypes.h"

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildEmptyWeaponSlotViewData(EPRWeaponSlotType SlotType)
{
	// 빈 주무기와 보조무기 슬롯 기본 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.DisplayName = SlotType == EPRWeaponSlotType::Primary
		? FText::FromString(TEXT("주무기 없음"))
		: FText::FromString(TEXT("보조무기 없음"));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped)
{
	// 무기 ItemInstance 기반 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(WeaponItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 무기 아이템 없음"));
		return ViewData;
	}

	UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	ViewData.ItemData = WeaponData;
	ViewData.ItemInstance = WeaponItem;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

	// 무기 정적 데이터 기반 이름과 아이콘
	if (IsValid(WeaponData))
	{
		ViewData.DisplayName = WeaponData->GetDisplayName();
		ViewData.Icon = WeaponData->GetIcon();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 무기 아이템 데이터 없음"));
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildWeaponDataViewData(UPRWeaponDataAsset* WeaponData, bool bSelected)
{
	// 저장 데이터 기반 무기 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemData = WeaponData;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.bSelected = bSelected;

	// 무기 정적 데이터 기반 이름과 아이콘
	if (IsValid(WeaponData))
	{
		ViewData.DisplayName = WeaponData->GetDisplayName();
		ViewData.Icon = WeaponData->GetIcon();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 무기 데이터 없음"));
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped)
{
	// Mod ItemInstance 기반 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(ModItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] Mod 아이템 없음"));
		return ViewData;
	}

	UPRWeaponModDataAsset* ModData = ModItem->GetModData();
	ViewData.ItemData = ModData;
	ViewData.ItemInstance = ModItem;
	ViewData.ItemType = EPRItemType::Mod;
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

	// Mod 정적 데이터 기반 이름과 아이콘
	if (IsValid(ModData))
	{
		ViewData.DisplayName = ModData->GetDisplayName();
		ViewData.Icon = ModData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildConsumableItemViewData(UPRItemInstance_Consumable* ConsumableItem)
{
	// 소비 ItemInstance 기반 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(ConsumableItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 소비 아이템 없음"));
		return ViewData;
	}

	UPRConsumableDataAsset* ConsumableData = ConsumableItem->GetConsumableData();
	ViewData.ItemData = ConsumableData;
	ViewData.ItemInstance = ConsumableItem;
	ViewData.ItemType = EPRItemType::Consumable;
	ViewData.InventoryAction = EPRInventoryAction::Activate;
	ViewData.StackCount = ConsumableItem->GetStackCount();
	ViewData.bShowStackCount = true;
	ViewData.OwnedStackCount = ConsumableItem->GetStackCount();
	ViewData.bHasOwnedStackCount = true;

	// 소비 아이템 정적 데이터 기반 이름과 아이콘
	if (IsValid(ConsumableData))
	{
		ViewData.DisplayName = ConsumableData->GetDisplayName();
		ViewData.Icon = ConsumableData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildMaterialSlotViewData()
{
	// 재료 목록을 여는 고정 진입 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Material;
	ViewData.DisplayName = FText::FromString(TEXT("재료"));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildMaterialItemViewData(UPRItemInstance_Material* MaterialItem)
{
	// 재료 ItemInstance 기반 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(MaterialItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 재료 아이템 없음"));
		return ViewData;
	}

	UPRMaterialDataAsset* MaterialData = MaterialItem->GetMaterialData();
	ViewData.ItemData = MaterialData;
	ViewData.ItemInstance = MaterialItem;
	ViewData.ItemType = EPRItemType::Material;
	ViewData.StackCount = MaterialItem->GetStackCount();
	ViewData.bShowStackCount = true;
	ViewData.OwnedStackCount = MaterialItem->GetStackCount();
	ViewData.bHasOwnedStackCount = true;

	// 재료 정적 데이터 기반 이름과 아이콘
	if (IsValid(MaterialData))
	{
		ViewData.DisplayName = MaterialData->GetDisplayName();
		ViewData.Icon = MaterialData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildQuickSlotViewData(const FPRQuickSlotViewData& QuickSlotViewData)
{
	// 퀵슬롯 컴포넌트가 계산한 표시 스냅샷 변환값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Consumable;
	ViewData.ContextIndex = QuickSlotViewData.SlotIndex;
	ViewData.ItemData = QuickSlotViewData.ConsumableData;
	ViewData.ItemInstance = QuickSlotViewData.ConsumableItem;
	ViewData.Icon = QuickSlotViewData.Icon;
	ViewData.StackCount = QuickSlotViewData.StackCount;
	ViewData.bShowStackCount = QuickSlotViewData.bHasRegisteredItem;
	ViewData.OwnedStackCount = QuickSlotViewData.StackCount;
	ViewData.bHasOwnedStackCount = QuickSlotViewData.bHasRegisteredItem;

	// 등록된 소비 아이템이 없을 때 빈 퀵슬롯 표시값
	if (IsValid(QuickSlotViewData.ConsumableData))
	{
		ViewData.DisplayName = QuickSlotViewData.ConsumableData->GetDisplayName();
	}
	else
	{
		ViewData.DisplayName = FText::FromString(TEXT("(비어 있음)"));
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildEmptyQuickSlotViewData(int32 SlotIndex)
{
	// 퀵슬롯 컴포넌트 미연결 상태의 빈 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Consumable;
	ViewData.ContextIndex = SlotIndex;
	ViewData.DisplayName = FText::FromString(TEXT("(비어 있음)"));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildEmptyEquipmentSlotViewData(EPREquipmentSlotType SlotType)
{
	// 장비 슬롯은 비어 있어도 슬롯 타입 컨텍스트 유지
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Equipment;
	ViewData.ContextIndex = static_cast<int32>(SlotType);
	ViewData.DisplayName = FText::Format(FText::FromString(TEXT("{0} 없음")), GetEquipmentSlotDisplayName(SlotType));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildEquipmentItemViewData(UPRItemInstance_Equipment* EquipmentItem, bool bEquipped)
{
	// 장비 ItemInstance 기반 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(EquipmentItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemSlotViewDataBuilder] 장비 아이템 없음"));
		return ViewData;
	}

	UPREquipmentDataAsset* EquipmentData = EquipmentItem->GetEquipmentData();
	const EPREquipmentSlotType SlotType = EquipmentItem->GetSlotType();
	ViewData.ItemData = EquipmentData;
	ViewData.ItemInstance = EquipmentItem;
	ViewData.ItemType = EPRItemType::Equipment;
	ViewData.ContextIndex = static_cast<int32>(SlotType);
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

	// 장비 정적 데이터 기반 이름과 아이콘
	if (IsValid(EquipmentData))
	{
		ViewData.DisplayName = EquipmentData->GetDisplayName();
		ViewData.Icon = EquipmentData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildEquipmentDataViewData(UPREquipmentDataAsset* EquipmentData, EPREquipmentSlotType SlotType, bool bSelected)
{
	// 저장 데이터 기반 장비 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemData = EquipmentData;
	ViewData.ItemType = EPRItemType::Equipment;
	ViewData.ContextIndex = static_cast<int32>(SlotType);
	ViewData.bSelected = bSelected;

	// 장비 정적 데이터 기반 이름과 아이콘
	if (IsValid(EquipmentData))
	{
		ViewData.DisplayName = EquipmentData->GetDisplayName();
		ViewData.Icon = EquipmentData->GetIcon();
	}
	else
	{
		ViewData.DisplayName = FText::Format(FText::FromString(TEXT("{0} 없음")), GetEquipmentSlotDisplayName(SlotType));
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryItemSlotViewDataBuilder::BuildDeactivateActionViewData(EPRItemType ListType, UPRItemInstance* TargetItem)
{
	// 실제 해제 대상 ItemInstance를 보존하는 명령 슬롯 표시값
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemInstance = TargetItem;
	ViewData.InventoryAction = EPRInventoryAction::Deactivate;

	// 목록 타입별 해제 명령 문구
	if (ListType == EPRItemType::Weapon
		|| ListType == EPRItemType::PrimaryWeapon
		|| ListType == EPRItemType::SecondaryWeapon)
	{
		ViewData.ItemType = EPRItemType::Weapon;
		ViewData.DisplayName = FText::FromString(TEXT("무기 해제"));
	}
	else if (ListType == EPRItemType::Mod)
	{
		ViewData.ItemType = EPRItemType::Mod;
		ViewData.DisplayName = FText::FromString(TEXT("Mod 해제"));
	}
	else if (ListType == EPRItemType::Equipment)
	{
		ViewData.ItemType = EPRItemType::Equipment;
		ViewData.DisplayName = FText::FromString(TEXT("장비 해제"));

		// 장비 해제 후 목록 갱신에 필요한 슬롯 타입 컨텍스트
		if (const UPRItemInstance_Equipment* EquipmentItem = Cast<UPRItemInstance_Equipment>(TargetItem))
		{
			ViewData.ContextIndex = static_cast<int32>(EquipmentItem->GetSlotType());
		}
	}

	return ViewData;
}

FText UPRInventoryItemSlotViewDataBuilder::GetEquipmentSlotDisplayName(EPREquipmentSlotType SlotType)
{
	// 장비 슬롯 enum에 대응하는 사용자 표시 이름
	switch (SlotType)
	{
	case EPREquipmentSlotType::Head:
		return FText::FromString(TEXT("머리"));

	case EPREquipmentSlotType::Body:
		return FText::FromString(TEXT("몸통"));

	case EPREquipmentSlotType::Hands:
		return FText::FromString(TEXT("손"));

	case EPREquipmentSlotType::Legs:
		return FText::FromString(TEXT("다리"));

	case EPREquipmentSlotType::Amulet:
		return FText::FromString(TEXT("목걸이"));

	case EPREquipmentSlotType::Ring1:
		return FText::FromString(TEXT("반지 1"));

	case EPREquipmentSlotType::Ring2:
		return FText::FromString(TEXT("반지 2"));

	case EPREquipmentSlotType::None:
	default:
		return FText::FromString(TEXT("장비"));
	}
}

bool UPRInventoryItemSlotViewDataBuilder::IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem)
{
	// 입력 ItemInstance 검증
	if (!IsValid(ModItem) || !IsValid(WeaponItem))
	{
		return false;
	}

	// 호환성 판정에 필요한 정적 데이터 검증
	const UPRWeaponModDataAsset* ModData = ModItem->GetModData();
	const UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	if (!IsValid(ModData) || !IsValid(WeaponData))
	{
		return false;
	}

	if (ModData->ModTags.IsEmpty())
	{
		return true;
	}

	// 태그 기반 Mod 호환성 판정
	if (WeaponData->SupportedModTags.IsEmpty())
	{
		return false;
	}

	return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
}
