// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Stamina Bar UI 위젯 구현)
#include "ProjectR/UI/HUD/PRStaminaBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "TimerManager.h"

UPRStaminaBarWidget::UPRStaminaBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRStaminaBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentBindRetryCount = 0;
	TryBindToOwnerAbilitySystem();
}

void UPRStaminaBarWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	UnbindFromAbilitySystem();

	Super::NativeDestruct();
}

void UPRStaminaBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bHasInitializedPercent || !bInterpolateStaminaChange)
	{
		return;
	}

	if (FMath::IsNearlyEqual(CurrentPercent, TargetPercent, StaminaSnapTolerance))
	{
		if (!FMath::IsNearlyEqual(CurrentPercent, TargetPercent))
		{
			CurrentPercent = TargetPercent;
			ApplyDisplayedPercentToFill();
		}
		return;
	}

	CurrentPercent = FMath::FInterpTo(CurrentPercent, TargetPercent, InDeltaTime, StaminaInterpSpeed);
	ApplyDisplayedPercentToFill();
}

/*~ Public API ~*/

void UPRStaminaBarWidget::InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	BindToAbilitySystem(InAbilitySystemComponent);
}

void UPRStaminaBarWidget::RefreshStaminaFromOwner()
{
	TryBindToOwnerAbilitySystem();
	RefreshStaminaFromAbilitySystem();
}

void UPRStaminaBarWidget::SetStaminaPercent(float NewPercent)
{
	TargetPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);

	if (!bHasInitializedPercent || !bInterpolateStaminaChange)
	{
		CurrentPercent = TargetPercent;
		bHasInitializedPercent = true;
		ApplyDisplayedPercentToFill();
	}

	BP_OnStaminaPercentChanged(TargetPercent, CurrentStamina, CurrentMaxStamina);
}

/*~ Binding ~*/

void UPRStaminaBarWidget::TryBindToOwnerAbilitySystem()
{
	if (CachedAbilitySystemComponent.IsValid())
	{
		RefreshStaminaFromAbilitySystem();
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	APRPlayerState* PlayerState = IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APRPlayerState>() : nullptr;
	if (!IsValid(PlayerState))
	{
		HandleBindRetryTimer();
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = PlayerState->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		HandleBindRetryTimer();
		return;
	}

	BindToAbilitySystem(AbilitySystemComponent);
}

void UPRStaminaBarWidget::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!IsValid(InAbilitySystemComponent))
	{
		return;
	}

	if (CachedAbilitySystemComponent.Get() == InAbilitySystemComponent
		&& StaminaChangedDelegateHandle.IsValid()
		&& MaxStaminaChangedDelegateHandle.IsValid())
	{
		RefreshStaminaFromAbilitySystem();
		return;
	}

	UnbindFromAbilitySystem();

	CachedAbilitySystemComponent = InAbilitySystemComponent;
	StaminaChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Player::GetStaminaAttribute()).AddUObject(this, &UPRStaminaBarWidget::HandleStaminaChanged);
	MaxStaminaChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Player::GetMaxStaminaAttribute()).AddUObject(this, &UPRStaminaBarWidget::HandleMaxStaminaChanged);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	RefreshStaminaFromAbilitySystem();
}

void UPRStaminaBarWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent))
	{
		if (StaminaChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Player::GetStaminaAttribute()).Remove(StaminaChangedDelegateHandle);
		}

		if (MaxStaminaChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Player::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedDelegateHandle);
		}
	}

	StaminaChangedDelegateHandle.Reset();
	MaxStaminaChangedDelegateHandle.Reset();
	CachedAbilitySystemComponent.Reset();
}

void UPRStaminaBarWidget::RefreshStaminaFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	CurrentStamina = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Player::GetStaminaAttribute());
	CurrentMaxStamina = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Player::GetMaxStaminaAttribute());

	const float NewPercent = CurrentMaxStamina > KINDA_SMALL_NUMBER
		? CurrentStamina / CurrentMaxStamina
		: 0.0f;
	SetStaminaPercent(NewPercent);
}

void UPRStaminaBarWidget::ApplyDisplayedPercentToFill()
{
	if (USizeBox* FillSizeBox = GetFillSizeBox())
	{
		FillSizeBox->SetWidthOverride(MaxBarWidth * CurrentPercent);
	}
}

void UPRStaminaBarWidget::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentStamina = ChangeData.NewValue;
	const float NewPercent = CurrentMaxStamina > KINDA_SMALL_NUMBER
		? CurrentStamina / CurrentMaxStamina
		: 0.0f;
	SetStaminaPercent(NewPercent);
}

void UPRStaminaBarWidget::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxStamina = ChangeData.NewValue;
	const float NewPercent = CurrentMaxStamina > KINDA_SMALL_NUMBER
		? CurrentStamina / CurrentMaxStamina
		: 0.0f;
	SetStaminaPercent(NewPercent);
}

void UPRStaminaBarWidget::HandleBindRetryTimer()
{
	if (CurrentBindRetryCount >= MaxBindRetryCount)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(BindRetryTimerHandle))
	{
		return;
	}

	++CurrentBindRetryCount;
	World->GetTimerManager().SetTimer(
		BindRetryTimerHandle,
		this,
		&UPRStaminaBarWidget::TryBindToOwnerAbilitySystem,
		BindRetryInterval,
		false);
}

USizeBox* UPRStaminaBarWidget::GetFillSizeBox() const
{
	if (IsValid(StaminaFillSizeBox))
	{
		return StaminaFillSizeBox;
	}

	if (IsValid(SizeBox_FillClip))
	{
		return SizeBox_FillClip;
	}

	if (IsValid(FillClipBox))
	{
		return FillClipBox;
	}

	return nullptr;
}
