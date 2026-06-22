// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 상세 툴팁 뷰 데이터 정의)
// Author: 배유찬 (장비 슬롯 및 아이템 목록 뷰 데이터 정의)
// Author: 이건주 (캐릭터 3D 프리뷰 연동 뷰 데이터 정의)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "PRInventoryUITypes.generated.h"

class UPRItemDataAsset;
class UPRItemInstance;
class UTexture2D;

// 인벤토리 슬롯 선택 실행 동작
UENUM(BlueprintType)
enum class EPRInventoryAction : uint8
{
	// 실행 동작 없음
	None,

	// 활성화 요청
	Activate,

	// 비활성화 요청
	Deactivate
};

// 인벤토리 UI에서 아이템 한 칸을 표시하기 위한 데이터다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRInventoryItemSlotViewData
{
	GENERATED_BODY()

public:
	// 표시할 아이템 공통 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;

	// 선택 시 장착 요청에 사용할 아이템 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemInstance> ItemInstance = nullptr;

	// UI에 표시할 아이템 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	FText DisplayName;

	// UI에 표시할 아이템 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 아이템 분류
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	EPRItemType ItemType = EPRItemType::None;

	// 선택 시 실행할 인벤토리 동작
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	EPRInventoryAction InventoryAction = EPRInventoryAction::None;

	// 슬롯 클릭 후 목록 처리에 필요한 숫자 컨텍스트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 ContextIndex = INDEX_NONE;

	// 도메인별 선택 표시 상태
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bSelected = false;

	// 슬롯에 표시할 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 StackCount = 0;

	// 수량 텍스트 표시 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bShowStackCount = false;

	// 실제 인벤토리 보유 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 OwnedStackCount = 0;

	// 실제 인벤토리 보유 수량 유효 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bHasOwnedStackCount = false;
};

// 아이템 툴팁 상세 정보 한 줄 표시 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRItemTooltipDetailLineViewData
{
	GENERATED_BODY()

public:
	// 상세 항목 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	FText Label;

	// 상세 항목 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	FText Value;
};

// 아이템 툴팁 표시 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRItemTooltipViewData
{
	GENERATED_BODY()

public:
	// UI에 표시할 아이템 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	FText DisplayName;

	// UI에 표시할 아이템 타입
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	FText ItemTypeText;

	// UI에 표시할 아이템 설명
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	FText Description;

	// UI에 표시할 상세 항목 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	TArray<FPRItemTooltipDetailLineViewData> DetailLines;

	// 표시할 실제 아이템 정보 보유 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory|Tooltip")
	bool bHasItem = false;
};
