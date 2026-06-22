// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Enemy World Health Bar UI 위젯 구현)
#include "ProjectR/UI/HUD/PREnemyWorldHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"

UPREnemyWorldHealthBarWidget::UPREnemyWorldHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPREnemyWorldHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyDisplayedPercentsToFill();
}

void UPREnemyWorldHealthBarWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	Super::NativeDestruct();
}

void UPREnemyWorldHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

void UPREnemyWorldHealthBarWidget::InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	BindToAbilitySystem(InAbilitySystemComponent);
}

void UPREnemyWorldHealthBarWidget::ClearHealthSource()
{
	UnbindFromAbilitySystem();

	CurrentHealth = 0.0f;
	CurrentMaxHealth = 0.0f;
	HealthPercent = 0.0f;
	DelayedHealthPercent = 0.0f;
	bIsDelayedLayerActive = false;

	BroadcastHealthChanged();
}

void UPREnemyWorldHealthBarWidget::CompleteDelayedLayer()
{
	DelayedStartPercent = HealthPercent;
	DelayedHealthPercent = HealthPercent;
	DelayedElapsed = 0.0f;
	bIsDelayedLayerActive = false;

	BroadcastHealthChanged();
}

void UPREnemyWorldHealthBarWidget::ApplyHealthChange(float InOldHealth, float InNewHealth, float InMaxHealth)
{
	CurrentHealth = InNewHealth;
	CurrentMaxHealth = InMaxHealth;

	if (InNewHealth < InOldHealth)
	{
		const float DamageStartPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
			? FMath::Clamp(InOldHealth / CurrentMaxHealth, 0.0f, 1.0f)
			: HealthPercent;
		StartDelayedLayer(FMath::Max(DelayedHealthPercent, DamageStartPercent));
		RefreshHealthPercents(false);
		return;
	}

	RefreshHealthPercents(true);
}

/*~ Binding ~*/

void UPREnemyWorldHealthBarWidget::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!IsValid(InAbilitySystemComponent))
	{
		ClearHealthSource();
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
		UPRAttributeSet_Common::GetHealthAttribute()).AddUObject(this, &UPREnemyWorldHealthBarWidget::HandleHealthChanged);
	MaxHealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetMaxHealthAttribute()).AddUObject(this, &UPREnemyWorldHealthBarWidget::HandleMaxHealthChanged);

	RefreshHealthFromAbilitySystem(true);
}

void UPREnemyWorldHealthBarWidget::UnbindFromAbilitySystem()
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

void UPREnemyWorldHealthBarWidget::RefreshHealthFromAbilitySystem(bool bForceDelayedPercent)
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

void UPREnemyWorldHealthBarWidget::RefreshHealthPercents(bool bForceDelayedPercent)
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

void UPREnemyWorldHealthBarWidget::StartDelayedLayer(float InStartPercent)
{
	DelayedStartPercent = FMath::Clamp(InStartPercent, 0.0f, 1.0f);
	DelayedHealthPercent = DelayedStartPercent;
	DelayedElapsed = 0.0f;
	bIsDelayedLayerActive = true;
}

void UPREnemyWorldHealthBarWidget::BroadcastHealthChanged()
{
	ApplyDisplayedPercentsToFill();
	BP_OnEnemyHealthChanged(HealthPercent, DelayedHealthPercent, CurrentHealth, CurrentMaxHealth);
}

void UPREnemyWorldHealthBarWidget::ApplyDisplayedPercentsToFill()
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

void UPREnemyWorldHealthBarWidget::ApplyBarLayoutWidths()
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

float UPREnemyWorldHealthBarWidget::GetBackBorderWidth() const
{
	return MaxBarWidth;
}

float UPREnemyWorldHealthBarWidget::GetFillAreaWidth() const
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

float UPREnemyWorldHealthBarWidget::GetMaxFillWidth() const
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

void UPREnemyWorldHealthBarWidget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	ApplyHealthChange(ChangeData.OldValue, ChangeData.NewValue, CurrentMaxHealth);
}

void UPREnemyWorldHealthBarWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxHealth = ChangeData.NewValue;
	RefreshHealthPercents(true);
}
