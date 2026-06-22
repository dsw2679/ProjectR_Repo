// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (능력치 특성 투자 버튼 및 스탯 변경 정보 구현)
// Author: 배유찬 (특성창 UI 스택 입력 연동)
#include "PRTraitWindowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Growth.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"

UPRTraitWindowWidget::UPRTraitWindowWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRTraitWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindChildWidgetEvents();
	BindSourceEvents();
	RefreshTraitWindowView();
}

void UPRTraitWindowWidget::NativeDestruct()
{
	UnbindChildWidgetEvents();
	UnbindSourceEvents();
	UnbindGrowthComponent();

	Super::NativeDestruct();
}

void UPRTraitWindowWidget::SetGrowthSource(APRPlayerState* InPlayerState)
{
	UnbindGrowthComponent();

	BoundPlayerState = InPlayerState;
	BoundGrowthComponent = IsValid(InPlayerState) ? InPlayerState->GetGrowthComponent() : nullptr;
	if (UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get())
	{
		GrowthComponent->OnTraitInvestmentChanged.AddDynamic(this, &ThisClass::HandleTraitInvestmentChanged);
	}

	RefreshPreviewFromGrowth();
}

void UPRTraitWindowWidget::RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestConfirmTraitInvestment(DesiredInvestment);
}

void UPRTraitWindowWidget::RequestConfirmPreviewInvestment()
{
	RequestConfirmTraitInvestment(PreviewInvestmentInfo);
}

void UPRTraitWindowWidget::RequestResetTraitInvestment()
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestResetTraitInvestment();
}

void UPRTraitWindowWidget::RefreshPreviewFromGrowth()
{
	if (const UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get())
	{
		CommittedInvestmentInfo = GrowthComponent->GetTraitInvestmentInfo();
	}
	else
	{
		CommittedInvestmentInfo = FPRTraitInvestmentInfo();
	}

	PreviewInvestmentInfo = CommittedInvestmentInfo;
	RefreshTraitWindowView();
}

bool UPRTraitWindowWidget::IncreaseTraitPoint(EPRTraitStatType TraitType, int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	const FPRTraitPreviewInfo PreviewInfo = GetTraitPreviewInfo(TraitType);
	const int32 IncreaseAmount = FMath::Min(Amount, PreviewInfo.AddablePoint);
	if (IncreaseAmount <= 0)
	{
		return false;
	}

	SetInvestmentPoint(PreviewInvestmentInfo, TraitType, PreviewInfo.PreviewPoint + IncreaseAmount);
	if (IsValid(ResultText))
	{
		ResultText->SetText(FText::GetEmpty());
	}
	RefreshTraitWindowView();
	return true;
}

bool UPRTraitWindowWidget::DecreaseTraitPoint(EPRTraitStatType TraitType, int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	const FPRTraitPreviewInfo PreviewInfo = GetTraitPreviewInfo(TraitType);
	const int32 DecreaseAmount = FMath::Min(Amount, PreviewInfo.RemovablePoint);
	if (DecreaseAmount <= 0)
	{
		return false;
	}

	SetInvestmentPoint(PreviewInvestmentInfo, TraitType, PreviewInfo.PreviewPoint - DecreaseAmount);
	if (IsValid(ResultText))
	{
		ResultText->SetText(FText::GetEmpty());
	}
	RefreshTraitWindowView();
	return true;
}

bool UPRTraitWindowWidget::IncreaseTraitPointToAvailableMax(EPRTraitStatType TraitType)
{
	const FPRTraitPreviewInfo PreviewInfo = GetTraitPreviewInfo(TraitType);
	return IncreaseTraitPoint(TraitType, PreviewInfo.AddablePoint);
}

bool UPRTraitWindowWidget::DecreaseTraitPointToCommitted(EPRTraitStatType TraitType)
{
	const FPRTraitPreviewInfo PreviewInfo = GetTraitPreviewInfo(TraitType);
	return DecreaseTraitPoint(TraitType, PreviewInfo.RemovablePoint);
}

