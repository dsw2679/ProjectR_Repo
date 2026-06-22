// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 Item Detail UI 위젯 구현)
#include "PRShopItemDetailWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/UI/Shop/PRShopMaterialCostWidget.h"
#include "Styling/SlateColor.h"

void UPRShopItemDetailWidget::SetSelectedItemDetails(const FPRShopItemSlotViewData& InViewData, EPRShopTabType InCurrentTabType, int32 InOwnedStackCount)
{
	ViewData = InViewData;
	CurrentTabType = InCurrentTabType;
	OwnedStackCount = InOwnedStackCount;
	RefreshNativeDisplay();
}

/*~ UUserWidget Interface ~*/

void UPRShopItemDetailWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RefreshNativeDisplay();
}

void UPRShopItemDetailWidget::RefreshNativeDisplay()
{
	const FPRInventoryItemSlotViewData& ItemViewData = ViewData.ItemViewData;
	UPRItemDataAsset* ItemData = ItemViewData.ItemData.Get();
	const bool bHasSelectedItem = IsValid(ItemData);

	if (IsValid(SelectedItemNameText))
	{
		SelectedItemNameText->SetText(bHasSelectedItem ? ItemViewData.DisplayName : FText::FromString(TEXT("아이템 선택")));
	}

	if (IsValid(SelectedItemDescriptionText))
	{
		SelectedItemDescriptionText->SetText(bHasSelectedItem ? ItemData->GetDescription() : FText::GetEmpty());
	}

	if (IsValid(SelectedItemIconImage))
	{
		UTexture2D* ItemIcon = bHasSelectedItem ? ItemViewData.Icon.Get() : nullptr;
		if (IsValid(ItemIcon))
		{
			SelectedItemIconImage->SetVisibility(ESlateVisibility::Visible);
			SelectedItemIconImage->SetBrushFromTexture(ItemIcon);
		}
		else
		{
			SelectedItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsValid(SelectedItemPriceText))
	{
		SelectedItemPriceText->SetText(bHasSelectedItem ? FText::AsNumber(ViewData.UnitScrapPrice) : FText::GetEmpty());
		const bool bEnoughPrice = !bHasSelectedItem || CurrentTabType != EPRShopTabType::Buy || ViewData.bEnoughScrap;
		SelectedItemPriceText->SetColorAndOpacity(FSlateColor(bEnoughPrice ? EnoughPriceColor : NotEnoughPriceColor));
	}

	RefreshMaterialCostWidgets();

	if (IsValid(SelectedItemStockText))
	{
		SelectedItemStockText->SetText(MakeStockText(bHasSelectedItem));
	}

	if (IsValid(SelectedItemOwnedCountText))
	{
		SelectedItemOwnedCountText->SetText(bHasSelectedItem ? FText::AsNumber(OwnedStackCount) : FText::GetEmpty());
	}
}

void UPRShopItemDetailWidget::RefreshMaterialCostWidgets()
{
	ClearMaterialCostWidgets();

	if (!IsValid(SelectedItemMaterialCostPanel) || !IsValid(MaterialCostWidgetClass.Get()))
	{
		return;
	}

	if (ViewData.MaterialCosts.IsEmpty())
	{
		SelectedItemMaterialCostPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SelectedItemMaterialCostPanel->SetVisibility(ESlateVisibility::Visible);
	for (const FPRShopMaterialCostViewData& MaterialCostViewData : ViewData.MaterialCosts)
	{
		AddMaterialCostWidget(MaterialCostViewData);
	}
}

void UPRShopItemDetailWidget::ClearMaterialCostWidgets()
{
	MaterialCostWidgets.Reset();

	if (IsValid(SelectedItemMaterialCostPanel))
	{
		SelectedItemMaterialCostPanel->ClearChildren();
	}
}

void UPRShopItemDetailWidget::AddMaterialCostWidget(const FPRShopMaterialCostViewData& MaterialCostViewData)
{
	if (!IsValid(SelectedItemMaterialCostPanel) || !IsValid(MaterialCostWidgetClass.Get()))
	{
		return;
	}

	UPRShopMaterialCostWidget* MaterialCostWidget = nullptr;
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (IsValid(OwningPlayer))
	{
		MaterialCostWidget = CreateWidget<UPRShopMaterialCostWidget>(OwningPlayer, MaterialCostWidgetClass);
	}
	else if (IsValid(WidgetTree))
	{
		MaterialCostWidget = WidgetTree->ConstructWidget<UPRShopMaterialCostWidget>(MaterialCostWidgetClass);
	}

	if (!IsValid(MaterialCostWidget))
	{
		return;
	}

	MaterialCostWidget->SetMaterialCostViewData(MaterialCostViewData);
	SelectedItemMaterialCostPanel->AddChild(MaterialCostWidget);
	MaterialCostWidgets.Add(MaterialCostWidget);
}

FText UPRShopItemDetailWidget::MakeStockText(bool bHasSelectedItem) const
{
	if (!bHasSelectedItem || CurrentTabType == EPRShopTabType::Sell)
	{
		return FText::GetEmpty();
	}

	if (ViewData.bUnlimitedStock)
	{
		return FText::FromString(TEXT("제한 없음"));
	}

	return FText::AsNumber(ViewData.RemainingStock);
}
