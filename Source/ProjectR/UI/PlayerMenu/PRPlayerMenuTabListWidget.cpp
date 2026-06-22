// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenuTabListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "Components/Widget.h"
#include "ProjectR/UI/TextButton/PRTextButton.h"

void UPRPlayerMenuTabListWidget::RebuildDesignPreviewTabs(const TArray<FName>& TabNameIDs, TSubclassOf<UCommonButtonBase> ButtonWidgetType)
{
	if (!IsDesignTime() || !IsValid(TabButtonContainer))
	{
		return;
	}

	TabButtonContainer->ClearChildren();
	if (!IsValid(ButtonWidgetType.Get()) || !IsValid(WidgetTree))
	{
		return;
	}

	for (const FName TabNameID : TabNameIDs)
	{
		UCommonButtonBase* PreviewTabButton = WidgetTree->ConstructWidget<UCommonButtonBase>(ButtonWidgetType);
		if (!IsValid(PreviewTabButton))
		{
			continue;
		}

		UPRTextButton* PreviewTextButton = Cast<UPRTextButton>(PreviewTabButton);
		if (!IsValid(PreviewTextButton))
		{
			TabButtonContainer->AddChild(PreviewTabButton);
			continue;
		}

		// 디자인 탭 텍스트
		PreviewTextButton->SetText(FText::FromName(TabNameID));
		TabButtonContainer->AddChild(PreviewTextButton);
	}
}

bool UPRPlayerMenuTabListWidget::RegisterRuntimeTabs(UCommonActivatableWidgetSwitcher* InWidgetSwitcher, TSubclassOf<UCommonButtonBase> ButtonWidgetType, int32 DesiredTabIndex)
{
	if (!IsValid(InWidgetSwitcher) || !IsValid(TabButtonContainer) || !IsValid(ButtonWidgetType.Get()))
	{
		return false;
	}

	// 기존 탭 정리
	RemoveAllTabs();

	// 스위처 연결
	SetLinkedSwitcher(InWidgetSwitcher);

	FName FallbackTabName = NAME_None;
	FName DesiredTabName = NAME_None;
	for (int32 ChildIndex = 0; ChildIndex < InWidgetSwitcher->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* ChildWidget = InWidgetSwitcher->GetChildAt(ChildIndex);
		if (!IsValid(ChildWidget))
		{
			continue;
		}

		const FName ChildName = ChildWidget->GetFName();
		if (FallbackTabName.IsNone())
		{
			FallbackTabName = ChildName;
		}

		if (ChildIndex == DesiredTabIndex)
		{
			DesiredTabName = ChildName;
		}

		// 스위처 자식 기반 탭 등록
		RegisterTab(ChildName, ButtonWidgetType, ChildWidget, ChildIndex);
	}

	const FName SelectTabName = DesiredTabName.IsNone() ? FallbackTabName : DesiredTabName;
	if (!SelectTabName.IsNone())
	{
		// 요청 탭 선택
		SelectTabByID(SelectTabName, true);
	}

	// 탭 입력 수신
	SetListeningForInput(true);

	return true;
}

void UPRPlayerMenuTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	if (!IsValid(TabButtonContainer))
	{
		return;
	}

	if (!IsValid(TabButton))
	{
		return;
	}

	UPRTextButton* TabTextButton = Cast<UPRTextButton>(TabButton);
	if (!IsValid(TabTextButton))
	{
		return;
	}


	TabTextButton->SetText(FText::FromName(TabNameID));
	TabButtonContainer->AddChild(TabTextButton);
}

void UPRPlayerMenuTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);

	if (!IsValid(TabButtonContainer))
	{
		return;
	}

	TabButtonContainer->RemoveChild(TabButton);
}