int32 UPRTraitWindowWidget::GetPreviewRemainingPoint() const
{
	return FMath::Max(GetTotalEarnedPoint() - GetTotalSpentPoint(PreviewInvestmentInfo), 0);
}

int32 UPRTraitWindowWidget::GetPreviewSpentPoint() const
{
	return GetTotalSpentPoint(PreviewInvestmentInfo);
}

FPRTraitPreviewInfo UPRTraitWindowWidget::GetTraitPreviewInfo(EPRTraitStatType TraitType) const
{
	FPRTraitPreviewInfo PreviewInfo;
	PreviewInfo.TraitType = TraitType;
	PreviewInfo.CommittedPoint = GetInvestmentPoint(CommittedInvestmentInfo, TraitType);
	PreviewInfo.PreviewPoint = GetInvestmentPoint(PreviewInvestmentInfo, TraitType);
	PreviewInfo.PendingPoint = PreviewInfo.PreviewPoint - PreviewInfo.CommittedPoint;
	PreviewInfo.MaxPoint = GetTraitMaxPoint(TraitType);
	PreviewInfo.AddablePoint = FMath::Min(
		FMath::Max(PreviewInfo.MaxPoint - PreviewInfo.PreviewPoint, 0),
		GetPreviewRemainingPoint());
	PreviewInfo.RemovablePoint = FMath::Max(PreviewInfo.PreviewPoint - PreviewInfo.CommittedPoint, 0);

	const float ValuePerPoint = GetTraitValuePerPoint(TraitType);
	PreviewInfo.CommittedBonusValue = static_cast<float>(PreviewInfo.CommittedPoint) * ValuePerPoint;
	PreviewInfo.PreviewBonusValue = static_cast<float>(PreviewInfo.PreviewPoint) * ValuePerPoint;
	PreviewInfo.DeltaBonusValue = PreviewInfo.PreviewBonusValue - PreviewInfo.CommittedBonusValue;
	return PreviewInfo;
}

TArray<FPRTraitPreviewInfo> UPRTraitWindowWidget::GetAllTraitPreviewInfos() const
{
	TArray<FPRTraitPreviewInfo> PreviewInfos;
	for (int32 TraitIndex = static_cast<int32>(EPRTraitStatType::MaxHealth);
		TraitIndex <= static_cast<int32>(EPRTraitStatType::CriticalDamageMultiplier);
		++TraitIndex)
	{
		PreviewInfos.Add(GetTraitPreviewInfo(static_cast<EPRTraitStatType>(TraitIndex)));
	}
	return PreviewInfos;
}

