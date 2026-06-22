// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (HUD Message UI 위젯 구현)
#include "PRHUDMessageWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"

UPRHUDMessageWidget::UPRHUDMessageWidget()
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRHUDMessageWidget::ShowMessage(const FText& InMessage)
{
	if (InMessage.IsEmpty())
	{
		HideMessage();
		return;
	}

	if (IsValid(MessageText))
	{
		MessageText->SetText(InMessage);
	}

	if (IsValid(MessagePanel))
	{
		MessagePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRHUDMessageWidget::HideMessage()
{
	if (IsValid(MessagePanel))
	{
		MessagePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UPRHUDMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HideMessage();

	UnbindFromEventManager();
	if (UPREventManagerSubsystem* EventManager = GetEventManager())
	{
		// PlayerController RPC가 로컬 EventManager로 발송한 HUD 메시지 이벤트 구독
		HUDMessageEventHandle = EventManager->Listen(
			PRGameplayTags::Event_Player_HUDMessage,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleHUDMessage));
	}
}

void UPRHUDMessageWidget::NativeDestruct()
{
	UnbindFromEventManager();
	Super::NativeDestruct();
}

void UPRHUDMessageWidget::HandleHUDMessage(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRHUDMessageEventPayload* MessagePayload = Payload.GetPtr<FPRHUDMessageEventPayload>();
	if (MessagePayload == nullptr || !MessagePayload->bShow)
	{
		// 빈 페이로드 또는 숨김 요청 처리
		HideMessage();
		return;
	}

	ShowMessage(MessagePayload->Message);
}

void UPRHUDMessageWidget::UnbindFromEventManager()
{
	if (UPREventManagerSubsystem* EventManager = GetEventManager())
	{
		EventManager->UnlistenAll(HUDMessageEventHandle);
	}
}
