// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (픽업 Notification List UI 위젯 구현)
#include "PRPickupNotificationListWidget.h"

#include "Components/VerticalBox.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/UI/HUD/PRPickupNotificationEntryWidget.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "PRPickupNotificationListWidget"

UPRPickupNotificationListWidget::UPRPickupNotificationListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRPickupNotificationListWidget::AddNotification(const FPRPickupNotificationPayload& Payload)
{
	if (!IsValid(NotificationListBox) || !IsValid(EntryWidgetClass.Get()) || Payload.Quantity <= 0)
	{
		return;
	}

	if (FPRPickupNotificationEntryState* MergeTarget = FindMergeTarget(Payload))
	{
		MergeTarget->Quantity += Payload.Quantity;

		FPRPickupNotificationPayload MergedPayload = Payload;
		MergedPayload.Quantity = MergeTarget->Quantity;
		MergeTarget->EntryWidget->SetNotificationData(MakeViewData(MergedPayload));
		MergeTarget->EntryWidget->CancelFadeOutAnimation();
		RestartDisplayTimer(*MergeTarget);
		return;
	}

	const int32 EffectiveMaxVisibleCount = FMath::Max(1, MaxVisibleCount);
	while (EntryStates.Num() >= EffectiveMaxVisibleCount)
	{
		RemoveOldestNotificationImmediately();
	}

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	UPRPickupNotificationEntryWidget* EntryWidget = CreateWidget<UPRPickupNotificationEntryWidget>(OwningPlayerController, EntryWidgetClass);
	if (!IsValid(EntryWidget))
	{
		return;
	}

	EntryWidget->SetNotificationData(MakeViewData(Payload));
	EntryWidget->OnFadeOutFinished.AddDynamic(this, &ThisClass::HandleEntryFadeOutFinished);
	NotificationListBox->AddChildToVerticalBox(EntryWidget);

	FPRPickupNotificationEntryState EntryState;
	EntryState.EntryWidget = EntryWidget;
	EntryState.RewardType = Payload.RewardType;
	EntryState.ItemAssetId = Payload.ItemAssetId;
	EntryState.Quantity = Payload.Quantity;
	EntryStates.Add(EntryState);

	EntryWidget->PlayFadeInAnimation();

	RestartDisplayTimer(EntryStates.Last());
}

void UPRPickupNotificationListWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		for (FPRPickupNotificationEntryState& EntryState : EntryStates)
		{
			World->GetTimerManager().ClearTimer(EntryState.DisplayTimerHandle);
		}
	}

	EntryStates.Reset();

	Super::NativeDestruct();
}

FPRPickupNotificationViewData UPRPickupNotificationListWidget::MakeViewData(const FPRPickupNotificationPayload& Payload) const
{
	FPRPickupNotificationViewData ViewData;

	if (Payload.RewardType == EPRRewardType::Currency)
	{
		ViewData.InfoText = FText::Format(
			LOCTEXT("ScrapNotificationFormat", "+{0} {1}"),
			FText::AsNumber(Payload.Quantity),
			LOCTEXT("ScrapName", "고철"));
		ViewData.Icon = ScrapIconTexture;
		return ViewData;
	}

	if ((Payload.RewardType == EPRRewardType::Item || Payload.RewardType == EPRRewardType::Ammo) && Payload.ItemAssetId.IsValid())
	{
		UPRItemDataAsset* ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Payload.ItemAssetId);
		const FText ItemName = IsValid(ItemData)
			? ItemData->GetDisplayName()
			: FText::FromName(Payload.ItemAssetId.PrimaryAssetName);

		ViewData.InfoText = FText::Format(
			LOCTEXT("ItemNotificationFormat", "+{0} {1}"),
			FText::AsNumber(Payload.Quantity),
			ItemName);
		ViewData.Icon = IsValid(ItemData) ? ItemData->GetIcon() : nullptr;
	}

	return ViewData;
}

FPRPickupNotificationEntryState* UPRPickupNotificationListWidget::FindMergeTarget(const FPRPickupNotificationPayload& Payload)
{
	for (FPRPickupNotificationEntryState& EntryState : EntryStates)
	{
		if (IsSameNotification(EntryState, Payload))
		{
			return &EntryState;
		}
	}

	return nullptr;
}

bool UPRPickupNotificationListWidget::IsSameNotification(const FPRPickupNotificationEntryState& EntryState, const FPRPickupNotificationPayload& Payload) const
{
	if (!IsValid(EntryState.EntryWidget) || EntryState.RewardType != Payload.RewardType)
	{
		return false;
	}

	if (Payload.RewardType == EPRRewardType::Currency)
	{
		return true;
	}

	if (Payload.RewardType == EPRRewardType::Item || Payload.RewardType == EPRRewardType::Ammo)
	{
		return EntryState.ItemAssetId == Payload.ItemAssetId;
	}

	return false;
}

void UPRPickupNotificationListWidget::RestartDisplayTimer(FPRPickupNotificationEntryState& EntryState)
{
	UPRPickupNotificationEntryWidget* EntryWidget = EntryState.EntryWidget;
	if (!IsValid(EntryWidget))
	{
		return;
	}

	if (DisplayDuration <= 0.0f)
	{
		HandleDisplayDurationExpired(EntryWidget);
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EntryState.DisplayTimerHandle);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ThisClass::HandleDisplayDurationExpired, EntryWidget);
	World->GetTimerManager().SetTimer(
		EntryState.DisplayTimerHandle,
		TimerDelegate,
		DisplayDuration,
		false);
}

void UPRPickupNotificationListWidget::RemoveOldestNotificationImmediately()
{
	if (EntryStates.IsEmpty())
	{
		return;
	}

	UPRPickupNotificationEntryWidget* OldestEntryWidget = EntryStates[0].EntryWidget;
	RemoveNotificationImmediately(OldestEntryWidget);
}

void UPRPickupNotificationListWidget::RemoveNotificationImmediately(UPRPickupNotificationEntryWidget* EntryWidget)
{
	if (!IsValid(EntryWidget))
	{
		return;
	}

	ClearDisplayTimer(EntryWidget);
	EntryWidget->OnFadeOutFinished.RemoveAll(this);

	if (IsValid(NotificationListBox))
	{
		NotificationListBox->RemoveChild(EntryWidget);
	}
	else
	{
		EntryWidget->RemoveFromParent();
	}

	EntryStates.RemoveAll([EntryWidget](const FPRPickupNotificationEntryState& EntryState)
	{
		return EntryState.EntryWidget == EntryWidget;
	});
}

void UPRPickupNotificationListWidget::ClearDisplayTimer(UPRPickupNotificationEntryWidget* EntryWidget)
{
	if (!IsValid(EntryWidget))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (FPRPickupNotificationEntryState& EntryState : EntryStates)
	{
		if (EntryState.EntryWidget == EntryWidget)
		{
			World->GetTimerManager().ClearTimer(EntryState.DisplayTimerHandle);
			return;
		}
	}
}

void UPRPickupNotificationListWidget::HandleDisplayDurationExpired(UPRPickupNotificationEntryWidget* EntryWidget)
{
	if (!IsValid(EntryWidget))
	{
		return;
	}

	ClearDisplayTimer(EntryWidget);
	EntryWidget->PlayFadeOutAnimation();
}

void UPRPickupNotificationListWidget::HandleEntryFadeOutFinished(UPRPickupNotificationEntryWidget* EntryWidget)
{
	RemoveNotificationImmediately(EntryWidget);
}

#undef LOCTEXT_NAMESPACE