void UPRTraitWindowWidget::RefreshTraitWindowView()
{
	const int32 TotalEarnedPoint = GetTotalEarnedPoint();
	const int32 PreviewSpentPoint = GetPreviewSpentPoint();
	const int32 RemainingPoint = GetPreviewRemainingPoint();
	const int32 CommittedSpentPoint = GetTotalSpentPoint(CommittedInvestmentInfo);
	const bool bHasPendingChange = PreviewSpentPoint != CommittedSpentPoint;

	if (IsValid(RemainingPointText))
	{
		RemainingPointText->SetText(FText::AsNumber(RemainingPoint));
	}

	if (IsValid(LevelText))
	{
		LevelText->SetText(FText::AsNumber(GetCurrentLevel()));
	}

	if (IsValid(SpentPointText))
	{
		SpentPointText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), PreviewSpentPoint, TotalEarnedPoint)));
	}

	if (IsValid(ConfirmButton))
	{
		ConfirmButton->SetIsEnabled(bHasPendingChange);
	}

	if (IsValid(ResetButton))
	{
		ResetButton->SetIsEnabled(CommittedSpentPoint > 0);
	}

	RefreshTraitRow(EPRTraitStatType::MaxHealth, MaxHealthPointText, MaxHealthValueText, MaxHealthDeltaText,
		MaxHealthIncreaseButton, MaxHealthDecreaseButton, MaxHealthIncreaseMaxButton, MaxHealthDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::Armor, ArmorPointText, ArmorValueText, ArmorDeltaText,
		ArmorIncreaseButton, ArmorDecreaseButton, ArmorIncreaseMaxButton, ArmorDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::MovementSpeed, MovementSpeedPointText, MovementSpeedValueText, MovementSpeedDeltaText,
		MovementSpeedIncreaseButton, MovementSpeedDecreaseButton, MovementSpeedIncreaseMaxButton, MovementSpeedDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::AttackPower, AttackPowerPointText, AttackPowerValueText, AttackPowerDeltaText,
		AttackPowerIncreaseButton, AttackPowerDecreaseButton, AttackPowerIncreaseMaxButton, AttackPowerDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::MaxStamina, MaxStaminaPointText, MaxStaminaValueText, MaxStaminaDeltaText,
		MaxStaminaIncreaseButton, MaxStaminaDecreaseButton, MaxStaminaIncreaseMaxButton, MaxStaminaDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::CriticalHitChance, CriticalHitChancePointText, CriticalHitChanceValueText, CriticalHitChanceDeltaText,
		CriticalHitChanceIncreaseButton, CriticalHitChanceDecreaseButton, CriticalHitChanceIncreaseMaxButton, CriticalHitChanceDecreaseMaxButton);
	RefreshTraitRow(EPRTraitStatType::CriticalDamageMultiplier, CriticalDamageMultiplierPointText, CriticalDamageMultiplierValueText, CriticalDamageMultiplierDeltaText,
		CriticalDamageMultiplierIncreaseButton, CriticalDamageMultiplierDecreaseButton, CriticalDamageMultiplierIncreaseMaxButton, CriticalDamageMultiplierDecreaseMaxButton);

	OnTraitPreviewChanged.Broadcast();
}

void UPRTraitWindowWidget::HandleTraitInvestmentChanged(const FPRTraitInvestmentInfo& InvestmentInfo)
{
	CommittedInvestmentInfo = InvestmentInfo;
	PreviewInvestmentInfo = InvestmentInfo;
	RefreshTraitWindowView();
}

void UPRTraitWindowWidget::HandleTraitInvestmentResult(const FPRTraitInvestmentResult& Result)
{
	if (Result.bSuccess)
	{
		CommittedInvestmentInfo = Result.InvestmentInfo;
		PreviewInvestmentInfo = Result.InvestmentInfo;
		if (IsValid(ResultText))
		{
			ResultText->SetText(FText::FromString(TEXT("특성 적용 완료")));
		}
		RefreshTraitWindowView();
		return;
	}

	if (IsValid(ResultText))
	{
		ResultText->SetText(MakeTraitInvestmentFailReasonText(Result.FailReason));
	}
}

