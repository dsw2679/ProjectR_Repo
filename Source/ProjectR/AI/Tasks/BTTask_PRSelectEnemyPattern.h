// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 여부 연동 패턴 선택 확률 보정 구현)
// Author: 손승우 (보스 및 일반 몬스터의 거리 기반 패턴 선택 알고리즘 구현)
// Author: 이건주 (Penitent 몬스터 패턴 선택 분기 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRSelectEnemyPattern.generated.h"

// PatternDataAsset의 조건/가중치를 평가해서 이번에 실행할 AbilityTag를 고르는 Task다.
// 결과는 Blackboard의 selected_ability_tag(Name)에 기록되고 ActivateEnemyAbility Task가 읽는다.
UCLASS()
class PROJECTR_API UBTTask_PRSelectEnemyPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRSelectEnemyPattern();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 아래 키 이름들은 Blackboard 에셋과 맞아야 한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelectedAbilityTagKey = TEXT("selected_ability_tag");

	// 이 Task가 선택할 패턴 계열이다. Any면 계열을 제한하지 않는다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	EPRPatternCategory PatternCategoryFilter = EPRPatternCategory::Any;

	// 비어 있지 않으면 해당 원작 패턴군 태그와 맞는 후보만 선택한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	FGameplayTag PatternGroupFilter;

	// true면 ASC가 쿨다운/차단 상태 때문에 실행할 수 없는 패턴은 후보에서 제외한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	bool bCheckAbilityCanActivate = true;

	// true면 패턴 선택 성공 시 전술 모드를 공격 상태로 기록한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	bool bSetTacticalModeOnSelection = false;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern", meta = (EditCondition = "bSetTacticalModeOnSelection"))
	EPRTacticalMode TacticalModeOnSelection = EPRTacticalMode::Attack;

	// true면 직전 실행 패턴의 선택 가중치를 낮춘다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	bool bLowerLastUsedPatternWeight = false;

	// 직전 실행 AbilityTag 이름을 읽는 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern", meta = (EditCondition = "bLowerLastUsedPatternWeight"))
	FName LastUsedAbilityTagKey = TEXT("last_used_ranged_ability");

	// 직전 실행 패턴에 적용할 가중치 배율
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern", meta = (ClampMin = "0.01", ClampMax = "1.0", EditCondition = "bLowerLastUsedPatternWeight"))
	float LastUsedPatternWeightMultiplier = 0.25f;
};
