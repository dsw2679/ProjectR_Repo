// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 인벤토리 Item 슬롯 View 데이터 Builder 구현)
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "ProjectR/ItemSystem/Types/PRQuickSlotTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "PRInventoryItemSlotViewDataBuilder.generated.h"

class UPRItemInstance;
class UPRItemInstance_Consumable;
class UPRItemInstance_Equipment;
class UPRItemInstance_Material;
class UPRItemInstance_Mod;
class UPRItemInstance_Weapon;
class UPREquipmentDataAsset;
class UPRWeaponDataAsset;

// 인벤토리와 시작 메뉴가 공유하는 슬롯 표시 데이터 생성기
UCLASS()
class PROJECTR_API UPRInventoryItemSlotViewDataBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 빈 무기 슬롯 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildEmptyWeaponSlotViewData(EPRWeaponSlotType SlotType);

	// 무기 아이템 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped);

	// 무기 데이터 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildWeaponDataViewData(UPRWeaponDataAsset* WeaponData, bool bSelected);

	// Mod 아이템 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped);

	// 소비 아이템 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildConsumableItemViewData(UPRItemInstance_Consumable* ConsumableItem);

	// 재료 목록 진입 슬롯 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildMaterialSlotViewData();

	// 재료 아이템 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildMaterialItemViewData(UPRItemInstance_Material* MaterialItem);

	// 퀵슬롯 표시 데이터를 생성함
	static FPRInventoryItemSlotViewData BuildQuickSlotViewData(const FPRQuickSlotViewData& QuickSlotViewData);

	// 빈 퀵슬롯 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildEmptyQuickSlotViewData(int32 SlotIndex);

	// 빈 장비 슬롯 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildEmptyEquipmentSlotViewData(EPREquipmentSlotType SlotType);

	// 장비 아이템 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildEquipmentItemViewData(UPRItemInstance_Equipment* EquipmentItem, bool bEquipped);

	// 장비 데이터 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildEquipmentDataViewData(UPREquipmentDataAsset* EquipmentData, EPREquipmentSlotType SlotType, bool bSelected);

	// 비활성화 명령 표시 데이터를 생성함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FPRInventoryItemSlotViewData BuildDeactivateActionViewData(EPRItemType ListType, UPRItemInstance* TargetItem);

	// 장비 슬롯 표시 이름을 반환함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static FText GetEquipmentSlotDisplayName(EPREquipmentSlotType SlotType);

	// 무기와 Mod의 장착 호환 여부를 반환함
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|ViewData")
	static bool IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem);
};
