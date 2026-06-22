// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (성장 특성 Noti Text UI 위젯 구현)
#include "PRTraitNotiTextWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Growth.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "PRTraitNotiTextWidget"

UPRTraitNotiTextWidget::UPRTraitNotiTextWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
	MessageFormat = LOCTEXT("TraitPointNotificationFormat", "특성 포인트 {0}개를 사용할 수 있습니다");
}

void UPRTraitNotiTextWidget::RefreshTraitPointFromOwner()
{
	TryBindToOwnerAbilitySystem();
	RefreshTraitPointFromAbilitySystem();
}

void UPRTraitNotiTextWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentBindRetryCount = 0;
	ApplyTraitPoint(0);
	TryBindToOwnerAbilitySystem();
}

void UPRTraitNotiTextWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	UnbindFromAbilitySystem();

	Super::NativeDestruct();
}

void UPRTraitNotiTextWidget::TryBindToOwnerAbilitySystem()
{
	if (CachedAbilitySystemComponent.IsValid())
	{
		RefreshTraitPointFromAbilitySystem();
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

void UPRTraitNotiTextWidget::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!IsValid(InAbilitySystemComponent))
	{
		return;
	}

	if (CachedAbilitySystemComponent.Get() == InAbilitySystemComponent
		&& TraitPointChangedDelegateHandle.IsValid())
	{
		RefreshTraitPointFromAbilitySystem();
		return;
	}

	UnbindFromAbilitySystem();

	CachedAbilitySystemComponent = InAbilitySystemComponent;
	TraitPointChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Growth::GetTraitPointAttribute()).AddUObject(this, &ThisClass::HandleTraitPointChanged);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	RefreshTraitPointFromAbilitySystem();
}

void UPRTraitNotiTextWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent) && TraitPointChangedDelegateHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UPRAttributeSet_Growth::GetTraitPointAttribute()).Remove(TraitPointChangedDelegateHandle);
	}

	TraitPointChangedDelegateHandle.Reset();
	CachedAbilitySystemComponent.Reset();
}

void UPRTraitNotiTextWidget::RefreshTraitPointFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	const int32 TraitPoint = FMath::Max(
		FMath::RoundToInt(AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Growth::GetTraitPointAttribute())),
		0);
	ApplyTraitPoint(TraitPoint);
}

void UPRTraitNotiTextWidget::ApplyTraitPoint(int32 NewTraitPoint)
{
	CurrentTraitPoint = FMath::Max(NewTraitPoint, 0);
	const bool bVisible = CurrentTraitPoint > 0;

	if (IsValid(TraitNotiText))
	{
		TraitNotiText->SetText(FText::Format(MessageFormat, FText::AsNumber(CurrentTraitPoint)));
	}

	if (IsValid(TraitNotiPanel))
	{
		TraitNotiPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_OnTraitPointNotificationChanged(bVisible, CurrentTraitPoint);
}

void UPRTraitNotiTextWidget::HandleTraitPointChanged(const FOnAttributeChangeData& ChangeData)
{
	ApplyTraitPoint(FMath::RoundToInt(ChangeData.NewValue));
}

void UPRTraitNotiTextWidget::HandleBindRetryTimer()
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
		&ThisClass::TryBindToOwnerAbilitySystem,
		BindRetryInterval,
		false);
}

#undef LOCTEXT_NAMESPACE
