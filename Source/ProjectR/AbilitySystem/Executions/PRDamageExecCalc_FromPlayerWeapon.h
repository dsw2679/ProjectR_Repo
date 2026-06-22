// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (특성 성장 수치 기반 피해 보정 연동 구현)
// Author: 배유찬 (무기 피해량 연산, Mod 게이지 획득 및 대미지 팝업 연동)
// Author: 손승우 (적 AI 상태별 무기 피해 보정 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRDamageExecCalcBase.h"
#include "PRDamageExecCalc_FromPlayerWeapon.generated.h"

// 플레이어 총기 데미지 ExecCalc.
UCLASS()
class PROJECTR_API UPRDamageExecCalc_FromPlayerWeapon : public UPRDamageExecCalcBase
{
	GENERATED_BODY()

public:
	UPRDamageExecCalc_FromPlayerWeapon();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
