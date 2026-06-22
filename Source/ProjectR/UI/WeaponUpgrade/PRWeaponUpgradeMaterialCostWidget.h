// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Weapon 강화/업그레이드 Material Cost UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/ItemSystem/Types/PRWeaponUpgradeTypes.h"
#include "PRWeaponUpgradeMaterialCostWidget.generated.h"

class UImage;
class UTextBlock;

// 무기 강화에 필요한 재료 아이콘과 수량을 표시하는 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWeaponUpgradeMaterialCostWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 재료 비용 표시 데이터를 지정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void SetMaterialCostViewData(const FPRWeaponUpgradeMaterialCostViewData& InViewData);

	// 현재 재료 비용 표시 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradeMaterialCostViewData GetMaterialCostViewData() const { return ViewData; }

protected:
	/*~ UUserWidget Interface ~*/
	// 에디터와 런타임에서 현재 표시 데이터를 네이티브 위젯에 반영한다
	virtual void NativePreConstruct() override;

private:
	// 현재 표시 데이터를 아이콘과 수량 텍스트에 반영한다
	void RefreshNativeDisplay();

protected:
	// UMG에서 바인딩할 재료 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UImage> MaterialIconImage;

	// UMG에서 바인딩할 재료 수량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTextBlock> MaterialQuantityText;

	// UMG에서 바인딩할 재료 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTextBlock> MaterialNameText;

	// 요구 수량을 충족했을 때 수량 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	FLinearColor EnoughQuantityColor = FLinearColor::White;

	// 요구 수량이 부족할 때 수량 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	FLinearColor NotEnoughQuantityColor = FLinearColor::Red;

private:
	// 현재 재료 비용 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (AllowPrivateAccess = "true"))
	FPRWeaponUpgradeMaterialCostViewData ViewData;
};
