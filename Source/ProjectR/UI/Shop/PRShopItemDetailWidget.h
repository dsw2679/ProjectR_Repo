// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 Item Detail UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopItemDetailWidget.generated.h"

class UImage;
class UPanelWidget;
class UPRShopMaterialCostWidget;
class UTextBlock;

// 상점에서 선택된 아이템의 상세 정보를 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopItemDetailWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 선택 아이템 상세 표시 데이터를 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetSelectedItemDetails(const FPRShopItemSlotViewData& InViewData, EPRShopTabType InCurrentTabType, int32 InOwnedStackCount);

protected:
	/*~ UUserWidget Interface ~*/
	// 초기화 시 현재 상세 표시 데이터를 반영
	virtual void NativeOnInitialized() override;

private:
	// 현재 상세 표시 데이터를 네이티브 바인딩 위젯에 반영
	void RefreshNativeDisplay();

	// 재료 비용 목록 위젯 갱신
	void RefreshMaterialCostWidgets();

	// 생성된 재료 비용 위젯 정리
	void ClearMaterialCostWidgets();

	// 재료 비용 행 위젯 생성 및 추가
	void AddMaterialCostWidget(const FPRShopMaterialCostViewData& MaterialCostViewData);

	// 재고 표시 텍스트를 만든다
	FText MakeStockText(bool bHasSelectedItem) const;

protected:
	// UMG에서 바인딩할 선택 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemNameText;

	// UMG에서 바인딩할 선택 아이템 설명 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemDescriptionText;

	// UMG에서 바인딩할 선택 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> SelectedItemIconImage;

	// UMG에서 바인딩할 선택 아이템 가격 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemPriceText;

	// 고철 가격을 충족했을 때 가격 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	FLinearColor EnoughPriceColor = FLinearColor::White;

	// 고철 가격이 부족할 때 가격 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	FLinearColor NotEnoughPriceColor = FLinearColor::Red;

	// UMG에서 바인딩할 선택 아이템 재고 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemStockText;

	// UMG에서 바인딩할 선택 아이템 보유 수량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemOwnedCountText;

	// UMG에서 바인딩할 선택 아이템 재료 비용 목록 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UPanelWidget> SelectedItemMaterialCostPanel;

	// 재료 비용 목록에 동적으로 생성할 행 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	TSubclassOf<UPRShopMaterialCostWidget> MaterialCostWidgetClass;

private:
	// 현재 선택된 상점 아이템 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	FPRShopItemSlotViewData ViewData;

	// 현재 상점 탭
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	EPRShopTabType CurrentTabType = EPRShopTabType::Buy;

	// 현재 플레이어가 보유한 선택 아이템 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	int32 OwnedStackCount = 0;

	// 동적으로 생성한 재료 비용 행 위젯 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRShopMaterialCostWidget>> MaterialCostWidgets;
};
