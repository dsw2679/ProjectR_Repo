// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 충전 게이지 비주얼 연동 구현)
// Author: 이건주 (무기 종류 아이콘 실시간 갱신 구현)
#include "PRWeaponStatusWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"

void UPRWeaponStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (HighlightBorder)
	{
		HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	StopModDurationText();
}

void UPRWeaponStatusWidget::NativeDestruct()
{
	StopModDurationText();
	Super::NativeDestruct();
}

void UPRWeaponStatusWidget::SetSlotType(EPRWeaponSlotType InSlotType)
{
	SlotType = InSlotType;
}

void UPRWeaponStatusWidget::SetWeaponStatus(const FPRWeaponStatusViewData& ViewData)
{
	SlotType = ViewData.SlotType;

	if (!ViewData.bHasWeapon)
	{
		ClearWeaponStatus();
		return;
	}
	
	UnbindHighlightEvent();
	BindHighlightEvent();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetWeaponIcon(ViewData.WeaponIcon);
	SetMagazineAmmoText(ViewData.MagazineAmmo);
	SetReserveAmmoText(ViewData.ReserveAmmo);

	if (ViewData.bHasMod)
	{
		SetModIcon(ViewData.ModIcon);
		SetModGaugePercent(ViewData.ModGaugePercent);
		if (ViewData.bUsesModDuration)
		{
			HideModStackText();
			StartModDurationText(ViewData.ModRemainingDurationSeconds);
		}
		else
		{
			StopModDurationText();
			SetModStackText(ViewData.ModStackCount);
		}
	}
	else
	{
		ClearModStatus();
	}
}

void UPRWeaponStatusWidget::SetWeaponIcon(UTexture2D* Icon)
{
	if (!IsValid(WeaponIconImage))
	{
		return;
	}

	if (!IsValid(Icon))
	{
		WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	WeaponIconImage->SetBrushFromTexture(Icon);
	WeaponIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetMagazineAmmoText(float MagazineAmmo)
{
	if (!IsValid(MagazineAmmoText))
	{
		return;
	}

	MagazineAmmoText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(MagazineAmmo));
	MagazineAmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetReserveAmmoText(float ReserveAmmo)
{
	if (!IsValid(ReserveAmmoText))
	{
		return;
	}

	ReserveAmmoText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(ReserveAmmo));
	ReserveAmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModIcon(UTexture2D* Icon)
{
	if (!IsValid(ModIconImage))
	{
		return;
	}

	if (!IsValid(Icon))
	{
		ModIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ModIconImage->SetBrushFromTexture(Icon);
	ModIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModGaugePercent(float Percent)
{
	if (!IsValid(ModGaugeBar))
	{
		return;
	}

	ModGaugeBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	ModGaugeBar->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModStackText(float StackCount)
{
	if (!IsValid(ModStackText))
	{
		return;
	}

	ModStackText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(StackCount));
	ModStackText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::ClearWeaponStatus()
{
	if (IsValid(WeaponIconImage))
	{
		WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	SetMagazineAmmoText(0.0f);
	SetReserveAmmoText(0.0f);

	ClearModStatus();
	UnbindHighlightEvent();
}

void UPRWeaponStatusWidget::ClearModStatus()
{
	if (IsValid(ModIconImage))
	{
		ModIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(ModGaugeBar))
	{
		ModGaugeBar->SetPercent(0.0f);
		ModGaugeBar->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(ModStackText))
	{
		ModStackText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(0.0f));
		ModStackText->SetVisibility(ESlateVisibility::Hidden);
	}

	StopModDurationText();
}

void UPRWeaponStatusWidget::BindHighlightEvent()
{
	// 모드 활성화 수신 대기
	if (UPREventManagerSubsystem* EventManager = GetEventManager())
	{
		HighlightEventDelegateHandle = EventManager->Listen(PRGameplayTags::Event_Player_ModActivation,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleModActivation));
	}
}

void UPRWeaponStatusWidget::StartModDurationText(float RemainingDurationSeconds)
{
	if (!IsValid(ModDurationText))
	{
		return;
	}

	ModDurationEndServerWorldTimeSeconds = ResolveServerWorldTimeSeconds() + FMath::Max(RemainingDurationSeconds, 0.0f);
	RefreshModDurationText();

	if (RemainingDurationSeconds <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		ModDurationTimerHandle,
		this,
		&UPRWeaponStatusWidget::RefreshModDurationText,
		0.1f,
		true);
}

void UPRWeaponStatusWidget::StopModDurationText()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ModDurationTimerHandle);
	}

	ModDurationEndServerWorldTimeSeconds = 0.0f;

	if (IsValid(ModDurationText))
	{
		ModDurationText->SetText(FText::GetEmpty());
		ModDurationText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPRWeaponStatusWidget::RefreshModDurationText()
{
	if (!IsValid(ModDurationText))
	{
		StopModDurationText();
		return;
	}

	const float RemainingDurationSeconds = FMath::Max(ModDurationEndServerWorldTimeSeconds - ResolveServerWorldTimeSeconds(), 0.0f);
	if (RemainingDurationSeconds <= 0.0f)
	{
		StopModDurationText();
		return;
	}

	ModDurationText->SetText(MakeModDurationText(RemainingDurationSeconds));
	ModDurationText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

FText UPRWeaponStatusWidget::MakeModDurationText(float RemainingDurationSeconds) const
{
	const int32 MaximumFractionalDigits = RemainingDurationSeconds >= 1.0f ? 0 : 1;
	return UPRUserInterfaceStatics::ConvertFloatToText(RemainingDurationSeconds, MaximumFractionalDigits);
}

float UPRWeaponStatusWidget::ResolveServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UPRWeaponStatusWidget::HideModStackText()
{
	if (!IsValid(ModStackText))
	{
		return;
	}

	ModStackText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(0.0f));
	ModStackText->SetVisibility(ESlateVisibility::Hidden);
}

void UPRWeaponStatusWidget::UnbindHighlightEvent()
{
	if (HighlightEventDelegateHandle.IsValid())
	{
		if (UPREventManagerSubsystem* EventManager = GetEventManager())
		{
			EventManager->UnlistenAll(HighlightEventDelegateHandle);
		}
	}
}

void UPRWeaponStatusWidget::HandleModActivation(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRModActivationPayload* EventData = Payload.GetPtr<FPRModActivationPayload>();
	if (!EventData)
	{
		return;
	}
	
	if (EventData->SlotType != SlotType)
	{
		return;
	}
	
	if (EventData->bActivated)
	{
		if (EventData->bUsesModDuration)
		{
			HideModStackText();
			StartModDurationText(EventData->ModDurationSeconds);
		}

		if (HighlightBorder)
		{
			HighlightBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (HighlightAnimation)
		{
			PlayAnimation(HighlightAnimation,0.f,0);
		}
	}
	else
	{
		if (EventData->bUsesModDuration && EventData->bWasCancelled)
		{
			StopModDurationText();
		}

		if (HighlightBorder)
		{
			HighlightBorder->SetVisibility(ESlateVisibility::Hidden);
		}
		if (HighlightAnimation)
		{
			StopAnimation(HighlightAnimation);
		}
	}
}
