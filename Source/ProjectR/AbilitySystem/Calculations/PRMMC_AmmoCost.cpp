// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (무기별 기본 탄약 소모 공식 구현)
// Author: 이건주 (Mod 버프 활성화 시 탄약 소모량 감소 계산식 구현)
#include "PRMMC_AmmoCost.h"

#include "ProjectR/Combat/PRCombatGameplayTags.h"

UPRMMC_AmmoCost::UPRMMC_AmmoCost()
{
}

float UPRMMC_AmmoCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 미지정 시 단발 1.0으로 처리한다
	const float AmmoCost = Spec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoCost, false, 1.0f);
	return FMath::Max(AmmoCost, 0.0f);
}
