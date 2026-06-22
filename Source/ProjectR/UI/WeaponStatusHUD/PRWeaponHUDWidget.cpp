// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (무기 변경 시 HUD 동기화 구현)
// Author: 이건주 (무기 장착 상태, 탄창 실시간 잔여량 및 Mod 버프 애니메이션 구현)
#include "PRWeaponHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "ProjectR/AbilitySystem/Abilities/Mod/PRGA_Mod_HasDuration.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"
#include "ProjectR/UI/WeaponStatusHUD/PRWeaponStatusWidget.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

void UPRWeaponHUDWidget::InitializeWeaponHUD()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(OwningPlayerController->GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	APRPlayerState* PlayerState = PlayerCharacter->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		return;
	}

	SetWeaponHUDSources(PlayerCharacter->GetWeaponManager(), PlayerState->GetWeaponSet());
}

void UPRWeaponHUDWidget::SetWeaponHUDSources(UPRWeaponManagerComponent* InWeaponManagerComponent, UPRAttributeSet_Weapon* InWeaponSet)
{
	UnbindWeaponHUDSources();

	WeaponManagerComponent = InWeaponManagerComponent;
	WeaponSet = InWeaponSet;

	if (IsValid(PrimaryWeaponStatusWidget))
	{
		PrimaryWeaponStatusWidget->SetSlotType(EPRWeaponSlotType::Primary);
	}

	if (IsValid(SecondaryWeaponStatusWidget))
	{
		SecondaryWeaponStatusWidget->SetSlotType(EPRWeaponSlotType::Secondary);
	}

	if (UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get())
	{
		WeaponManager->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
		WeaponManager->GetOnWeaponEquipmentChanged().AddDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
	}

	if (const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get())
	{
		if (UAbilitySystemComponent* ASC = CurrentWeaponSet->GetOwningAbilitySystemComponent())
		{
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
		}
	}

	RefreshAllWeaponStatus();
	RefreshWeaponSlotHighlight();
}

void UPRWeaponHUDWidget::RefreshAllWeaponStatus()
{
	RefreshWeaponStatus(EPRWeaponSlotType::Primary);
	RefreshWeaponStatus(EPRWeaponSlotType::Secondary);
}

void UPRWeaponHUDWidget::RefreshWeaponStatus(EPRWeaponSlotType SlotType)
{
	const FPRWeaponStatusViewData ViewData = BuildWeaponStatusViewData(SlotType);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		if (IsValid(PrimaryWeaponStatusWidget))
		{
			PrimaryWeaponStatusWidget->SetWeaponStatus(ViewData);
		}
		return;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		if (IsValid(SecondaryWeaponStatusWidget))
		{
			SecondaryWeaponStatusWidget->SetWeaponStatus(ViewData);
		}
	}
}

FPRWeaponStatusViewData UPRWeaponHUDWidget::BuildWeaponStatusViewData(EPRWeaponSlotType SlotType) const
{
	FPRWeaponStatusViewData ViewData;
	ViewData.SlotType = SlotType;

	const UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get();
	const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get();
	if (!IsValid(WeaponManager) || !IsValid(CurrentWeaponSet))
	{
		return ViewData;
	}

	const UPRWeaponDataAsset* WeaponData = WeaponManager->GetWeaponDataBySlotType(SlotType);
	const UPRItemInstance_Weapon* WeaponInstance = WeaponManager->GetWeaponInstanceBySlotType(SlotType);
	const FPRWeaponVisualInfo& VisualInfo = WeaponManager->GetVisualInfoBySlotType(SlotType);
	const UPRWeaponModDataAsset* ModData = VisualInfo.ModData;
	const EPRAmmoType AmmoType = IsValid(WeaponData)
		? WeaponData->GetAmmoType()
		: (SlotType == EPRWeaponSlotType::Primary ? EPRAmmoType::Primary : EPRAmmoType::Secondary);
	const float ModGauge = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryModGauge()
		: CurrentWeaponSet->GetSecondaryModGauge();
	const float MaxModGauge = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryMaxModGauge()
		: CurrentWeaponSet->GetSecondaryMaxModGauge();
	const float ModStack = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryModStack()
		: CurrentWeaponSet->GetSecondaryModStack();

	ViewData.bIsCurrentWeaponSlot = WeaponManager->GetCurrentWeaponSlot() == SlotType;
	ViewData.bHasWeapon = IsValid(WeaponData);
	ViewData.WeaponIcon = IsValid(WeaponData) ? WeaponData->GetIcon() : nullptr;
	ViewData.MagazineAmmo = CurrentWeaponSet->GetMagazineAmmoByType(AmmoType);
	ViewData.ReserveAmmo = CurrentWeaponSet->GetReserveAmmoByType(AmmoType);
	ViewData.bHasMod = IsValid(ModData);
	ViewData.ModIcon = IsValid(ModData) ? ModData->GetIcon() : nullptr;
	ViewData.ModGaugePercent = IsValid(ModData)
		? UPRUserInterfaceStatics::ConvertMinMaxToPercent(ModGauge, MaxModGauge)
		: 0.0f;
	ViewData.ModStackCount = ModStack;
	ViewData.bUsesModDuration = DoesModUseDuration(ModData);
	ViewData.ModRemainingDurationSeconds = (ViewData.bUsesModDuration && IsValid(WeaponInstance))
		? WeaponInstance->GetRemainingModDurationSeconds(ResolveServerWorldTimeSeconds())
		: 0.0f;

	return ViewData;
}

