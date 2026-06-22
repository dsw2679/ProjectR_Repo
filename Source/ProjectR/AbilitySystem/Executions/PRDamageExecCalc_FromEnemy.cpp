// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피격 Poise 경직 및 다운/사망 임계치 연동 계산식 구현)
// Author: 배유찬 (피해량 산정 및 대미지 팝업 연동 계산식 구현)
// Author: 손승우 (적 공격 속성별 피해량 보정 계산식 구현)
#include "PRDamageExecCalc_FromEnemy.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

struct FFromEnemyCaptureDefs
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);

	FFromEnemyCaptureDefs()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, AttackPower, Source, false);
	}
};

static const FFromEnemyCaptureDefs& GetFromEnemyCaptureDefs()
{
	static FFromEnemyCaptureDefs Defs;
	return Defs;
}

UPRDamageExecCalc_FromEnemy::UPRDamageExecCalc_FromEnemy()
{
	const FFromEnemyCaptureDefs& Defs = GetFromEnemyCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.HealthDef);
	RelevantAttributesToCapture.Add(Defs.MaxHealthDef);
	RelevantAttributesToCapture.Add(Defs.ArmorDef);
	RelevantAttributesToCapture.Add(Defs.AttackPowerDef);
}

void UPRDamageExecCalc_FromEnemy::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	if (TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Invulnerable))
	{
		return;
	}

	if (IsValid(TargetASC->GetSet<UPRAttributeSet_Enemy>()))
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FFromEnemyCaptureDefs& Defs = GetFromEnemyCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;

	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	float TargetArmor = 0.0f;
	float AttackPower = 0.0f;
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.HealthDef, EvalParams, CurrentHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.MaxHealthDef, EvalParams, MaxHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.ArmorDef, EvalParams, TargetArmor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.AttackPowerDef, EvalParams, AttackPower);
	
	AActor* SourceActor = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC->GetAvatarActor();
	
	FPRDamageInputs Inputs;
	Inputs.bIsFromPlayer = false;
	Inputs.bIsFromFriendly = UPRCombatStatics::IsFriendly(SourceActor, TargetActor);
	Inputs.TargetArmor = TargetArmor;
	
	const float DamageMultiplier = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AttackMultiplier, false, 1.0f);
	if (AttackPower > 0.0f)
	{
		// 모든 적/보스 발신 공격은 StatRow AttackPower에 공격별 배율만 곱한다.
		Inputs.BaseDamage = AttackPower * DamageMultiplier;
	}

	const FHitResult EmptyHitResult;
	const FHitResult* HitResult = OwningSpec.GetContext().GetHitResult();
	const FHitResult& EffectiveHitResult = HitResult != nullptr ? *HitResult : EmptyHitResult;

	const FPRDamageOutputs Outputs = UPRCombatStatics::ComputeDamage(Inputs, EffectiveHitResult, TargetActor);

	if (Outputs.FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Common::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage));

		if (IsValid(TargetASC->GetSet<UPRAttributeSet_Player>()))
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				UPRAttributeSet_Player::GetIncomingRecoverableDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage * 0.5f));
		}
	}

	const bool bHasSetByCallerPoiseDamage = OwningSpec.SetByCallerTagMagnitudes.Contains(PRCombatGameplayTags::SetByCaller_PoiseDamage);
	const UPRAttributeSet_Player* PlayerSet = TargetASC->GetSet<UPRAttributeSet_Player>();
	if (bHasSetByCallerPoiseDamage && IsValid(PlayerSet))
	{
		// 플레이어 강인도 피해는 공격 데이터의 고정값을 그대로 사용한다.
		const float PoiseDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_PoiseDamage, false, 0.0f);
		const float FinalPoiseDamage = FMath::Max(PoiseDamage, 0.0f);
		if (FinalPoiseDamage > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				UPRAttributeSet_Player::GetIncomingPoiseDamageAttribute(), EGameplayModOp::Additive, FinalPoiseDamage));
		}
	}
	
	// 사망한 경우 회복 가능 체력을 0으로 override
	float HealthAfterDamage = FMath::Clamp(CurrentHealth - Outputs.FinalDamage, 0.0f, MaxHealth);
	if (FMath::IsNearlyZero(HealthAfterDamage))
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Player::GetRecoverableHealthAttribute(),EGameplayModOp::Override,0.0f));
	}

	DispatchPostDamageApplied(TargetActor, OwningSpec, Outputs, EffectiveHitResult, CurrentHealth, MaxHealth);
}
