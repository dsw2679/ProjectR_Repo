// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 Material Cost UI 위젯 구현)
#include "PRShopMaterialCostWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPRShopMaterialCostWidget::SetMaterialCostViewData(const FPRShopMaterialCostViewData& InViewData)
{
	ViewData = InViewData;
	RefreshNativeDisplay();
}

/*~ UUserWidget Interface ~*/

void UPRShopMaterialCostWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshNativeDisplay();
}

void UPRShopMaterialCostWidget::RefreshNativeDisplay()
{
	if (IsValid(MaterialIconImage))
	{
		UTexture2D* MaterialIcon = ViewData.Icon.Get();
		if (IsValid(MaterialIcon))
		{
			MaterialIconImage->SetVisibility(ESlateVisibility::Visible);
			MaterialIconImage->SetBrushFromTexture(MaterialIcon);
		}
		else
		{
			MaterialIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsValid(MaterialNameText))
	{
		MaterialNameText->SetText(ViewData.DisplayName);
	}

	if (IsValid(MaterialQuantityText))
	{
		MaterialQuantityText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), ViewData.RequiredQuantity)));
		MaterialQuantityText->SetColorAndOpacity(FSlateColor(ViewData.bEnough ? EnoughQuantityColor : NotEnoughQuantityColor));
	}
}
