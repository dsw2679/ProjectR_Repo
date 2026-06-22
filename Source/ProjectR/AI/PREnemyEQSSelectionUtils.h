// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy EQS Selection Utils 정적 유틸리티 함수 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"

class UEnvQuery;

namespace PREnemyEQSSelectionUtils
{
	// EQS를 즉시 실행하고 설정된 후보 선택 방식에 따라 위치 결과를 반환한다.
	bool RunLocationQuery(
		UObject* QueryOwner,
		UEnvQuery* QueryTemplate,
		const TArray<FPREnemyEQSFloatParam>& FloatParams,
		EEnvQueryRunMode::Type QueryRunMode,
		EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		int32 TopCandidateCount,
		float TopScoreCandidateRatio,
		FVector& OutLocation);

	// 후보 선택 방식에 맞춰 EQS 실행 방식을 보정한다.
	EEnvQueryRunMode::Type ResolveQueryRunMode(
		EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		EEnvQueryRunMode::Type RequestedRunMode);

	// EQS 결과에서 설정된 후보 선택 방식에 맞는 ItemIndex를 고른다.
	bool SelectItemIndex(
		const FEnvQueryResult& QueryResult,
		EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		int32 TopCandidateCount,
		float TopScoreCandidateRatio,
		int32& OutItemIndex);
}
