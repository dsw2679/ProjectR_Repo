// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI Interaction Progress Bar 구현)
#include "PRInteractionProgressBar.h"

#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

UPRInteractionProgressBar::UPRInteractionProgressBar()
{
	// HUD 레이어, 입력 모드 영향 없음
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRInteractionProgressBar::SetProgress(float InPercent)
{
	if (!IsValid(ProgressBar))
	{
		return;
	}
	ProgressBar->SetPercent(FMath::Clamp(InPercent, 0.f, 1.f));
}

void UPRInteractionProgressBar::SetActionNameText(const FText& InText)
{
	if (IsValid(ActionNameText))
	{
		if (InText.IsEmpty())
		{
			ActionNameText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ActionNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
			ActionNameText->SetText(InText);
		}
	}
}

void UPRInteractionProgressBar::SetProgressBarVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bVisible)
	{
		ProgressBar->SetPercent(0.f);
	}
}