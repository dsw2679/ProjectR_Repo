// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 Material Cost UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopMaterialCostWidget.generated.h"

class UImage;
class UTextBlock;

// 상점 구매에 필요한 재료 비용 행 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopMaterialCostWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 재료 비용 표시 데이터 지정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetMaterialCostViewData(const FPRShopMaterialCostViewData& InViewData);

	// 현재 재료 비용 표시 데이터 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	FPRShopMaterialCostViewData GetMaterialCostViewData() const { return ViewData; }

protected:
	/*~ UUserWidget Interface ~*/
	// 현재 표시 데이터의 디자인 타임 반영
	virtual void NativePreConstruct() override;

private:
	// 현재 표시 데이터의 네이티브 위젯 반영
	void RefreshNativeDisplay();

protected:
	// UMG에서 바인딩할 재료 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> MaterialIconImage;

	// UMG에서 바인딩할 재료 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> MaterialNameText;

	// UMG에서 바인딩할 필요 수량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> MaterialQuantityText;

	// 필요 수량을 충족했을 때 수량 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	FLinearColor EnoughQuantityColor = FLinearColor::White;

	// 필요 수량이 부족할 때 수량 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	FLinearColor NotEnoughQuantityColor = FLinearColor::Red;

private:
	// 현재 재료 비용 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	FPRShopMaterialCostViewData ViewData;
};