void UPRTraitWindowWidget::BindChildWidgetEvents()
{
	UnbindChildWidgetEvents();

#define PR_BIND_TRAIT_BUTTON(ButtonName, HandlerName) \
	if (IsValid(ButtonName)) \
	{ \
		ButtonName->OnClicked.AddDynamic(this, &ThisClass::HandlerName); \
	}

	PR_BIND_TRAIT_BUTTON(MaxHealthIncreaseButton, HandleMaxHealthIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxHealthDecreaseButton, HandleMaxHealthDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxHealthIncreaseMaxButton, HandleMaxHealthIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxHealthDecreaseMaxButton, HandleMaxHealthDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(ArmorIncreaseButton, HandleArmorIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(ArmorDecreaseButton, HandleArmorDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(ArmorIncreaseMaxButton, HandleArmorIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(ArmorDecreaseMaxButton, HandleArmorDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(MovementSpeedIncreaseButton, HandleMovementSpeedIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MovementSpeedDecreaseButton, HandleMovementSpeedDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MovementSpeedIncreaseMaxButton, HandleMovementSpeedIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(MovementSpeedDecreaseMaxButton, HandleMovementSpeedDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(AttackPowerIncreaseButton, HandleAttackPowerIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(AttackPowerDecreaseButton, HandleAttackPowerDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(AttackPowerIncreaseMaxButton, HandleAttackPowerIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(AttackPowerDecreaseMaxButton, HandleAttackPowerDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxStaminaIncreaseButton, HandleMaxStaminaIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxStaminaDecreaseButton, HandleMaxStaminaDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxStaminaIncreaseMaxButton, HandleMaxStaminaIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(MaxStaminaDecreaseMaxButton, HandleMaxStaminaDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalHitChanceIncreaseButton, HandleCriticalHitChanceIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalHitChanceDecreaseButton, HandleCriticalHitChanceDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalHitChanceIncreaseMaxButton, HandleCriticalHitChanceIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalHitChanceDecreaseMaxButton, HandleCriticalHitChanceDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalDamageMultiplierIncreaseButton, HandleCriticalDamageMultiplierIncreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalDamageMultiplierDecreaseButton, HandleCriticalDamageMultiplierDecreaseButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalDamageMultiplierIncreaseMaxButton, HandleCriticalDamageMultiplierIncreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(CriticalDamageMultiplierDecreaseMaxButton, HandleCriticalDamageMultiplierDecreaseMaxButtonClicked)
	PR_BIND_TRAIT_BUTTON(ConfirmButton, HandleConfirmButtonClicked)
	PR_BIND_TRAIT_BUTTON(ResetButton, HandleResetButtonClicked)

#undef PR_BIND_TRAIT_BUTTON
}

void UPRTraitWindowWidget::UnbindChildWidgetEvents()
{
#define PR_UNBIND_TRAIT_BUTTON(ButtonName, HandlerName) \
	if (IsValid(ButtonName)) \
	{ \
		ButtonName->OnClicked.RemoveDynamic(this, &ThisClass::HandlerName); \
	}

	PR_UNBIND_TRAIT_BUTTON(MaxHealthIncreaseButton, HandleMaxHealthIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxHealthDecreaseButton, HandleMaxHealthDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxHealthIncreaseMaxButton, HandleMaxHealthIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxHealthDecreaseMaxButton, HandleMaxHealthDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ArmorIncreaseButton, HandleArmorIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ArmorDecreaseButton, HandleArmorDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ArmorIncreaseMaxButton, HandleArmorIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ArmorDecreaseMaxButton, HandleArmorDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MovementSpeedIncreaseButton, HandleMovementSpeedIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MovementSpeedDecreaseButton, HandleMovementSpeedDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MovementSpeedIncreaseMaxButton, HandleMovementSpeedIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MovementSpeedDecreaseMaxButton, HandleMovementSpeedDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(AttackPowerIncreaseButton, HandleAttackPowerIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(AttackPowerDecreaseButton, HandleAttackPowerDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(AttackPowerIncreaseMaxButton, HandleAttackPowerIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(AttackPowerDecreaseMaxButton, HandleAttackPowerDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxStaminaIncreaseButton, HandleMaxStaminaIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxStaminaDecreaseButton, HandleMaxStaminaDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxStaminaIncreaseMaxButton, HandleMaxStaminaIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(MaxStaminaDecreaseMaxButton, HandleMaxStaminaDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalHitChanceIncreaseButton, HandleCriticalHitChanceIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalHitChanceDecreaseButton, HandleCriticalHitChanceDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalHitChanceIncreaseMaxButton, HandleCriticalHitChanceIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalHitChanceDecreaseMaxButton, HandleCriticalHitChanceDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalDamageMultiplierIncreaseButton, HandleCriticalDamageMultiplierIncreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalDamageMultiplierDecreaseButton, HandleCriticalDamageMultiplierDecreaseButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalDamageMultiplierIncreaseMaxButton, HandleCriticalDamageMultiplierIncreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(CriticalDamageMultiplierDecreaseMaxButton, HandleCriticalDamageMultiplierDecreaseMaxButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ConfirmButton, HandleConfirmButtonClicked)
	PR_UNBIND_TRAIT_BUTTON(ResetButton, HandleResetButtonClicked)

#undef PR_UNBIND_TRAIT_BUTTON
}

void UPRTraitWindowWidget::BindSourceEvents()
{
	if (APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>())
	{
		PlayerController->OnTraitInvestmentResult.RemoveDynamic(this, &ThisClass::HandleTraitInvestmentResult);
		PlayerController->OnTraitInvestmentResult.AddDynamic(this, &ThisClass::HandleTraitInvestmentResult);
	}
}

void UPRTraitWindowWidget::UnbindSourceEvents()
{
	if (APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>())
	{
		PlayerController->OnTraitInvestmentResult.RemoveDynamic(this, &ThisClass::HandleTraitInvestmentResult);
	}
}

void UPRTraitWindowWidget::RefreshTraitRow(EPRTraitStatType TraitType, UTextBlock* PointText, UTextBlock* ValueText, UTextBlock* DeltaText,
	UButton* IncreaseButton, UButton* DecreaseButton, UButton* IncreaseMaxButton, UButton* DecreaseMaxButton)
{
	const FPRTraitPreviewInfo PreviewInfo = GetTraitPreviewInfo(TraitType);
	if (IsValid(PointText))
	{
		PointText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d (%+d)"),
			PreviewInfo.PreviewPoint,
			PreviewInfo.MaxPoint,
			PreviewInfo.PendingPoint)));
	}

	if (IsValid(ValueText))
	{
		ValueText->SetText(FText::FromString(FString::Printf(
			TEXT("%s -> %s"),
			*MakeTraitValueText(TraitType, PreviewInfo.CommittedBonusValue).ToString(),
			*MakeTraitValueText(TraitType, PreviewInfo.PreviewBonusValue).ToString())));
	}

	if (IsValid(DeltaText))
	{
		DeltaText->SetText(MakeTraitDeltaText(TraitType, PreviewInfo.DeltaBonusValue));
	}

	if (IsValid(IncreaseButton))
	{
		IncreaseButton->SetIsEnabled(PreviewInfo.AddablePoint > 0);
	}

	if (IsValid(DecreaseButton))
	{
		DecreaseButton->SetIsEnabled(PreviewInfo.RemovablePoint > 0);
	}

	if (IsValid(IncreaseMaxButton))
	{
		IncreaseMaxButton->SetIsEnabled(PreviewInfo.AddablePoint > 0);
	}

	if (IsValid(DecreaseMaxButton))
	{
		DecreaseMaxButton->SetIsEnabled(PreviewInfo.RemovablePoint > 0);
	}
}

FText UPRTraitWindowWidget::MakeTraitValueText(EPRTraitStatType TraitType, float Value) const
{
	if (TraitType == EPRTraitStatType::MovementSpeed
		|| TraitType == EPRTraitStatType::CriticalHitChance
		|| TraitType == EPRTraitStatType::CriticalDamageMultiplier)
	{
		return FText::FromString(FString::Printf(TEXT("+%.1f%%"), Value * 100.0f));
	}

	return FText::FromString(FString::Printf(TEXT("+%.1f"), Value));
}

FText UPRTraitWindowWidget::MakeTraitDeltaText(EPRTraitStatType TraitType, float Value) const
{
	if (TraitType == EPRTraitStatType::MovementSpeed
		|| TraitType == EPRTraitStatType::CriticalHitChance
		|| TraitType == EPRTraitStatType::CriticalDamageMultiplier)
	{
		return FText::FromString(FString::Printf(TEXT("%+.1f%%"), Value * 100.0f));
	}

	return FText::FromString(FString::Printf(TEXT("%+.1f"), Value));
}

FText UPRTraitWindowWidget::MakeTraitInvestmentFailReasonText(EPRTraitInvestmentFailReason FailReason) const
{
	switch (FailReason)
	{
	case EPRTraitInvestmentFailReason::InvalidPlayerState:
		return FText::FromString(TEXT("플레이어 상태가 유효하지 않습니다."));
	case EPRTraitInvestmentFailReason::InvalidAbilitySystem:
		return FText::FromString(TEXT("AbilitySystem이 유효하지 않습니다."));
	case EPRTraitInvestmentFailReason::InvalidGrowthComponent:
		return FText::FromString(TEXT("성장 컴포넌트가 유효하지 않습니다."));
	case EPRTraitInvestmentFailReason::InvalidDataTable:
		return FText::FromString(TEXT("성장 데이터 테이블이 유효하지 않습니다."));
	case EPRTraitInvestmentFailReason::NegativePoint:
		return FText::FromString(TEXT("투자 포인트는 음수가 될 수 없습니다."));
	case EPRTraitInvestmentFailReason::ExceedTraitMaxPoint:
		return FText::FromString(TEXT("특성 최대 투자치를 초과했습니다."));
	case EPRTraitInvestmentFailReason::ExceedTotalPoint:
		return FText::FromString(TEXT("보유 특성 포인트를 초과했습니다."));
	case EPRTraitInvestmentFailReason::InvalidEffect:
		return FText::FromString(TEXT("특성 효과가 유효하지 않습니다."));
	case EPRTraitInvestmentFailReason::None:
	default:
		return FText::FromString(TEXT("특성 적용에 실패했습니다."));
	}
}

#define PR_DEFINE_TRAIT_BUTTON_HANDLERS(Name, TraitTypeValue) \
	void UPRTraitWindowWidget::Handle##Name##IncreaseButtonClicked() \
	{ \
		IncreaseTraitPoint(TraitTypeValue, 1); \
	} \
	void UPRTraitWindowWidget::Handle##Name##DecreaseButtonClicked() \
	{ \
		DecreaseTraitPoint(TraitTypeValue, 1); \
	} \
	void UPRTraitWindowWidget::Handle##Name##IncreaseMaxButtonClicked() \
	{ \
		IncreaseTraitPointToAvailableMax(TraitTypeValue); \
	} \
	void UPRTraitWindowWidget::Handle##Name##DecreaseMaxButtonClicked() \
	{ \
		DecreaseTraitPointToCommitted(TraitTypeValue); \
	}

PR_DEFINE_TRAIT_BUTTON_HANDLERS(MaxHealth, EPRTraitStatType::MaxHealth)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(Armor, EPRTraitStatType::Armor)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(MovementSpeed, EPRTraitStatType::MovementSpeed)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(AttackPower, EPRTraitStatType::AttackPower)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(MaxStamina, EPRTraitStatType::MaxStamina)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(CriticalHitChance, EPRTraitStatType::CriticalHitChance)
PR_DEFINE_TRAIT_BUTTON_HANDLERS(CriticalDamageMultiplier, EPRTraitStatType::CriticalDamageMultiplier)

#undef PR_DEFINE_TRAIT_BUTTON_HANDLERS

void UPRTraitWindowWidget::HandleConfirmButtonClicked()
{
	RequestConfirmPreviewInvestment();
}

void UPRTraitWindowWidget::HandleResetButtonClicked()
{
	RequestResetTraitInvestment();
}

void UPRTraitWindowWidget::UnbindGrowthComponent()
{
	if (UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get())
	{
		GrowthComponent->OnTraitInvestmentChanged.RemoveDynamic(this, &ThisClass::HandleTraitInvestmentChanged);
	}
}

int32 UPRTraitWindowWidget::GetCurrentLevel() const
{
	const APRPlayerState* PlayerState = BoundPlayerState.Get();
	const UPRAbilitySystemComponent* ASC = IsValid(PlayerState) ? PlayerState->GetPRAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC))
	{
		return 1;
	}

	return FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1);
}

int32 UPRTraitWindowWidget::GetTotalEarnedPoint() const
{
	const APRPlayerState* PlayerState = BoundPlayerState.Get();
	const UPRAbilitySystemComponent* ASC = IsValid(PlayerState) ? PlayerState->GetPRAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC))
	{
		return GetTotalSpentPoint(CommittedInvestmentInfo);
	}

	const int32 TraitPoint = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetTraitPointAttribute())), 0);
	const int32 SpentPoint = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetSpentPointAttribute())), 0);
	return TraitPoint + SpentPoint;
}

int32 UPRTraitWindowWidget::GetInvestmentPoint(const FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType) const
{
	switch (TraitType)
	{
	case EPRTraitStatType::MaxHealth:
		return InvestmentInfo.MaxHealth;
	case EPRTraitStatType::Armor:
		return InvestmentInfo.Armor;
	case EPRTraitStatType::MovementSpeed:
		return InvestmentInfo.MovementSpeed;
	case EPRTraitStatType::AttackPower:
		return InvestmentInfo.AttackPower;
	case EPRTraitStatType::MaxStamina:
		return InvestmentInfo.MaxStamina;
	case EPRTraitStatType::CriticalHitChance:
		return InvestmentInfo.CriticalHitChance;
	case EPRTraitStatType::CriticalDamageMultiplier:
		return InvestmentInfo.CriticalDamageMultiplier;
	default:
		return 0;
	}
}

void UPRTraitWindowWidget::SetInvestmentPoint(FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType, int32 Point) const
{
	switch (TraitType)
	{
	case EPRTraitStatType::MaxHealth:
		InvestmentInfo.MaxHealth = Point;
		break;
	case EPRTraitStatType::Armor:
		InvestmentInfo.Armor = Point;
		break;
	case EPRTraitStatType::MovementSpeed:
		InvestmentInfo.MovementSpeed = Point;
		break;
	case EPRTraitStatType::AttackPower:
		InvestmentInfo.AttackPower = Point;
		break;
	case EPRTraitStatType::MaxStamina:
		InvestmentInfo.MaxStamina = Point;
		break;
	case EPRTraitStatType::CriticalHitChance:
		InvestmentInfo.CriticalHitChance = Point;
		break;
	case EPRTraitStatType::CriticalDamageMultiplier:
		InvestmentInfo.CriticalDamageMultiplier = Point;
		break;
	default:
		break;
	}
}

int32 UPRTraitWindowWidget::GetTotalSpentPoint(const FPRTraitInvestmentInfo& InvestmentInfo) const
{
	return InvestmentInfo.MaxHealth
		+ InvestmentInfo.Armor
		+ InvestmentInfo.MovementSpeed
		+ InvestmentInfo.AttackPower
		+ InvestmentInfo.MaxStamina
		+ InvestmentInfo.CriticalHitChance
		+ InvestmentInfo.CriticalDamageMultiplier;
}

int32 UPRTraitWindowWidget::GetTraitMaxPoint(EPRTraitStatType TraitType) const
{
	const UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get();
	FPRTraitStatRuleRow Rule;
	return IsValid(GrowthComponent) && GrowthComponent->GetTraitRule(TraitType, Rule) ? FMath::Max(Rule.MaxPoint, 0) : 0;
}

float UPRTraitWindowWidget::GetTraitValuePerPoint(EPRTraitStatType TraitType) const
{
	const UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get();
	FPRTraitStatRuleRow Rule;
	return IsValid(GrowthComponent) && GrowthComponent->GetTraitRule(TraitType, Rule) ? Rule.ValuePerPoint : 0.0f;
}
