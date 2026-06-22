// Copyright ProjectR. All Rights Reserved.

#include "PRShopItemListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/UI/Shop/PRShopItemSlotWidget.h"

void UPRShopItemListWidget::SetShopItemList(EPRShopTabType InTabType, const TArray<FPRShopItemSlotViewData>& InItems)
{
	const bool bTabChanged = TabType != InTabType;
	TabType = InTabType;
	Items = InItems;

	if (IsValid(ItemListTypeText))
	{
		const FText TabText = TabType == EPRShopTabType::Buy
			? FText::FromString(TEXT("구매"))
			: FText::FromString(TEXT("판매"));
		ItemListTypeText->SetText(TabText);
	}

	const int32 DesiredSlotCount = GetDesiredSlotCount();
	const bool bSlotCountChanged = ItemSlotWidgets.Num() != DesiredSlotCount;
	if (bSlotCountChanged)
	{
		RebuildNativeItemSlots();
	}

	RebuildNativeItemList();

	if (bTabChanged || bSlotCountChanged)
	{
		ResetItemListScroll();
	}
}

/*~ UUserWidget Interface ~*/

void UPRShopItemListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RebuildNativeItemSlots();
	RebuildNativeItemList();
}

void UPRShopItemListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RebuildNativeItemSlots();
	RebuildNativeItemList();
}

void UPRShopItemListWidget::RebuildNativeItemSlots()
{
	ClearGeneratedItemSlots();

	if (!IsValid(ItemListPanel) || !IsValid(ItemSlotWidgetClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShopItemListWidget] 슬롯 생성 실패. 패널 또는 슬롯 클래스가 유효하지 않습니다."));
		return;
	}

	const int32 ValidSlotCount = GetDesiredSlotCount();
	for (int32 SlotIndex = 0; SlotIndex < ValidSlotCount; ++SlotIndex)
	{
		UPRShopItemSlotWidget* ItemSlotWidget = nullptr;
		APlayerController* OwningPlayer = GetOwningPlayer();
		if (IsValid(OwningPlayer))
		{
			ItemSlotWidget = CreateWidget<UPRShopItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass);
		}
		else if (IsValid(WidgetTree))
		{
			ItemSlotWidget = WidgetTree->ConstructWidget<UPRShopItemSlotWidget>(ItemSlotWidgetClass);
		}

		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		AddGeneratedItemSlot(ItemSlotWidget);
		AddItemSlotToPanel(ItemSlotWidget, SlotIndex);
	}
}

int32 UPRShopItemListWidget::GetDesiredSlotCount() const
{
	const int32 ValidRowCount = FMath::Max(0, RowCount);
	const int32 ValidColumnCount = FMath::Max(1, ColumnCount);
	const int32 VisibleSlotCount = ValidRowCount * ValidColumnCount;
	return FMath::Max(VisibleSlotCount, Items.Num());
}

void UPRShopItemListWidget::ClearGeneratedItemSlots()
{
	for (UPRShopItemSlotWidget* ItemSlotWidget : ItemSlotWidgets)
	{
		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		ItemSlotWidget->OnClicked.RemoveDynamic(this, &UPRShopItemListWidget::HandleItemSlotClicked);
	}

	ItemSlotWidgets.Reset();

	if (IsValid(ItemListPanel))
	{
		ItemListPanel->ClearChildren();
	}
}

void UPRShopItemListWidget::RebuildNativeItemList()
{
	for (int32 SlotIndex = 0; SlotIndex < ItemSlotWidgets.Num(); ++SlotIndex)
	{
		UPRShopItemSlotWidget* ItemSlotWidget = ItemSlotWidgets[SlotIndex];
		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		if (Items.IsValidIndex(SlotIndex))
		{
			ItemSlotWidget->SetSlotViewData(Items[SlotIndex]);
			ItemSlotWidget->SetVisibility(ESlateVisibility::Visible);
			continue;
		}

		FPRShopItemSlotViewData EmptyViewData;
		ItemSlotWidget->SetSlotViewData(EmptyViewData);
		ItemSlotWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPRShopItemListWidget::ResetItemListScroll()
{
	if (IsValid(ItemListScrollBox))
	{
		ItemListScrollBox->SetScrollOffset(0.0f);
	}
}

void UPRShopItemListWidget::AddItemSlotToPanel(UPRShopItemSlotWidget* ItemSlotWidget, int32 SlotIndex)
{
	if (!IsValid(ItemListPanel) || !IsValid(ItemSlotWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShopItemListWidget] 슬롯 패널 추가 실패. 패널 또는 슬롯이 유효하지 않습니다."));
		return;
	}

	if (UGridPanel* GridPanel = Cast<UGridPanel>(ItemListPanel))
	{
		const int32 ValidColumnCount = FMath::Max(1, ColumnCount);
		const int32 Row = SlotIndex / ValidColumnCount;
		const int32 Column = SlotIndex % ValidColumnCount;
		GridPanel->AddChildToGrid(ItemSlotWidget, Row, Column);
		return;
	}

	ItemListPanel->AddChild(ItemSlotWidget);
}

void UPRShopItemListWidget::AddGeneratedItemSlot(UPRShopItemSlotWidget* ItemSlotWidget)
{
	if (!IsValid(ItemSlotWidget))
	{
		return;
	}

	ItemSlotWidget->OnClicked.RemoveDynamic(this, &UPRShopItemListWidget::HandleItemSlotClicked);
	ItemSlotWidget->OnClicked.AddDynamic(this, &UPRShopItemListWidget::HandleItemSlotClicked);
	ItemSlotWidgets.Add(ItemSlotWidget);
}

void UPRShopItemListWidget::HandleItemSlotClicked(const FPRShopItemSlotViewData& ViewData)
{
	if (ViewData.EntryId.IsNone() || !IsValid(ViewData.ItemViewData.ItemData.Get()))
	{
		return;
	}

	OnItemSelected.Broadcast(ViewData);
}
