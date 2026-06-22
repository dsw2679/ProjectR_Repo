// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Boss Health Bar UI 위젯 구현)
#include "ProjectR/UI/HUD/PRBossHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

UPRBossHealthBarWidget::UPRBossHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyDisplayedPercentsToFill();
	SetBossHealthVisible(CachedBoss.IsValid());
}

void UPRBossHealthBarWidget::NativeDestruct()
{
	ClearBoss();
	Super::NativeDestruct();
}

void UPRBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsDelayedLayerActive)
	{
		return;
	}

	DelayedElapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(DelayedElapsed / DelayedLayerDuration, 0.0f, 1.0f);
	DelayedHealthPercent = FMath::Lerp(DelayedStartPercent, HealthPercent, Alpha);
	if (Alpha >= 1.0f)
	{
		DelayedHealthPercent = HealthPercent;
		bIsDelayedLayerActive = false;
	}

	BroadcastHealthChanged();
}

/*~ Public API ~*/

void UPRBossHealthBarWidget::BindBoss(APRBossBaseCharacter* InBoss)
{
	if (!IsValid(InBoss))
	{
		ClearBoss();
		return;
	}

	CachedBoss = InBoss;
	BindToAbilitySystem(InBoss->GetAbilitySystemComponent());

	TArray<float> MarkerPercents;
	InBoss->GetPhaseThresholdRatios(MarkerPercents);
	BP_OnBossNameChanged(InBoss->GetBossDisplayName());
	BP_OnBossPhaseMarkersChanged(MarkerPercents);
	SetBossHealthVisible(true);
}

void UPRBossHealthBarWidget::ClearBoss()
{
	UnbindFromAbilitySystem();
	CachedBoss.Reset();

	CurrentHealth = 0.0f;
	CurrentMaxHealth = 0.0f;
	HealthPercent = 0.0f;
	DelayedHealthPercent = 0.0f;
	bIsDelayedLayerActive = false;

	BroadcastHealthChanged();
	BP_OnBossPhaseMarkersChanged(TArray<float>());
	SetBossHealthVisible(false);
}

/*~ Binding ~*/

void UPRBossHealthBarWidget::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
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
		RefreshHealthFromAbilitySystem(true);
		return;
	}

	UnbindFromAbilitySystem();

	CachedAbilitySystemComponent = InAbilitySystemComponent;
	HealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetHealthAttribute()).AddUObject(this, &UPRBossHealthBarWidget::HandleHealthChanged);
	MaxHealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetMaxHealthAttribute()).AddUObject(this, &UPRBossHealthBarWidget::HandleMaxHealthChanged);

	RefreshHealthFromAbilitySystem(true);
}

void UPRBossHealthBarWidget::UnbindFromAbilitySystem()
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

void UPRBossHealthBarWidget::RefreshHealthFromAbilitySystem(bool bForceDelayedPercent)
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	CurrentMaxHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	RefreshHealthPercents(bForceDelayedPercent);
}

void UPRBossHealthBarWidget::RefreshHealthPercents(bool bForceDelayedPercent)
{
	HealthPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentHealth / CurrentMaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (bForceDelayedPercent)
	{
		DelayedHealthPercent = HealthPercent;
		bIsDelayedLayerActive = false;
	}
	else if (!bIsDelayedLayerActive)
	{
		DelayedHealthPercent = HealthPercent;
	}

	BroadcastHealthChanged();
}

void UPRBossHealthBarWidget::StartDelayedLayer(float InStartPercent)
{
	DelayedStartPercent = FMath::Clamp(InStartPercent, 0.0f, 1.0f);
	DelayedHealthPercent = DelayedStartPercent;
	DelayedElapsed = 0.0f;
	bIsDelayedLayerActive = true;
}

void UPRBossHealthBarWidget::BroadcastHealthChanged()
{
	ApplyDisplayedPercentsToFill();
	BP_OnBossHealthChanged(HealthPercent, DelayedHealthPercent, CurrentHealth, CurrentMaxHealth);
}

void UPRBossHealthBarWidget::ApplyDisplayedPercentsToFill()
{
	ApplyBarLayoutWidths();

	const float MaxFillWidth = GetMaxFillWidth();
	if (IsValid(HealthFillSizeBox))
	{
		HealthFillSizeBox->SetWidthOverride(MaxFillWidth * HealthPercent);
	}

	if (IsValid(DelayedFillSizeBox))
	{
		DelayedFillSizeBox->SetWidthOverride(MaxFillWidth * DelayedHealthPercent);
	}

	InvalidateLayoutAndVolatility();
}

void UPRBossHealthBarWidget::ApplyBarLayoutWidths()
{
	if (IsValid(BackBorderSizeBox))
	{
		BackBorderSizeBox->SetWidthOverride(GetBackBorderWidth());
	}

	if (IsValid(FillAreaSizeBox))
	{
		FillAreaSizeBox->SetWidthOverride(GetFillAreaWidth());
	}
}

float UPRBossHealthBarWidget::GetBackBorderWidth() const
{
	return MaxBarWidth;
}

float UPRBossHealthBarWidget::GetFillAreaWidth() const
{
	const float BackBorderWidth = GetBackBorderWidth();
	if (!IsValid(FillAreaSizeBox))
	{
		return BackBorderWidth;
	}

	float HorizontalPadding = 0.0f;
	if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(FillAreaSizeBox->Slot))
	{
		const FMargin SlotPadding = OverlaySlot->GetPadding();
		HorizontalPadding = SlotPadding.Left + SlotPadding.Right;
	}

	const float FillAreaWidth = BackBorderWidth - HorizontalPadding;
	return FillAreaWidth > 0.0f ? FillAreaWidth : BackBorderWidth;
}

float UPRBossHealthBarWidget::GetMaxFillWidth() const
{
	if (IsValid(FillAreaSizeBox) && FillAreaSizeBox->IsWidthOverride())
	{
		const float FillWidth = FillAreaSizeBox->GetWidthOverride();
		if (FillWidth > 0.0f)
		{
			return FillWidth;
		}
	}

	return GetFillAreaWidth();
}

void UPRBossHealthBarWidget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentHealth = ChangeData.NewValue;

	if (ChangeData.NewValue < ChangeData.OldValue)
	{
		const float DamageStartPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
			? FMath::Clamp(ChangeData.OldValue / CurrentMaxHealth, 0.0f, 1.0f)
			: HealthPercent;
		StartDelayedLayer(FMath::Max(DelayedHealthPercent, DamageStartPercent));
		RefreshHealthPercents(false);
	}
	else
	{
		RefreshHealthPercents(true);
	}
}

void UPRBossHealthBarWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxHealth = ChangeData.NewValue;
	RefreshHealthPercents(true);
}

void UPRBossHealthBarWidget::SetBossHealthVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_OnBossHealthVisibilityChanged(bVisible);
}
