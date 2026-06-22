// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI 전투 Spacing For Duration 비헤이비어 트리 태스크 구현)
#include "BTTask_PRCombatSpacingForDuration.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

/*~ 내부 유틸리티 ~*/
namespace
{
	// 거리 기준 중앙값 계산
	float GetSpacingCenterDistance(const FPRPenitentCombatRangeData& RangeData)
	{
		return (RangeData.SpacingMinRange + RangeData.SpacingMaxRange) * 0.5f;
	}

	// 2D 안전 방향 계산
	FVector GetSafeDirection2D(const FVector& Direction, const FVector& FallbackDirection)
	{
		FVector SafeDirection = Direction;
		SafeDirection.Z = 0.0f;

		if (SafeDirection.IsNearlyZero())
		{
			SafeDirection = FallbackDirection;
			SafeDirection.Z = 0.0f;
		}

		if (SafeDirection.IsNearlyZero())
		{
			return FVector::ForwardVector;
		}

		return SafeDirection.GetSafeNormal2D();
	}
}

/*~ 생성자 ~*/
UBTTask_PRCombatSpacingForDuration::UBTTask_PRCombatSpacingForDuration()
{
	NodeName = TEXT("Penitent Combat Spacing For Duration");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

/*~ UBTTaskNode Interface ~*/
EBTNodeResult::Type UBTTask_PRCombatSpacingForDuration::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = nullptr;
	APawn* ControlledPawn = nullptr;
	AActor* CurrentTarget = nullptr;
	if (!ResolveRequiredContext(OwnerComp, BlackboardComponent, ControlledPawn, CurrentTarget))
	{
		return EBTNodeResult::Failed;
	}

	const UPRPenitentCombatDataAsset* ResolvedCombatDataAsset = ResolveCombatDataAsset(ControlledPawn);
	if (!IsValid(ResolvedCombatDataAsset) || !ResolvedCombatDataAsset->CombatRangeData.IsValidRangeOrder())
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!IsValid(World))
	{
		return EBTNodeResult::Failed;
	}

	const FPRPenitentCombatRangeData& RangeData = ResolvedCombatDataAsset->CombatRangeData;
	const float MinDuration = FMath::Max(0.0f, RangeData.CombatSpacingMinTime);
	const float MaxDuration = FMath::Max(MinDuration, RangeData.CombatSpacingMaxTime);
	const float Duration = FMath::RandRange(MinDuration, MaxDuration);
	if (Duration <= 0.0f)
	{
		return EBTNodeResult::Succeeded;
	}

	const float CurrentTime = World->GetTimeSeconds();
	EndTimeSeconds = CurrentTime + Duration;
	NextGoalUpdateTimeSeconds = 0.0f;
	HoldDirectionSign = FMath::RandBool() ? 1.0f : -1.0f;

	if (!ApplySpacingMoveGoal(OwnerComp))
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_PRCombatSpacingForDuration::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	StopActiveMovement(OwnerComp);
	return EBTNodeResult::Aborted;
}