void UPRWeaponHUDWidget::RefreshWeaponSlotHighlight()
{
	if (!ActiveSlotPresentation.bValid || !InactiveSlotPresentation.bValid)
	{
		CacheWeaponSlotPresentations();
	}

	const UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get();
	const EPRWeaponSlotType CurrentWeaponSlot = IsValid(WeaponManager)
		? WeaponManager->GetCurrentWeaponSlot()
		: EPRWeaponSlotType::None;
	UPRWeaponStatusWidget* PrimaryStatusWidget = GetWeaponStatusWidgetBySlot(EPRWeaponSlotType::Primary);
	UPRWeaponStatusWidget* SecondaryStatusWidget = GetWeaponStatusWidgetBySlot(EPRWeaponSlotType::Secondary);
	const bool bHasCachedPresentation = IsValid(PrimaryStatusWidget)
		&& IsValid(SecondaryStatusWidget)
		&& ActiveSlotPresentation.bValid
		&& InactiveSlotPresentation.bValid;

	if (!bHasCachedPresentation)
	{
		return;
	}

	if (CurrentWeaponSlot != EPRWeaponSlotType::Primary && CurrentWeaponSlot != EPRWeaponSlotType::Secondary)
	{
		// 비강조 기본값
		ApplyWeaponSlotPresentation(EPRWeaponSlotType::Primary, ActiveSlotPresentation, InactiveSlotPresentation);
		ApplyWeaponSlotPresentation(EPRWeaponSlotType::Secondary, InactiveSlotPresentation, InactiveSlotPresentation);
		return;
	}

	const EPRWeaponSlotType InactiveWeaponSlot = CurrentWeaponSlot == EPRWeaponSlotType::Primary
		? EPRWeaponSlotType::Secondary
		: EPRWeaponSlotType::Primary;

	// 강조 상태 적용
	ApplyWeaponSlotHighlight(CurrentWeaponSlot, true);
	ApplyWeaponSlotHighlight(InactiveWeaponSlot, false);
}

void UPRWeaponHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheWeaponSlotPresentations();
}

void UPRWeaponHUDWidget::NativeDestruct()
{
	UnbindWeaponHUDSources();
	Super::NativeDestruct();
}

void UPRWeaponHUDWidget::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	if (ChangedWeaponManagerComponent != WeaponManagerComponent.Get())
	{
		return;
	}

	if (ChangedSlot == EPRWeaponSlotType::None)
	{
		RefreshAllWeaponStatus();
		RefreshWeaponSlotHighlight();
		return;
	}

	RefreshAllWeaponStatus();
	RefreshWeaponSlotHighlight();
}

UPRWeaponStatusWidget* UPRWeaponHUDWidget::GetWeaponStatusWidgetBySlot(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponStatusWidget;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponStatusWidget;
	}

	return nullptr;
}

