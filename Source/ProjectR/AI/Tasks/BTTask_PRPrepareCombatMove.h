// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Prepare 전투 Move 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRPrepareCombatMove.generated.h"

class UPRCombatMoveDataAsset;

UENUM(BlueprintType)
enum class EPRCombatMoveQueryType : uint8
{
	Strafe				UMETA(DisplayName = "Strafe"),
	ApproachMeleeRange	UMETA(DisplayName = "ApproachMeleeRange"),
	FastApproach		UMETA(DisplayName = "FastApproach"),
	PatternFallbackReposition UMETA(DisplayName = "PatternFallbackReposition")
};

// EQS 기반 전투 이동 목표 선택 및 전투 표현 문맥 준비 Task
UCLASS()
class PROJECTR_API UBTTask_PRPrepareCombatMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRPrepareCombatMove();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 이동 목표 위치 기록 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// tactical_mode Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 공용 전투 데이터 자산 override
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UPRCombatMoveDataAsset> CombatDataAsset = nullptr;

	// 실행할 전투 이동 쿼리 종류
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	EPRCombatMoveQueryType SelectionMode = EPRCombatMoveQueryType::Strafe;

	// 성공 시 tactical_mode 기록 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	bool bSetTacticalModeOnSuccess = false;

	// 성공 시 기록할 tactical_mode
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat", meta = (EditCondition = "bSetTacticalModeOnSuccess"))
	EPRTacticalMode TacticalModeOnSuccess = EPRTacticalMode::Strafe;
};
