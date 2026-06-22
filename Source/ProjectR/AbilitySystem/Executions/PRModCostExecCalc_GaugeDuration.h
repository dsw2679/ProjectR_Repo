// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (Mod Cost Exec Calc Gauge Duration 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PRModCostExecCalc_GaugeDuration.generated.h"

// 지속시간형 Mod 게이지 비용을 주기마다 계산한다
UCLASS()
class PROJECTR_API UPRModCostExecCalc_GaugeDuration : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPRModCostExecCalc_GaugeDuration();

	/*~ UGameplayEffectExecutionCalculation Interface ~*/
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
