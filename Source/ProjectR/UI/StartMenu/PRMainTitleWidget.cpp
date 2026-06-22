// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Main Title UI base implementation)
#include "PRMainTitleWidget.h"

#include "Animation/WidgetAnimation.h"

namespace
{
	constexpr float MainTitleFadeOutDuration = 1.0f;
}

UPRMainTitleWidget::UPRMainTitleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UPRMainTitleWidget::RequestContinue()
{
	if (bContinueRequested)
	{
		return;
	}

	bContinueRequested = true;
	if (IsValid(Flickering.Get()))
	{
		StopAnimation(Flickering);
	}

	if (IsValid(FadeOut.Get()))
	{
		PlayAnimation(FadeOut, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
	else
	{
		FinishContinueRequest();
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || MainTitleFadeOutDuration <= 0.0f)
	{
		FinishContinueRequest();
		return;
	}

	World->GetTimerManager().SetTimer(
		ContinueRequestTimerHandle,
		this,
		&UPRMainTitleWidget::FinishContinueRequest,
		MainTitleFadeOutDuration,
		false);
}

void UPRMainTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetKeyboardFocus();
	if (IsValid(Flickering.Get()))
	{
		PlayAnimation(Flickering, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void UPRMainTitleWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ContinueRequestTimerHandle);
	}

	Super::NativeDestruct();
}

FReply UPRMainTitleWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	RequestContinue();
	return FReply::Handled();
}

FReply UPRMainTitleWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	RequestContinue();
	return FReply::Handled();
}

void UPRMainTitleWidget::FinishContinueRequest()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ContinueRequestTimerHandle);
	}

	OnContinueRequested.Broadcast();
}
