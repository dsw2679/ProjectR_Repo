// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (재화 Display UI 위젯 구현)
#include "PRCurrencyDisplayWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPRCurrencyDisplayWidget::SetScrapAmount(int32 InScrapAmount)
{
	ScrapAmount = FMath::Max(0, InScrapAmount);
	RefreshNativeDisplay();
}

/*~ UUserWidget Interface ~*/

void UPRCurrencyDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshNativeDisplay();
}

void UPRCurrencyDisplayWidget::RefreshNativeDisplay()
{
	if (IsValid(ScrapIconImage) && IsValid(ScrapIconTexture))
	{
		ScrapIconImage->SetBrushFromTexture(ScrapIconTexture);
	}

	if (IsValid(ScrapAmountText))
	{
		ScrapAmountText->SetText(FText::AsNumber(ScrapAmount));
	}
}
