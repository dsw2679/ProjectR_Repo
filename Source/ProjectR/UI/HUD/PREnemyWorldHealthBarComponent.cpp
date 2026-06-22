// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (UI Enemy World Health Bar 컴포넌트 구현)
#include "ProjectR/UI/HUD/PREnemyWorldHealthBarComponent.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/UI/HUD/PREnemyWorldHealthBarWidget.h"
#include "TimerManager.h"

UPREnemyWorldHealthBarComponent::UPREnemyWorldHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(false);
	SetDrawSize(FVector2D(140.0f, 16.0f));
	SetRelativeLocation(HeadOffset);
	SetVisibility(false);
}

/*~ UActorComponent Interface ~*/

void UPREnemyWorldHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	SetRelativeLocation(HeadOffset);
	InitializeUserWidget();
}

void UPREnemyWorldHealthBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	UnbindFromAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

/*~ Public API ~*/

void UPREnemyWorldHealthBarComponent::InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	BindToAbilitySystem(InAbilitySystemComponent);
}

void UPREnemyWorldHealthBarComponent::ShowForDuration()
{
	if (bSuppressedVisibility)
	{
		HideHealthBar();
		return;
	}
	
	if (CurrentHealth <= 0.0f)
	{
		HideHealthBar();
		return;
	}
	
	// 체력 초기화 과정인 경우
	if (LastMaxHealth + 0.001f < CurrentMaxHealth)
	{
		LastMaxHealth = CurrentMaxHealth;
		HideHealthBar();
		return;
	}
	if (LastCurrentHealth + 0.001f < CurrentHealth)
	{
		LastCurrentHealth = CurrentHealth;
		HideHealthBar();
		return;
	}
	

	UpdateHealthBarVisibility(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		if (VisibleDurationAfterHit > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				HideTimerHandle,
				this,
				&UPREnemyWorldHealthBarComponent::HandleHideTimer,
				VisibleDurationAfterHit,
				false);
		}
	}
}

void UPREnemyWorldHealthBarComponent::HideHealthBar()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	if (UPREnemyWorldHealthBarWidget* HealthBarWidget = Cast<UPREnemyWorldHealthBarWidget>(GetUserWidgetObject()))
	{
		HealthBarWidget->CompleteDelayedLayer();
	}

	UpdateHealthBarVisibility(false);
}

/*~ Binding ~*/

void UPREnemyWorldHealthBarComponent::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!IsValid(InAbilitySystemComponent))
	{
		UnbindFromAbilitySystem();
		return;
	}

	if (CachedAbilitySystemComponent.Get() == InAbilitySystemComponent
		&& HealthChangedDelegateHandle.IsValid()
		&& MaxHealthChangedDelegateHandle.IsValid())
	{
		InitializeUserWidget();
		return;
	}

	UnbindFromAbilitySystem();

	CachedAbilitySystemComponent = InAbilitySystemComponent;
	HealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetHealthAttribute()).AddUObject(this, &UPREnemyWorldHealthBarComponent::HandleHealthChanged);
	MaxHealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetMaxHealthAttribute()).AddUObject(this, &UPREnemyWorldHealthBarComponent::HandleMaxHealthChanged);

	CurrentHealth = InAbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	CurrentMaxHealth = InAbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());

	InitializeUserWidget();

	if (bHideAtFullHealth || CurrentHealth <= 0.0f)
	{
		HideHealthBar();
	}
}

void UPREnemyWorldHealthBarComponent::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent))
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Common::GetHealthAttribute()).Remove(HealthChangedDelegateHandle);
		}

		if (MaxHealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Common::GetMaxHealthAttribute()).Remove(MaxHealthChangedDelegateHandle);
		}
	}

	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
	CachedAbilitySystemComponent.Reset();
}

void UPREnemyWorldHealthBarComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentHealth = ChangeData.NewValue;
	LastCurrentHealth = CurrentHealth;
	
	if (CurrentHealth <= 0.0f)
	{
		HideHealthBar();
		return;
	}

	if (ChangeData.NewValue < ChangeData.OldValue)
	{
		ShowForDuration();

		if (UPREnemyWorldHealthBarWidget* HealthBarWidget = Cast<UPREnemyWorldHealthBarWidget>(GetUserWidgetObject()))
		{
			HealthBarWidget->ApplyHealthChange(ChangeData.OldValue, ChangeData.NewValue, CurrentMaxHealth);
		}
	}
}

void UPREnemyWorldHealthBarComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxHealth = ChangeData.NewValue;
	LastMaxHealth = CurrentMaxHealth;
}

void UPREnemyWorldHealthBarComponent::HandleHideTimer()
{
	if (CurrentHealth > 0.0f)
	{
		if (UPREnemyWorldHealthBarWidget* HealthBarWidget = Cast<UPREnemyWorldHealthBarWidget>(GetUserWidgetObject()))
		{
			HealthBarWidget->CompleteDelayedLayer();
		}

		UpdateHealthBarVisibility(false);
	}
}

void UPREnemyWorldHealthBarComponent::UpdateHealthBarVisibility(bool bInVisible)
{
	SetVisibility(bInVisible);

	if (UUserWidget* UserWidget = GetUserWidgetObject())
	{
		UserWidget->SetVisibility(bInVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPREnemyWorldHealthBarComponent::InitializeUserWidget()
{
	InitWidget();

	UPREnemyWorldHealthBarWidget* HealthBarWidget = Cast<UPREnemyWorldHealthBarWidget>(GetUserWidgetObject());
	if (!IsValid(HealthBarWidget))
	{
		return;
	}

	HealthBarWidget->InitializeFromAbilitySystem(CachedAbilitySystemComponent.Get());
}
