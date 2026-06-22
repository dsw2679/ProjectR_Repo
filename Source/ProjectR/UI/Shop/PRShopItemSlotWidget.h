// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (상점 Item 슬롯 UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopItemSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopItemSlotClickedSignature, const FPRShopItemSlotViewData&, ViewData);

// 상점 아이템 한 칸의 이름, 아이콘, 가격, 재고, 클릭 이벤트를 담당하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopItemSlotWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯 표시 데이터를 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetSlotViewData(const FPRShopItemSlotViewData& InViewData);

	// 현재 슬롯 표시 데이터를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	FPRShopItemSlotViewData GetSlotViewData() const { return ViewData; }

protected:
	/*~ UUserWidget Interface ~*/
	// 초기화 시 버튼 이벤트를 바인딩
	virtual void NativeOnInitialized() override;

	// 마우스 버튼 입력으로 슬롯 클릭을 처리
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// 클릭 버튼 좌클릭 이벤트를 슬롯 선택으로 전달
	UFUNCTION()
	void HandleClickButtonClicked();

	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영
	void RefreshNativeDisplay();

	// 재고 표시 텍스트를 만듬
	FText MakeStockText() const;

public:
	// 슬롯 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopItemSlotClickedSignature OnClicked;

protected:
	// UMG에서 바인딩할 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> ItemNameText;

	// UMG에서 바인딩할 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> ItemIconImage;

	// UMG에서 바인딩할 가격 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> PriceText;

	// UMG에서 바인딩할 재고 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> StockText;

	// UMG에서 바인딩할 플레이어 보유 수량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> OwnedCountText;

	// UMG에서 바인딩할 선택 표시 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> SelectedIndicatorImage;

	// UMG에서 바인딩할 클릭 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> ClickButton;

private:
	// 현재 슬롯 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	FPRShopItemSlotViewData ViewData;

};
