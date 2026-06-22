// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction Hint UI 위젯 구현)
#include "PRInteractionHintWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"


void UPRInteractionHintWidget::SetHintVisible(bool bVisible)
{
	if (bVisible)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPRInteractionHintWidget::SetInteractionHintText(const FText& InText)
{
	if (IsValid(InteractionHintText))
	{
		if (InText.IsEmpty())
		{
			InteractionHintText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			InteractionHintText->SetVisibility(ESlateVisibility::HitTestInvisible);
			InteractionHintText->SetText(InText);
		}
	}
}

void UPRInteractionHintWidget::SetCanInteract(const bool bCanInteractable)
{
	// TODO: 상호작용 불가시 힌트 반투명 처리
}