void UPRWeaponHUDWidget::ApplyWeaponSlotHighlight(EPRWeaponSlotType SlotType, bool bHighlighted) const
{
	const FPRWeaponHUDSlotPresentation& Presentation = bHighlighted
		? ActiveSlotPresentation
		: InactiveSlotPresentation;

	ApplyWeaponSlotPresentation(SlotType, Presentation, Presentation);
}

void UPRWeaponHUDWidget::CacheWeaponSlotPresentations()
{
	if (ActiveSlotPresentation.bValid && InactiveSlotPresentation.bValid)
	{
		return;
	}

	FPRWeaponHUDSlotPresentation NewActiveSlotPresentation;
	FPRWeaponHUDSlotPresentation NewInactiveSlotPresentation;
	const bool bCapturedActive = CaptureWeaponSlotPresentation(PrimaryWeaponStatusWidget, NewActiveSlotPresentation);
	const bool bCapturedInactive = CaptureWeaponSlotPresentation(SecondaryWeaponStatusWidget, NewInactiveSlotPresentation);

	if (!bCapturedActive || !bCapturedInactive)
	{
		return;
	}

	// 주무기 위젯 기준 활성값
	ActiveSlotPresentation = NewActiveSlotPresentation;

	// 보조무기 위젯 기준 비활성값
	InactiveSlotPresentation = NewInactiveSlotPresentation;
}

bool UPRWeaponHUDWidget::CaptureWeaponSlotPresentation(UPRWeaponStatusWidget* StatusWidget, FPRWeaponHUDSlotPresentation& OutPresentation) const
{
	if (!IsValid(StatusWidget))
	{
		return false;
	}

	const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(StatusWidget->Slot);
	if (!IsValid(CanvasSlot))
	{
		return false;
	}

	OutPresentation.LayoutData = CanvasSlot->GetLayout();
	OutPresentation.bAutoSize = CanvasSlot->GetAutoSize();
	OutPresentation.ZOrder = CanvasSlot->GetZOrder();
	OutPresentation.RenderScale = StatusWidget->GetRenderTransform().Scale;
	OutPresentation.RenderOpacity = StatusWidget->GetRenderOpacity();
	OutPresentation.bValid = true;
	return true;
}

void UPRWeaponHUDWidget::ApplyWeaponSlotPresentation(EPRWeaponSlotType SlotType, const FPRWeaponHUDSlotPresentation& LayoutPresentation, const FPRWeaponHUDSlotPresentation& RenderPresentation) const
{
	if (!LayoutPresentation.bValid || !RenderPresentation.bValid)
	{
		return;
	}

	UPRWeaponStatusWidget* StatusWidget = GetWeaponStatusWidgetBySlot(SlotType);
	if (!IsValid(StatusWidget))
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(StatusWidget->Slot);
	if (!IsValid(CanvasSlot))
	{
		return;
	}

	// 에디터 설정 적용
	CanvasSlot->SetLayout(LayoutPresentation.LayoutData);
	CanvasSlot->SetAutoSize(LayoutPresentation.bAutoSize);
	CanvasSlot->SetZOrder(LayoutPresentation.ZOrder);
	StatusWidget->SetRenderScale(RenderPresentation.RenderScale);
	StatusWidget->SetRenderOpacity(RenderPresentation.RenderOpacity);
}

bool UPRWeaponHUDWidget::DoesModUseDuration(const UPRWeaponModDataAsset* ModData) const
{
	if (!IsValid(ModData))
	{
		return false;
	}

	for (const FPRAbilityEntry& Entry : ModData->EquippedAbilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		if (Entry.AbilityClass->IsChildOf(UPRGA_Mod_HasDuration::StaticClass()))
		{
			return true;
		}
	}

	return false;
}

float UPRWeaponHUDWidget::ResolveServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshWeaponStatus(EPRWeaponSlotType::Primary);
}

void UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshWeaponStatus(EPRWeaponSlotType::Secondary);
}

void UPRWeaponHUDWidget::UnbindWeaponHUDSources()
{
	if (UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get())
	{
		WeaponManager->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
	}

	if (const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get())
	{
		if (UAbilitySystemComponent* ASC = CurrentWeaponSet->GetOwningAbilitySystemComponent())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute()).RemoveAll(this);
		}
	}

	AttributeChangeHandles.Reset();
	WeaponManagerComponent.Reset();
	WeaponSet.Reset();
}
