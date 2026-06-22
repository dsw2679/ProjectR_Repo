// Copyright ProjectR. All Rights Reserved.
// 김동석 (bgm 옵션 조절 기능 추가)

#include "PROptionWidget.h"

#include "Components/Button.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"

UPROptionWidget::UPROptionWidget()
{
	Layer = EPRUILayer::System;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPROptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonEvents();
}

void UPROptionWidget::NativeDestruct()
{
	UnbindButtonEvents();

	Super::NativeDestruct();
}

void UPROptionWidget::BindButtonEvents()
{
	UnbindButtonEvents();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}
}

void UPROptionWidget::UnbindButtonEvents()
{
	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}
}

UPRUIControllerComponent* UPROptionWidget::GetUIControllerComponent() const
{
	const APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	return IsValid(PlayerController) ? PlayerController->GetUIController() : nullptr;
}

void UPROptionWidget::HandleCloseButtonClicked()
{
	if (UPRUIControllerComponent* UIControllerComponent = GetUIControllerComponent())
	{
		UIControllerComponent->CloseOption();
	}
}
