// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Party Member Health UI 위젯 구현)
#include "ProjectR/UI/HUD/PRPartyMemberHealthWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/HUD/PRHealthBarWidget.h"

UPRPartyMemberHealthWidget::UPRPartyMemberHealthWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRPartyMemberHealthWidget::NativeDestruct()
{
	ClearPlayerState();

	Super::NativeDestruct();
}

void UPRPartyMemberHealthWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundPlayerState.IsValid())
	{
		RefreshSurvivalState();
	}
}

/*~ Public API ~*/

void UPRPartyMemberHealthWidget::SetPlayerState(APRPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState))
	{
		ClearPlayerState();
		return;
	}

	if (BoundPlayerState.Get() == InPlayerState)
	{
		BindDisplayNameChanged(InPlayerState);
		RefreshDisplayName();
		RefreshSurvivalState();
		return;
	}

	UnbindDisplayNameChanged();
	BoundPlayerState = InPlayerState;
	BindDisplayNameChanged(InPlayerState);

	if (IsValid(HealthBar))
	{
		HealthBar->SetPresentationMode(EPRHealthBarPresentationMode::PartyMember);
		HealthBar->InitializeFromPlayerState(InPlayerState);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshDisplayName();
	RefreshSurvivalState();
	ApplySurvivalStateIcon();
}

void UPRPartyMemberHealthWidget::ClearPlayerState()
{
	UnbindDisplayNameChanged();
	BoundPlayerState.Reset();

	if (IsValid(HealthBar))
	{
		HealthBar->ClearHealthSource();
	}

	if (IsValid(SurvivalStateIconImage))
	{
		SurvivalStateIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

/*~ Display ~*/

void UPRPartyMemberHealthWidget::BindDisplayNameChanged(APRPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState) || DisplayNameChangedDelegateHandle.IsValid())
	{
		return;
	}

	DisplayNameChangedDelegateHandle = InPlayerState->OnDisplayNameChanged.AddUObject(this, &ThisClass::HandleDisplayNameChanged);
}

void UPRPartyMemberHealthWidget::UnbindDisplayNameChanged()
{
	if (!DisplayNameChangedDelegateHandle.IsValid())
	{
		return;
	}

	APRPlayerState* PlayerState = BoundPlayerState.Get();
	if (IsValid(PlayerState))
	{
		PlayerState->OnDisplayNameChanged.Remove(DisplayNameChangedDelegateHandle);
	}

	DisplayNameChangedDelegateHandle.Reset();
}

void UPRPartyMemberHealthWidget::HandleDisplayNameChanged(const FString&)
{
	RefreshDisplayName();
}

void UPRPartyMemberHealthWidget::RefreshDisplayName()
{
	APRPlayerState* PlayerState = BoundPlayerState.Get();
	if (!IsValid(PlayerState))
	{
		return;
	}

	FString DisplayName = PlayerState->GetDisplayName();
	if (DisplayName.IsEmpty())
	{
		DisplayName = PlayerState->GetPlayerName();
	}

	const FText DisplayNameTextValue = FText::FromString(DisplayName);
	if (IsValid(DisplayNameText))
	{
		DisplayNameText->SetText(DisplayNameTextValue);
	}
	BP_OnDisplayNameChanged(DisplayNameTextValue);
}

void UPRPartyMemberHealthWidget::RefreshSurvivalState()
{
	APRPlayerState* PlayerState = BoundPlayerState.Get();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(PlayerState) ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	EPRPartyMemberSurvivalState NewState = EPRPartyMemberSurvivalState::Alive;
	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead))
	{
		NewState = EPRPartyMemberSurvivalState::Dead;
	}
	else if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		|| PlayerState->IsDownTimerInfoActive())
	{
		NewState = EPRPartyMemberSurvivalState::Down;
	}

	if (SurvivalState != NewState)
	{
		SurvivalState = NewState;
		ApplySurvivalStateIcon();
		BP_OnSurvivalStateChanged(SurvivalState);
	}
}

void UPRPartyMemberHealthWidget::ApplySurvivalStateIcon()
{
	if (!IsValid(SurvivalStateIconImage))
	{
		return;
	}

	UTexture2D* IconTexture = GetSurvivalStateIconTexture();
	if (!IsValid(IconTexture))
	{
		SurvivalStateIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SurvivalStateIconImage->SetBrushFromTexture(IconTexture);
	SurvivalStateIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

UTexture2D* UPRPartyMemberHealthWidget::GetSurvivalStateIconTexture() const
{
	switch (SurvivalState)
	{
	case EPRPartyMemberSurvivalState::Down:
		return DownIconTexture.Get();
	case EPRPartyMemberSurvivalState::Dead:
		return DeadIconTexture.Get();
	case EPRPartyMemberSurvivalState::Alive:
	default:
		return AliveIconTexture.Get();
	}
}
