// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy AI Debug 구현)
#include "PREnemyAIDebug.h"

#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogPREnemyAI);

namespace
{
	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugPresentation(
		TEXT("pr.EnemyAI.DebugPresentation"),
		0,
		TEXT("전투 이동 표현 적용/해제 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugAttackPressure(
		TEXT("pr.EnemyAI.DebugAttackPressure"),
		0,
		TEXT("attack_pressure 증감/리셋 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugPattern(
		TEXT("pr.EnemyAI.DebugPattern"),
		0,
		TEXT("패턴 선택/전투 이동 의도 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugTargeting(
		TEXT("pr.EnemyAI.DebugTargeting"),
		0,
		TEXT("타겟 후보/현재 타겟/공격 커밋 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugTargetDraw(
		TEXT("pr.EnemyAI.DebugTargetDraw"),
		0,
		TEXT("타겟 후보/현재 타겟/공격 커밋 화면 디버그를 표시한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarPREnemyAIDebugTargetDrawDuration(
		TEXT("pr.EnemyAI.DebugTargetDrawDuration"),
		0.25f,
		TEXT("타겟 화면 디버그 유지 시간이다."),
		ECVF_Default);
}

bool PREnemyAIDebug::IsPresentationLogEnabled()
{
	return CVarPREnemyAIDebugPresentation.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsAttackPressureLogEnabled()
{
	return CVarPREnemyAIDebugAttackPressure.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsPatternLogEnabled()
{
	return CVarPREnemyAIDebugPattern.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsTargetingLogEnabled()
{
	return CVarPREnemyAIDebugTargeting.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsTargetingDrawEnabled()
{
	return CVarPREnemyAIDebugTargetDraw.GetValueOnAnyThread() > 0;
}

float PREnemyAIDebug::GetTargetingDrawDuration()
{
	return FMath::Max(CVarPREnemyAIDebugTargetDrawDuration.GetValueOnAnyThread(), 0.0f);
}
