// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (장비 상세 속성 빌더 구현)
// Author: 배유찬 (아이템 능력치 수치 뷰 데이터 생성 구현)
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "PRItemTooltipViewDataBuilder.generated.h"

// 아이템 툴팁 표시 데이터 생성 빌더
UCLASS()
class PROJECTR_API UPRItemTooltipViewDataBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 슬롯 표시 데이터를 아이템 툴팁 표시 데이터로 변환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|Tooltip")
	static FPRItemTooltipViewData BuildTooltipViewData(const FPRInventoryItemSlotViewData& SlotViewData);
};
