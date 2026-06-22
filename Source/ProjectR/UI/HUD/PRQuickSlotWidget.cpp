// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (퀵슬롯 슬롯 UI 위젯 구현)
#include "PRQuickSlotWidget.h"

#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"

void UPRQuickSlotWidget::SetQuickSlotComponent(UPRQuickSlotComponent* InQuickSlotComponent)
{
	if (QuickSlotComponent == InQuickSlotComponent)
	{
		RefreshQuickSlots();
		return;
	}

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRQuickSlotWidget::HandleQuickSlotChanged);
	}

	QuickSlotComponent = InQuickSlotComponent;

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRQuickSlotWidget::HandleQuickSlotChanged);
		QuickSlotComponent->GetOnQuickSlotChanged().AddDynamic(this, &UPRQuickSlotWidget::HandleQuickSlotChanged);
	}

	RefreshQuickSlots();
}

void UPRQuickSlotWidget::RefreshQuickSlots()
{
	for (int32 SlotIndex = 0; SlotIndex < UPRQuickSlotComponent::MaxQuickSlotCount; ++SlotIndex)
	{
		RefreshQuickSlot(SlotIndex);
	}
}

void UPRQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeQuickSlotHUD();
}

void UPRQuickSlotWidget::NativeDestruct()
{
	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRQuickSlotWidget::HandleQuickSlotChanged);
	}

	Super::NativeDestruct();
}

void UPRQuickSlotWidget::InitializeQuickSlotHUD()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	APRPlayerState* PRPlayerState = OwningPlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return;
	}

	SetQuickSlotComponent(PRPlayerState->GetQuickSlotComponent());
}

void UPRQuickSlotWidget::HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex)
{
	if (ChangedQuickSlotComponent != QuickSlotComponent)
	{
		return;
	}

	if (ChangedSlotIndex == INDEX_NONE)
	{
		RefreshQuickSlots();
		return;
	}

	RefreshQuickSlot(ChangedSlotIndex);
}

void UPRQuickSlotWidget::RefreshQuickSlot(int32 SlotIndex)
{
	UPRItemSlotWidget* SlotWidget = GetQuickSlotItemWidget(SlotIndex);
	if (!IsValid(SlotWidget))
	{
		return;
	}

	const FPRQuickSlotViewData QuickSlotViewData = IsValid(QuickSlotComponent)
		? QuickSlotComponent->BuildQuickSlotViewData(SlotIndex)
		: FPRQuickSlotViewData();

	SlotWidget->SetSlotViewData(BuildItemSlotViewData(QuickSlotViewData));
}

UPRItemSlotWidget* UPRQuickSlotWidget::GetQuickSlotItemWidget(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:
		return QuickSlotItemWidget0;
	case 1:
		return QuickSlotItemWidget1;
	case 2:
		return QuickSlotItemWidget2;
	case 3:
		return QuickSlotItemWidget3;
	default:
		return nullptr;
	}
}

FPRInventoryItemSlotViewData UPRQuickSlotWidget::BuildItemSlotViewData(const FPRQuickSlotViewData& QuickSlotViewData) const
{
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemData = QuickSlotViewData.ConsumableData;
	ViewData.ItemInstance = QuickSlotViewData.ConsumableItem;
	ViewData.ItemType = EPRItemType::Consumable;
	ViewData.Icon = QuickSlotViewData.Icon;
	ViewData.StackCount = QuickSlotViewData.StackCount;
	ViewData.bShowStackCount = QuickSlotViewData.bHasRegisteredItem;
	ViewData.OwnedStackCount = QuickSlotViewData.StackCount;
	ViewData.bHasOwnedStackCount = QuickSlotViewData.bHasRegisteredItem;

	if (IsValid(QuickSlotViewData.ConsumableData))
	{
		ViewData.DisplayName = QuickSlotViewData.ConsumableData->GetDisplayName();
	}
	else
	{
		ViewData.DisplayName = FText::FromString(TEXT("(비어 있음)"));
	}

	return ViewData;
}
