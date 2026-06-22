// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 등급 비주얼 및 마우스 오버 툴팁 호출 구현)
// Author: 배유찬 (슬롯 클릭 시 장비 장착/해제 어빌리티 연동 구현)
// Author: 이건주 (3D 모델 아이콘 렌더링 구현)
#include "PRItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"

void UPRItemSlotWidget::SetSlotViewData(const FPRInventoryItemSlotViewData& InViewData)
{
	ViewData = InViewData;
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	RefreshNativeDisplay();
	if (bIsMouseHovering)
	{
		// 현재 표시 데이터로 툴팁 위젯을 갱신한다
		ShowTooltip();
	}
}

/*~ UUserWidget Interface ~*/

void UPRItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// 버튼 이벤트를 바인딩한다
	if (IsValid(ClickButton))
	{
		ClickButton->OnClicked.RemoveDynamic(this, &UPRItemSlotWidget::HandleClickButtonClicked);
		ClickButton->OnClicked.AddDynamic(this, &UPRItemSlotWidget::HandleClickButtonClicked);
	}
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	RefreshNativeDisplay();
	
	if (IsValid(EquippedIndicatorImage))
	{
		EquippedIndicatorImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPRItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsMouseHovering = true;
	ShowTooltip();
}

void UPRItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	bIsMouseHovering = false;
	HideTooltip();

	Super::NativeOnMouseLeave(InMouseEvent);
}

void UPRItemSlotWidget::NativeDestruct()
{
	HideTooltip();
	Super::NativeDestruct();
}

FReply UPRItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnRightClicked.Broadcast(ViewData);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsValid(ClickButton))
	{
		OnLeftClicked.Broadcast(ViewData);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPRItemSlotWidget::HandleClickButtonClicked()
{
	OnLeftClicked.Broadcast(ViewData);
}

void UPRItemSlotWidget::RefreshNativeDisplay()
{
	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(ViewData.DisplayName);
	}

	if (IsValid(ItemIconImage))
	{
		UTexture2D* ItemIcon = ViewData.Icon.Get();
		if (!IsValid(ItemIcon))
		{
			// 아이템 아이콘이 없으면 아이템 아이콘 이미지 숨김
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Visible);
			ItemIconImage->SetBrushFromTexture(ItemIcon);
		}
	}

	if (IsValid(EquippedIndicatorImage))
	{
		// 실제 아이템 항목의 장착 상태 표시와 명령 항목 표시 분리
		const bool bShowEquippedIndicator = ViewData.InventoryAction == EPRInventoryAction::Deactivate
			&& IsValid(ViewData.ItemInstance.Get())
			&& IsValid(ViewData.ItemData.Get());
		const bool bShowIndicator = bShowEquippedIndicator || ViewData.bSelected;
		const ESlateVisibility EquippedIndicatorVisibility = bShowIndicator
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden;
		EquippedIndicatorImage->SetVisibility(EquippedIndicatorVisibility);
	}

	if (IsValid(StackCountText))
	{
		const bool bShowStackCount = ViewData.bShowStackCount && IsValid(ViewData.ItemData.Get());
		StackCountText->SetVisibility(bShowStackCount ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		StackCountText->SetText(FText::AsNumber(ViewData.StackCount));
	}
}

void UPRItemSlotWidget::ShowTooltip()
{
	if (!IsValid(ViewData.ItemData.Get()))
	{
		HideTooltip();
		return;
	}

	UPRUIControllerComponent* UIController = ResolveUIController();
	if (!IsValid(UIController))
	{
		return;
	}

	UIController->ShowItemTooltip(this, ViewData);
}

void UPRItemSlotWidget::HideTooltip()
{
	if (UPRUIControllerComponent* UIController = ResolveUIController())
	{
		UIController->HideItemTooltip();
	}
}

UPRUIControllerComponent* UPRItemSlotWidget::ResolveUIController() const
{
	const APRPlayerController* PRPlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	return IsValid(PRPlayerController) ? PRPlayerController->GetUIController() : nullptr;
}
