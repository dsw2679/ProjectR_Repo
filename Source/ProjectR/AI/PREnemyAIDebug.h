// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy AI Debug 구현)
#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPREnemyAI, Log, All);

namespace PREnemyAIDebug
{
	// 전투 이동 표현 적용/해제 로그 활성 여부
	bool IsPresentationLogEnabled();

	// attack_pressure 증가/리셋 로그 활성 여부
	bool IsAttackPressureLogEnabled();

	// 패턴 선택/이동 의도 판단 로그 활성 여부
	bool IsPatternLogEnabled();

	// 타겟 후보/현재 타겟/공격 커밋 상태 로그 활성 여부
	bool IsTargetingLogEnabled();

	// 타겟 후보와 선택 상태 화면 디버그 활성 여부
	bool IsTargetingDrawEnabled();

	// 타겟 화면 디버그가 유지되는 시간
	float GetTargetingDrawDuration();
}
