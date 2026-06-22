// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 등급별 툴팁 레이아웃 구현)
// Author: 배유찬 (아이템 상세 능력치 및 장비 비교 정보 텍스트 구현)
// Author: 이건주 (무기 Mod 슬롯 설명 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;

// 아이템 슬롯에 마우스를 올렸을 때 표시할 상세 정보 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRItemTooltipWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 툴팁 표시 데이터 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Tooltip")
	void SetTooltipViewData(const FPRItemTooltipViewData& InViewData);

	// 현재 툴팁 표시 데이터 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|Tooltip")
	FPRItemTooltipViewData GetTooltipViewData() const;

private:
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영
	void RefreshNativeDisplay();

	// 상세 항목 표시 텍스트 생성
	FText MakeDetailText() const;

	// 상세 항목 이름 목록 텍스트 생성
	FText MakeDetailLabelText() const;

	// 상세 항목 값 목록 텍스트 생성
	FText MakeDetailValueText() const;

protected:
	// UMG에서 바인딩할 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemNameText;

	// UMG에서 바인딩할 아이템 타입 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemTypeText;

	// UMG에서 바인딩할 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UImage> ItemIconImage;

	// UMG에서 바인딩할 상세 설명 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> DescriptionText;

	// UMG에서 바인딩할 상세 항목 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> DetailText;

	// UMG에서 바인딩할 상세 항목 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> DetailLabelText;

	// UMG에서 바인딩할 상세 항목 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> DetailValueText;

private:
	// 현재 툴팁 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	FPRItemTooltipViewData ViewData;
};
