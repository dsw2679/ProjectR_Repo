// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Damage Exec Calc 기본 구조 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PRDamageExecCalcBase.generated.h"

struct FPRDamageOutputs;

// 데미지 ExecCalc 공통 베이스. 발신자별 ExecCalc가 공유하는 후처리/유틸 로직 제공.
UCLASS(Abstract)
class PROJECTR_API UPRDamageExecCalcBase : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	// 데미지 산출 결과를 IPRCombatInterface 수신자에게 전달. 컨텍스트 필드 일괄 구성 후 OnPostDamageApplied 호출
	static void DispatchPostDamageApplied(
		AActor* TargetActor,
		const FGameplayEffectSpec& OwningSpec,
		const FPRDamageOutputs& Outputs,
		const FHitResult& HitResult,
		float CurrentHealth,
		float MaxHealth);
};
