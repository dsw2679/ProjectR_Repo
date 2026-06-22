// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 조준 대응용 회피/돌진 패턴 데이터 정의)
// Author: 손승우 (해머 연계 공격 및 EQS 전투 이동 패턴 데이터 구축)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "PRSoldierArmoredPatternDataAsset.generated.h"

// Soldier_Armored 전용 패턴 DataAsset 타입
// 실제 패턴 후보, 거리, 가중치는 에디터 작성 기준
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredPatternDataAsset : public UPRPatternDataAsset
{
	GENERATED_BODY()

public:
	UPRSoldierArmoredPatternDataAsset();
};
