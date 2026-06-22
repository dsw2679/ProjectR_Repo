// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Mod 피해량 연산 및 대미지 팝업 연동 구현)
// Author: 손승우 (적 AI 상태별 Mod 피해 보정 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRDamageExecCalcBase.h"
#include "PRDamageExecCalc_FromPlayerMod.generated.h"

// 플레이어 모드 스킬 발신 데미지 ExecCalc.
UCLASS()
class PROJECTR_API UPRDamageExecCalc_FromPlayerMod : public UPRDamageExecCalcBase
{
	GENERATED_BODY()

public:
	UPRDamageExecCalc_FromPlayerMod();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
