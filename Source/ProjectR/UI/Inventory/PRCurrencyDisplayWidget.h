// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (재화 Display UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRCurrencyDisplayWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

// 고철 아이콘과 보유 수량을 하나의 표시 단위로 묶는 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRCurrencyDisplayWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 표시할 고철 수량을 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetScrapAmount(int32 InScrapAmount);

	// 현재 표시 중인 고철 수량을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	int32 GetScrapAmount() const { return ScrapAmount; }

protected:
	/*~ UUserWidget Interface ~*/
	// 에디터와 런타임에서 기본 아이콘과 수량을 네이티브 위젯에 반영한다
	virtual void NativePreConstruct() override;

private:
	// 현재 표시 상태를 바인딩된 이미지와 텍스트에 반영한다
	void RefreshNativeDisplay();

protected:
	// UMG에서 바인딩할 고철 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UImage> ScrapIconImage;

	// UMG에서 바인딩할 고철 보유량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ScrapAmountText;

	// 고철 아이콘 이미지에 적용할 기본 텍스처
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UTexture2D> ScrapIconTexture;

private:
	// 현재 표시 중인 고철 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	int32 ScrapAmount = 0;
};
