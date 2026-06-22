// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Boss 전투 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRBossCombatDataAsset.generated.h"

// 보스 몬스터가 공통으로 사용하는 전투 데이터 자산이다.
// 일반 적의 pressure/approach/fast 대신 패턴 fallback 이동과 표현 설정만 가진다.
UCLASS(BlueprintType)
class PROJECTR_API UPRBossCombatDataAsset : public UPRCombatMoveDataAsset
{
	GENERATED_BODY()

public:
	// 실행 가능한 보스 패턴이 없을 때 사용하는 보조 재배치 쿼리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig PatternFallbackRepositionConfig;
};
