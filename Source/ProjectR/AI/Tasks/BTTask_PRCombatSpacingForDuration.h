// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI 전투 Spacing For Duration 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/Data/UPRPenitentCombatDataAsset.h"
#include "BTTask_PRCombatSpacingForDuration.generated.h"

class AActor;
class APawn;
class UBlackboardComponent;
class UNavigationQueryFilter;

// Penitent 공격 후 교전 거리 조절 Task
UCLASS()
class PROJECTR_API UBTTask_PRCombatSpacingForDuration : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 기본 실행 설정 초기화
	UBTTask_PRCombatSpacingForDuration();

	/*~ UBTTaskNode Interface ~*/
	// Task 시작 처리
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Task 중단 처리
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 지속 시간 중 이동 목표 갱신
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 에디터 표시 설명
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 교전 거리 조절 방향 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CombatSpacingDirectionKey = TEXT("combat_spacing_direction");

	// 이동 목표 위치 기록 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// Penitent 전투 데이터 자산 override
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UPRPenitentCombatDataAsset> CombatDataAsset = nullptr;

	// 이동 목표 갱신 주기
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement", meta = (ClampMin = "0.05"))
	float GoalUpdateInterval = 1.f;

	// MoveTo 도착 허용 거리
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 80.0f;

	// 대치 유지 시 한 번에 회전할 각도
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement", meta = (ClampMin = "1.0", ClampMax = "90.0"))
	float HoldArcStepDegrees = 25.0f;

	// NavMesh 투영 수직 범위
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "0.0"))
	float NavProjectionHalfHeight = 300.0f;

	// 이동 가능 지점 검색에 사용할 Navigation Query Filter
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation")
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Task 종료 시 이동 정지 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement")
	bool bStopMovementOnFinish = true;

private:
	const UPRPenitentCombatDataAsset* ResolveCombatDataAsset(const APawn* ControlledPawn) const;
	bool ResolveRequiredContext(UBehaviorTreeComponent& OwnerComp, UBlackboardComponent*& OutBlackboardComponent, APawn*& OutControlledPawn, AActor*& OutCurrentTarget) const;
	bool ApplySpacingMoveGoal(UBehaviorTreeComponent& OwnerComp);
	bool BuildSpacingGoalLocation(const APawn* ControlledPawn, const AActor* CurrentTarget, EPRPenitentCombatSpacingDirection SpacingDirection, FVector& OutGoalLocation) const;
	bool ProjectGoalLocation(const UObject* WorldContextObject, const FVector& GoalLocation, FVector& OutProjectedLocation) const;
	bool WriteMoveGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const;
	bool RequestMoveToGoal(UBehaviorTreeComponent& OwnerComp, const FVector& GoalLocation) const;
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const;
	EPRPenitentCombatSpacingDirection ReadSpacingDirection(const UBlackboardComponent* BlackboardComponent) const;
	void StopActiveMovement(UBehaviorTreeComponent& OwnerComp) const;

	float EndTimeSeconds = 0.0f;
	float NextGoalUpdateTimeSeconds = 0.0f;
	float HoldDirectionSign = 1.0f;
};
