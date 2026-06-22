// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenu.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
#include "Components/Widget.h"
#include "ProjectR/UI/Growth/PRTraitWindowWidget.h"
#include "ProjectR/UI/Inventory/PRBagWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryWidget.h"
#include "PRPlayerMenuTabListWidget.h"

UPRPlayerMenu::UPRPlayerMenu()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRPlayerMenu::NativePreConstruct()
{
	// 부모 미리보기 처리
	Super::NativePreConstruct();

	if (!IsDesignTime() || !IsValid(TabList))
	{
		return;
	}

	TArray<FName> PreviewTabNames;
	if (IsValid(WidgetSwitcher))
	{
		for (int32 ChildIndex = 0; ChildIndex < WidgetSwitcher->GetChildrenCount(); ++ChildIndex)
		{
			UWidget* ChildWidget = WidgetSwitcher->GetChildAt(ChildIndex);
			if (!IsValid(ChildWidget))
			{
				continue;
			}

			// 디자인 탭 이름
			PreviewTabNames.Add(ChildWidget->GetFName());
		}
	}

	if (UPRPlayerMenuTabListWidget* PlayerMenuTabList = Cast<UPRPlayerMenuTabListWidget>(TabList))
	{
		PlayerMenuTabList->RebuildDesignPreviewTabs(PreviewTabNames, TabButtonClass);
	}
}

bool UPRPlayerMenu::RefreshRuntimeTabs(EPRPlayerMenuTabType DesiredTabType)
{
	if (!IsValid(WidgetSwitcher))
	{
		return false;
	}

	if (!IsValid(TabList))
	{
		return false;
	}

	const int32 DesiredTabIndex = ResolveRuntimeTabIndex(DesiredTabType);
	if (DesiredTabIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerMenu] 요청 탭 매핑 실패. TabType=%d"), static_cast<uint8>(DesiredTabType));
		return false;
	}

	if (UPRPlayerMenuTabListWidget* PlayerMenuTabList = Cast<UPRPlayerMenuTabListWidget>(TabList))
	{
		return PlayerMenuTabList->RegisterRuntimeTabs(WidgetSwitcher, TabButtonClass, DesiredTabIndex);
	}

	return false;
}

bool UPRPlayerMenu::SelectRuntimeTab(EPRPlayerMenuTabType TabType)
{
	if (!IsValid(WidgetSwitcher) || !IsValid(TabList))
	{
		return false;
	}

	const int32 TargetTabIndex = ResolveRuntimeTabIndex(TabType);
	if (TargetTabIndex == INDEX_NONE || TargetTabIndex >= TabList->GetTabCount())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerMenu] 탭 선택 실패. TabType=%d"), static_cast<uint8>(TabType));
		return false;
	}

	const FName TargetTabID = TabList->GetTabIdAtIndex(TargetTabIndex);
	if (TargetTabID.IsNone())
	{
		return false;
	}

	// 요청 탭 활성화
	TabList->SelectTabByID(TargetTabID, true);
	return true;
}

EPRPlayerMenuTabType UPRPlayerMenu::GetSelectedRuntimeTabType() const
{
	if (!IsValid(WidgetSwitcher))
	{
		return EPRPlayerMenuTabType::Inventory;
	}

	return ResolveRuntimeTabTypeByIndex(WidgetSwitcher->GetActiveWidgetIndex());
}

void UPRPlayerMenu::SetMenuSources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent, UPREquipmentManagerComponent* InEquipmentManagerComponent, APRPlayerState* InPlayerState)
{
	if (!IsValid(WidgetSwitcher))
	{
		return;
	}

	for (int32 ChildIndex = 0; ChildIndex < WidgetSwitcher->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* ChildWidget = WidgetSwitcher->GetChildAt(ChildIndex);
		if (UPRInventoryWidget* InventoryTabWidget = Cast<UPRInventoryWidget>(ChildWidget))
		{
			// 인벤토리 소스 전달
			InventoryTabWidget->SetInventorySources(InInventoryComponent, InWeaponManagerComponent, InQuickSlotComponent, InEquipmentManagerComponent);
			continue;
		}

		if (UPRBagWidget* BagTabWidget = Cast<UPRBagWidget>(ChildWidget))
		{
			// 가방 소스 전달
			BagTabWidget->SetBagSources(InInventoryComponent, InQuickSlotComponent);
			continue;
		}

		if (UPRTraitWindowWidget* TraitTabWidget = Cast<UPRTraitWindowWidget>(ChildWidget))
		{
			// 특성 소스 전달
			TraitTabWidget->SetGrowthSource(InPlayerState);
		}
	}
}

int32 UPRPlayerMenu::ResolveRuntimeTabIndex(EPRPlayerMenuTabType TabType) const
{
	if (!IsValid(WidgetSwitcher))
	{
		return INDEX_NONE;
	}

	const int32 TabIndex = static_cast<int32>(TabType);
	if (TabIndex >= 0 && TabIndex < WidgetSwitcher->GetChildrenCount())
	{
		return TabIndex;
	}

	return INDEX_NONE;
}

EPRPlayerMenuTabType UPRPlayerMenu::ResolveRuntimeTabTypeByIndex(int32 TabIndex) const
{
	switch (TabIndex)
	{
	case 0:
		return EPRPlayerMenuTabType::Inventory;
	case 1:
		return EPRPlayerMenuTabType::Trait;
	case 2:
		return EPRPlayerMenuTabType::Bag;
	case 3:
		return EPRPlayerMenuTabType::InGameMenu;
	default:
		return EPRPlayerMenuTabType::Inventory;
	}
}
