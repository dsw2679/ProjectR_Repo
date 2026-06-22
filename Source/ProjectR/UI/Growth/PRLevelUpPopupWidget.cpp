// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (레벨 Up Popup UI 위젯 구현)
#include "PRLevelUpPopupWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

UPRLevelUpPopupWidget::UPRLevelUpPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRLevelUpPopupWidget::ShowLevelUp(int32 PreviousLevel, int32 CurrentLevel)
{
	if (IsValid(Text_Bottom))
	{
		Text_Bottom->SetText(FText::FromString(FString::Printf(TEXT("%d -> %d"), PreviousLevel, CurrentLevel)));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	PlayLevelUpFadeAnimation();
	OnLevelUpPopupShown(PreviousLevel, CurrentLevel);

	ClearAutoHideTimer();
	if (AutoHideDelay > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoHideTimerHandle,
				this,
				&ThisClass::HideLevelUpPopup,
				AutoHideDelay,
				false);
		}
	}
}

void UPRLevelUpPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UPRLevelUpPopupWidget::NativeDestruct()
{
	ClearAutoHideTimer();

	Super::NativeDestruct();
}

void UPRLevelUpPopupWidget::PlayLevelUpFadeAnimation_Implementation()
{
	if (!IsValid(Fade))
	{
		return;
	}

	StopAnimation(Fade);
	PlayAnimation(Fade, 0.0f, 1);
}

void UPRLevelUpPopupWidget::HideLevelUpPopup()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPRLevelUpPopupWidget::ClearAutoHideTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoHideTimerHandle);
	}
}
