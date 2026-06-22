// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (무기별 기본 탄약 소모 공식 구현)
// Author: 이건주 (Mod 버프 활성화 시 탄약 소모량 감소 계산식 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "PRMMC_AmmoCost.generated.h"

// 사격 GE에서 탄약 차감량을 산출하는 MMC
// 결과는 SetByCaller.AmmoCost 값이며 기본값은 1발이다
// GE 측 Modifier Op은 Subtract로 사용하거나 Coefficient를 -1로 두어 차감 방향을 결정
UCLASS()
class PROJECTR_API UPRMMC_AmmoCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UPRMMC_AmmoCost();

	/*~ UGameplayModMagnitudeCalculation Interface ~*/
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
