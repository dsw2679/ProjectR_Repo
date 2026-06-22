// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (기본 재장전 연산 및 좌수 IK 보정 로직 구현)
// Author: 이건주 (Mod 버프에 따른 재장전 가중치 연산 구현)
#include "PRReloadExecCalc.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"

struct FReloadCaptureDefs
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryMaxMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryReserveAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryMaxMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryReserveAmmo);

	FReloadCaptureDefs()
	{
		// Source 캡처. 재장전 GE는 ASC가 자기 자신에게 적용한다
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryReserveAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryReserveAmmo, Source, false);
	}
};

static const FReloadCaptureDefs& GetReloadCaptureDefs()
{
	static FReloadCaptureDefs Defs;
	return Defs;
}

UPRReloadExecCalc::UPRReloadExecCalc()
{
	const FReloadCaptureDefs& Defs = GetReloadCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.PrimaryMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.PrimaryMaxMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.PrimaryReserveAmmoDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryMaxMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryReserveAmmoDef);
}

void UPRReloadExecCalc::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FReloadCaptureDefs& Defs = GetReloadCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;

	const bool bIsPrimary = (TargetAmmoType == EPRAmmoType::Primary);

	const FGameplayEffectAttributeCaptureDefinition& MagDef = bIsPrimary ? Defs.PrimaryMagazineAmmoDef : Defs.SecondaryMagazineAmmoDef;
	const FGameplayEffectAttributeCaptureDefinition& MaxMagDef = bIsPrimary ? Defs.PrimaryMaxMagazineAmmoDef : Defs.SecondaryMaxMagazineAmmoDef;
	const FGameplayEffectAttributeCaptureDefinition& ReserveDef = bIsPrimary ? Defs.PrimaryReserveAmmoDef : Defs.SecondaryReserveAmmoDef;

	float Magazine = 0.f;
	float MaxMagazine = 0.f;
	float Reserve = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MagDef, EvalParams, Magazine);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MaxMagDef, EvalParams, MaxMagazine);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ReserveDef, EvalParams, Reserve);

	const float NeededAmmo = FMath::Max(MaxMagazine - Magazine, 0.0f);
	const float TransferAmmo = FMath::Min(NeededAmmo, FMath::Max(Reserve, 0.0f));
	if (TransferAmmo <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 한 실행에서 두 어트리뷰트를 동시에 갱신: 탄창 += / 예비탄 -=
	const FGameplayAttribute MagAttr = bIsPrimary
		? UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
		: UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute();
	const FGameplayAttribute ReserveAttr = bIsPrimary
		? UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
		: UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute();

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		MagAttr, EGameplayModOp::Additive, TransferAmmo));
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		ReserveAttr, EGameplayModOp::Additive, -TransferAmmo));
}
