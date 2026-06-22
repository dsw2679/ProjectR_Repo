// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (Mod Cost Exec Calc Gauge Duration 구현)
#include "PRModCostExecCalc_GaugeDuration.h"

#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	struct FModGaugeDurationCostCaptureDefs
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryModGauge);
		DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryMaxModGauge);
		DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryModGauge);
		DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryMaxModGauge);

		FModGaugeDurationCostCaptureDefs()
		{
			// 지속시간형 Mod 비용 GE는 자기 자신에게 적용하므로 Source에서 슬롯 게이지를 캡처한다
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryModGauge, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryMaxModGauge, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryModGauge, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryMaxModGauge, Source, false);
		}
	};

	const FModGaugeDurationCostCaptureDefs& GetModGaugeDurationCostCaptureDefs()
	{
		static FModGaugeDurationCostCaptureDefs Defs;
		return Defs;
	}
}

UPRModCostExecCalc_GaugeDuration::UPRModCostExecCalc_GaugeDuration()
{
	const FModGaugeDurationCostCaptureDefs& Defs = GetModGaugeDurationCostCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.PrimaryModGaugeDef);
	RelevantAttributesToCapture.Add(Defs.PrimaryMaxModGaugeDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryModGaugeDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryMaxModGaugeDef);
}

void UPRModCostExecCalc_GaugeDuration::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer& DynamicGrantedTags = Spec.DynamicGrantedTags;
	const bool bIsPrimarySlot = DynamicGrantedTags.HasTagExact(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	const bool bIsSecondarySlot = DynamicGrantedTags.HasTagExact(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	if (bIsPrimarySlot == bIsSecondarySlot)
	{
		return;
	}

	const float Duration = Spec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModDuration, false, 0.0f);
	const float Period = Spec.GetPeriod();
	if (Duration <= KINDA_SMALL_NUMBER || Period <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FModGaugeDurationCostCaptureDefs& Defs = GetModGaugeDurationCostCaptureDefs();
	const FGameplayEffectAttributeCaptureDefinition& GaugeDef = bIsPrimarySlot
		? Defs.PrimaryModGaugeDef
		: Defs.SecondaryModGaugeDef;
	const FGameplayEffectAttributeCaptureDefinition& MaxGaugeDef = bIsPrimarySlot
		? Defs.PrimaryMaxModGaugeDef
		: Defs.SecondaryMaxModGaugeDef;

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float CurrentGauge = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GaugeDef, EvalParams, CurrentGauge);
	if (CurrentGauge <= 0.0f)
	{
		return;
	}

	float MaxGauge = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MaxGaugeDef, EvalParams, MaxGauge);
	if (MaxGauge <= 0.0f)
	{
		return;
	}

	const float TickCost = MaxGauge * Period / Duration;
	const float AppliedCost = FMath::Min(TickCost, CurrentGauge);
	if (AppliedCost <= 0.0f)
	{
		return;
	}

	const FGameplayAttribute GaugeAttribute = bIsPrimarySlot
		? UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()
		: UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute();

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		GaugeAttribute, EGameplayModOp::Additive, -AppliedCost));
}
