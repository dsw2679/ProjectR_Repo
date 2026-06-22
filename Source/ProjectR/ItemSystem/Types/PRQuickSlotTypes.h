// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (퀵슬롯 슬롯 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRQuickSlotTypes.generated.h"

class UPRConsumableDataAsset;
class UPRItemInstance_Consumable;
class UTexture2D;

// 퀵슬롯 한 칸의 등록 정본과 런타임 캐시다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRQuickSlotEntry
{
	GENERATED_BODY()

public:
	// 퀵슬롯에 등록된 소비 아이템 종류다
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRConsumableDataAsset> ConsumableData = nullptr;

	// 현재 인벤토리에 있는 소비 아이템 인스턴스 캐시다
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemInstance_Consumable> CachedConsumableItem = nullptr;
};

// 퀵슬롯 HUD와 인벤토리 퀵슬롯 영역이 사용하는 표시 데이터다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRQuickSlotViewData
{
	GENERATED_BODY()

public:
	// 0부터 시작하는 슬롯 인덱스다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	int32 SlotIndex = INDEX_NONE;

	// 등록된 소비 아이템 종류다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRConsumableDataAsset> ConsumableData = nullptr;

	// 현재 인벤토리에 있는 소비 아이템 인스턴스다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemInstance_Consumable> ConsumableItem = nullptr;

	// 화면에 표시할 아이콘이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 현재 보유 개수다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	int32 StackCount = 0;

	// 퀵슬롯에 아이템 종류가 등록되어 있는지 여부다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	bool bHasRegisteredItem = false;

	// 현재 사용 가능한 수량이 있는지 여부다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	bool bUsable = false;
};
