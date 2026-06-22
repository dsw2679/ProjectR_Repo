// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (픽업 Notification Entry UI 위젯 구현)
#include "PRPickupNotificationEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UPRPickupNotificationEntryWidget::UPRPickupNotificationEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRPickupNotificationEntryWidget::SetNotificationData(const FPRPickupNotificationViewData& InViewData)
{
	if (IsValid(InfoText))
	{
		InfoText->SetText(InViewData.InfoText);
	}

	if (!IsValid(ItemIcon))
	{
		return;
	}

	if (IsValid(InViewData.Icon))
	{
		ItemIcon->SetBrushFromTexture(InViewData.Icon);
		ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPRPickupNotificationEntryWidget::PlayFadeInAnimation()
{
	bWaitingFadeOutFinished = false;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (IsValid(FadeOut))
	{
		StopAnimation(FadeOut);
	}

	if (!IsValid(FadeIn))
	{
		return;
	}

	StopAnimation(FadeIn);
	PlayAnimation(FadeIn, 1.0f, 1);
}

void UPRPickupNotificationEntryWidget::PlayFadeOutAnimation()
{
	bWaitingFadeOutFinished = true;

	if (!IsValid(FadeOut))
	{
		HandleFadeOutFinished();
		return;
	}

	StopAnimation(FadeOut);
	PlayAnimation(FadeOut, 0.0f, 1);
}

void UPRPickupNotificationEntryWidget::CancelFadeOutAnimation()
{
	bWaitingFadeOutFinished = false;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.0f);

	if (IsValid(FadeOut))
	{
		StopAnimation(FadeOut);
	}
}

void UPRPickupNotificationEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(FadeOut))
	{
		FWidgetAnimationDynamicEvent FadeOutFinishedEvent;
		FadeOutFinishedEvent.BindDynamic(this, &ThisClass::HandleFadeOutFinished);
		BindToAnimationFinished(FadeOut, FadeOutFinishedEvent);
	}
}

void UPRPickupNotificationEntryWidget::NativeDestruct()
{
	if (IsValid(FadeOut))
	{
		UnbindAllFromAnimationFinished(FadeOut);
	}

	Super::NativeDestruct();
}

void UPRPickupNotificationEntryWidget::HandleFadeOutFinished()
{
	if (!bWaitingFadeOutFinished)
	{
		return;
	}

	bWaitingFadeOutFinished = false;
	OnFadeOutFinished.Broadcast(this);
}
