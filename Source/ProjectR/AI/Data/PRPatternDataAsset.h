// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피격 경직 및 조준 반응 조건 연동)
// Author: 손승우 (아머드 솔저 및 일반 몬스터 공격 패턴 데이터 설계)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRPatternDataAsset.generated.h"

// 몬스터가 거리/시야 조건에 따라 어떤 Ability를 실행할지 정의하는 데이터 에셋이다.
// BTTask_PRSelectEnemyPattern이 이 에셋을 읽어 Blackboard에 selected_ability_tag를 기록한다.
UCLASS(BlueprintType)
class PROJECTR_API UPRPatternDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// 패턴 후보 목록이다. 조건을 통과한 후보 중 SelectionWeight 비율로 하나가 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	TArray<FPRPatternRule> PatternRules;
};