void UBTTask_PRCombatSpacingForDuration::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UWorld* World = OwnerComp.GetWorld();
	if (!IsValid(World))
	{
		StopActiveMovement(OwnerComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime >= EndTimeSeconds)
	{
		StopActiveMovement(OwnerComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (CurrentTime < NextGoalUpdateTimeSeconds)
	{
		return;
	}

	if (!ApplySpacingMoveGoal(OwnerComp))
	{
		StopActiveMovement(OwnerComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString UBTTask_PRCombatSpacingForDuration::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nTarget Key: %s\nDirection Key: %s\nMove Goal Key: %s\nDataAsset: %s\nUpdate Interval: %.2f\nAcceptance Radius: %.1f"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*CombatSpacingDirectionKey.ToString(),
		*MoveGoalLocationKey.ToString(),
		*GetNameSafe(CombatDataAsset.Get()),
		GoalUpdateInterval,
		AcceptanceRadius);
}

/*~ 데이터 조회 ~*/
const UPRPenitentCombatDataAsset* UBTTask_PRCombatSpacingForDuration::ResolveCombatDataAsset(const APawn* ControlledPawn) const
{
	if (IsValid(CombatDataAsset))
	{
		return CombatDataAsset.Get();
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (!EnemyInterface)
	{
		return nullptr;
	}

	return Cast<UPRPenitentCombatDataAsset>(EnemyInterface->GetCombatDataAsset());
}

bool UBTTask_PRCombatSpacingForDuration::ResolveRequiredContext(
	UBehaviorTreeComponent& OwnerComp,
	UBlackboardComponent*& OutBlackboardComponent,
	APawn*& OutControlledPawn,
	AActor*& OutCurrentTarget) const
{
	OutBlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	OutControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;

	if (!IsValid(OutBlackboardComponent)
		|| !IsValid(OutControlledPawn)
		|| !HasBlackboardKey(OutBlackboardComponent, CurrentTargetKey)
		|| !HasBlackboardKey(OutBlackboardComponent, CombatSpacingDirectionKey)
		|| !HasBlackboardKey(OutBlackboardComponent, MoveGoalLocationKey))
	{
		return false;
	}

	OutCurrentTarget = Cast<AActor>(OutBlackboardComponent->GetValueAsObject(CurrentTargetKey));
	return IsValid(OutCurrentTarget);
}

/*~ 이동 목표 갱신 ~*/
bool UBTTask_PRCombatSpacingForDuration::ApplySpacingMoveGoal(UBehaviorTreeComponent& OwnerComp)
{
	UBlackboardComponent* BlackboardComponent = nullptr;
	APawn* ControlledPawn = nullptr;
	AActor* CurrentTarget = nullptr;
	if (!ResolveRequiredContext(OwnerComp, BlackboardComponent, ControlledPawn, CurrentTarget))
	{
		return false;
	}

	const EPRPenitentCombatSpacingDirection SpacingDirection = ReadSpacingDirection(BlackboardComponent);
	if (SpacingDirection == EPRPenitentCombatSpacingDirection::None)
	{
		return false;
	}

	FVector GoalLocation = FVector::ZeroVector;
	if (!BuildSpacingGoalLocation(ControlledPawn, CurrentTarget, SpacingDirection, GoalLocation))
	{
		return false;
	}

	FVector ProjectedGoalLocation = GoalLocation;
	ProjectGoalLocation(ControlledPawn, GoalLocation, ProjectedGoalLocation);

	if (!WriteMoveGoalLocation(BlackboardComponent, ProjectedGoalLocation))
	{
		return false;
	}

	if (!RequestMoveToGoal(OwnerComp, ProjectedGoalLocation))
	{
		return false;
	}

	if (UWorld* World = OwnerComp.GetWorld())
	{
		NextGoalUpdateTimeSeconds = World->GetTimeSeconds() + FMath::Max(GoalUpdateInterval, 0.05f);
	}

	return true;
}

bool UBTTask_PRCombatSpacingForDuration::BuildSpacingGoalLocation(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	EPRPenitentCombatSpacingDirection SpacingDirection,
	FVector& OutGoalLocation) const
{
	const UPRPenitentCombatDataAsset* ResolvedCombatDataAsset = ResolveCombatDataAsset(ControlledPawn);
	if (!IsValid(ControlledPawn)
		|| !IsValid(CurrentTarget)
		|| !IsValid(ResolvedCombatDataAsset)
		|| !ResolvedCombatDataAsset->CombatRangeData.IsValidRangeOrder())
	{
		return false;
	}

	const FPRPenitentCombatRangeData& RangeData = ResolvedCombatDataAsset->CombatRangeData;
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector AwayFromTarget = GetSafeDirection2D(
		PawnLocation - TargetLocation,
		-CurrentTarget->GetActorForwardVector());

	const float CenterDistance = GetSpacingCenterDistance(RangeData);
	const float CurrentDistance = FVector::Dist2D(PawnLocation, TargetLocation);
	FVector GoalDirection = AwayFromTarget;
	float DesiredDistance = CenterDistance;

	if (SpacingDirection == EPRPenitentCombatSpacingDirection::Hold)
	{
		const float SignedArcStep = HoldArcStepDegrees * HoldDirectionSign;
		GoalDirection = AwayFromTarget.RotateAngleAxis(SignedArcStep, FVector::UpVector).GetSafeNormal2D();
		DesiredDistance = FMath::Clamp(CurrentDistance, RangeData.SpacingMinRange, RangeData.SpacingMaxRange);
	}

	OutGoalLocation = TargetLocation + GoalDirection * DesiredDistance;
	OutGoalLocation.Z = PawnLocation.Z;
	return true;
}

bool UBTTask_PRCombatSpacingForDuration::ProjectGoalLocation(
	const UObject* WorldContextObject,
	const FVector& GoalLocation,
	FVector& OutProjectedLocation) const
{
	if (!IsValid(WorldContextObject))
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!IsValid(NavigationSystem))
	{
		return false;
	}

	ANavigationData* NavigationData = NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	FSharedConstNavQueryFilter QueryFilter = nullptr;
	if (IsValid(NavigationData) && FilterClass)
	{
		QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavigationData, FilterClass);
	}

	FNavLocation ProjectedLocation;
	const FVector QueryExtent(100.0f, 100.0f, FMath::Max(NavProjectionHalfHeight, 0.0f));
	if (!NavigationSystem->ProjectPointToNavigation(GoalLocation, ProjectedLocation, QueryExtent, NavigationData, QueryFilter))
	{
		return false;
	}

	OutProjectedLocation = ProjectedLocation.Location;
	return true;
}

bool UBTTask_PRCombatSpacingForDuration::WriteMoveGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const
{
	if (!HasBlackboardKey(BlackboardComponent, MoveGoalLocationKey))
	{
		return false;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, GoalLocation);
	return true;
}

bool UBTTask_PRCombatSpacingForDuration::RequestMoveToGoal(UBehaviorTreeComponent& OwnerComp, const FVector& GoalLocation) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!IsValid(AIController))
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		GoalLocation,
		AcceptanceRadius,
		true,
		true,
		false,
		true,
		FilterClass,
		true);

	return MoveResult != EPathFollowingRequestResult::Failed;
}

/*~ Blackboard 유틸리티 ~*/
bool UBTTask_PRCombatSpacingForDuration::HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const
{
	return IsValid(BlackboardComponent)
		&& KeyName != NAME_None
		&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
}

EPRPenitentCombatSpacingDirection UBTTask_PRCombatSpacingForDuration::ReadSpacingDirection(const UBlackboardComponent* BlackboardComponent) const
{
	if (!HasBlackboardKey(BlackboardComponent, CombatSpacingDirectionKey))
	{
		return EPRPenitentCombatSpacingDirection::None;
	}

	return static_cast<EPRPenitentCombatSpacingDirection>(BlackboardComponent->GetValueAsEnum(CombatSpacingDirectionKey));
}

/*~ 이동 정리 ~*/
void UBTTask_PRCombatSpacingForDuration::StopActiveMovement(UBehaviorTreeComponent& OwnerComp) const
{
	if (!bStopMovementOnFinish)
	{
		return;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}
}
