// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (특성 성장 수치 기반 피해 보정 연동 구현)
// Author: 배유찬 (무기 피해량 연산, Mod 게이지 획득 및 대미지 팝업 연동)
// Author: 손승우 (적 AI 상태별 무기 피해 보정 구현)
#include "PRDamageExecCalc_FromPlayerWeapon.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

struct FFromPlayerCaptureDefs
{
	// Target(피격 대상) 캡처 - Common AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	// Target(피격 대상) 캡처 - Enemy AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(GroggyGauge);

	// Source(공격자) 캡처 - Weapon AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(BaseDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(WeakpointMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(GroggyDamageMultiplier);

	// Source(공격자) 캡처 - Player AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PlayerAttackPower);

	FFromPlayerCaptureDefs()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, GroggyGauge, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, BaseDamage, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, DamageMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, ArmorPenetration, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, WeakpointMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, GroggyDamageMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Player, CriticalHitChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Player, CriticalDamageMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Player, PlayerAttackPower, Source, false);
	}
};

static const FFromPlayerCaptureDefs& GetFromPlayerCaptureDefs()
{
	static FFromPlayerCaptureDefs Defs;
	return Defs;
}

UPRDamageExecCalc_FromPlayerWeapon::UPRDamageExecCalc_FromPlayerWeapon()
{
	const FFromPlayerCaptureDefs& Defs = GetFromPlayerCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.HealthDef);
	RelevantAttributesToCapture.Add(Defs.MaxHealthDef);
	RelevantAttributesToCapture.Add(Defs.ArmorDef);
	RelevantAttributesToCapture.Add(Defs.GroggyGaugeDef);
	RelevantAttributesToCapture.Add(Defs.BaseDamageDef);
	RelevantAttributesToCapture.Add(Defs.DamageMultiplierDef);
	RelevantAttributesToCapture.Add(Defs.ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(Defs.WeakpointMultiplierDef);
	RelevantAttributesToCapture.Add(Defs.GroggyDamageMultiplierDef);
	RelevantAttributesToCapture.Add(Defs.CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(Defs.CriticalDamageMultiplierDef);
	RelevantAttributesToCapture.Add(Defs.PlayerAttackPowerDef);
}

void UPRDamageExecCalc_FromPlayerWeapon::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		// Target ASC가 없는 경우는 비정상 호출. 로그 없이 조용히 종료
		return;
	}

	// 사망/무적 상태 차단
	// Dead 상태에서 추가 데미지가 들어가면 OnDeath가 중복 발행될 수 있고,
	// Invulnerable 상태(회피 i-frame, 컷씬 등)에서는 모든 피해/그로기 누적이 무시되어야 한다.
	if (TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Invulnerable))
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer& DynamicAssetTags = OwningSpec.GetDynamicAssetTags();
	
	// HitResult는 GE Context에 포함되어 있으면 사용. 없으면 빈 HitResult로 진행하여 Region이 None으로 처리됨
	const FHitResult* HitResult = OwningSpec.GetContext().GetHitResult();
	const FHitResult EffectiveHitResult = HitResult != nullptr ? *HitResult : FHitResult();
	
	// FAggregatorEvaluateParameters는 Source/Target Tag 컨텍스트를 담을 수 있다.
	const FFromPlayerCaptureDefs& Defs = GetFromPlayerCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;
	
	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	float CurrentGroggyGauge = 0.0f;
	float TargetArmor = 0.0f;
	float WeaponBaseDamage = 0.0f;
	float WeaponDamageMultiplier = 1.0f;
	float ArmorPenetration = 0.f;
	float WeakpointMultiplier = 1.0f;
	float WeaponGroggyDamageMultiplier = 1.0f;
	float CriticalHitChance = 0.0f;
	float CriticalDamageMultiplier = 1.0f;
	float PlayerAttackPower = 0.0f;
	
	// AttemptCalculateCapturedAttributeMagnitude는 캡처 실패 시 false를 반환하지만
	// 이 ExecCalc에서는 캡처 실패해도 기본값으로 진행한다(예: friendly fire에서 Enemy AttributeSet 부재 시 GroggyGauge=0)
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.HealthDef, EvalParams, CurrentHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.MaxHealthDef, EvalParams, MaxHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.ArmorDef, EvalParams, TargetArmor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.GroggyGaugeDef, EvalParams, CurrentGroggyGauge);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.BaseDamageDef, EvalParams, WeaponBaseDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.DamageMultiplierDef, EvalParams, WeaponDamageMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.ArmorPenetrationDef, EvalParams, ArmorPenetration);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.WeakpointMultiplierDef, EvalParams, WeakpointMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.GroggyDamageMultiplierDef, EvalParams, WeaponGroggyDamageMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.CriticalHitChanceDef, EvalParams, CriticalHitChance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.CriticalDamageMultiplierDef, EvalParams, CriticalDamageMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.PlayerAttackPowerDef, EvalParams, PlayerAttackPower);

	const float SpecWeaponBaseDamage = OwningSpec.GetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage,
		false,
		-1.0f);
	if (SpecWeaponBaseDamage >= 0.0f)
	{
		WeaponBaseDamage = SpecWeaponBaseDamage;
	}
	
	AActor* SourceActor = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC->GetAvatarActor();
	
	FPRDamageInputs Inputs;
	Inputs.bIsFromPlayer = true;
	Inputs.bIsFromFriendly = UPRCombatStatics::IsFriendly(SourceActor, TargetActor);
	Inputs.TargetArmor = TargetArmor;
	Inputs.ArmorPenetration = ArmorPenetration;
	Inputs.WeakpointMultiplier = WeakpointMultiplier;
	Inputs.CriticalHitChance = CriticalHitChance;
	Inputs.CriticalDamageMultiplier = CriticalDamageMultiplier;
	if (WeaponDamageMultiplier > 0.0f)
	{
		Inputs.BaseDamage = FMath::Max(WeaponBaseDamage + PlayerAttackPower, 0.0f) * WeaponDamageMultiplier;
	}
	if (Inputs.BaseDamage > 0.0f)
	{
		Inputs.GroggyDamage = UPRCombatStatics::CalculateBaseGroggyDamage(Inputs.BaseDamage) * WeaponGroggyDamageMultiplier;
	}

	// ComputeDamage 내부에서 IPRCombatInterface::GetDamageRegionInfo로 부위 위치를 조회하고,
	// 무기 배율과 우호 감쇠를 합성하여 최종 데미지/그로기 데미지를 산출
	const FPRDamageOutputs Outputs = UPRCombatStatics::ComputeDamage(Inputs, EffectiveHitResult, TargetActor);

	/*~ Output Modifier 작성 ~*/
	if (Outputs.FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Common::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage));
	}

	// 그로기 게이지는 적 대상에서만 출력한다.
	// CurrentGroggyGauge가 0인 경우는 (1) Enemy AttributeSet 부재 또는 (2) 이미 그로기 진입 상태.
	// 두 경우 모두 출력 모디파이어가 의미 없으므로 스킵
	if (Outputs.GroggyDamage > 0.0f && CurrentGroggyGauge > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Enemy::GetGroggyGaugeAttribute(), EGameplayModOp::Additive, -Outputs.GroggyDamage));
	}

	/*~ 수신자 후처리 ~*/
	DispatchPostDamageApplied(TargetActor, OwningSpec, Outputs, EffectiveHitResult, CurrentHealth, MaxHealth);
}
