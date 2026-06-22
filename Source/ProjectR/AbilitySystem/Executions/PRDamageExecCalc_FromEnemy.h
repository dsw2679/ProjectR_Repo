// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피격 Poise 경직 및 다운/사망 임계치 연동 계산식 구현)
// Author: 배유찬 (피해량 산정 및 대미지 팝업 연동 계산식 구현)
// Author: 손승우 (적 공격 속성별 피해량 보정 계산식 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRDamageExecCalcBase.h"
#include "PRDamageExecCalc_FromEnemy.generated.h"

// 적 발신 데미지 ExecCalc.
UCLASS()
class PROJECTR_API UPRDamageExecCalc_FromEnemy : public UPRDamageExecCalcBase
{
	GENERATED_BODY()

public:
	UPRDamageExecCalc_FromEnemy();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
