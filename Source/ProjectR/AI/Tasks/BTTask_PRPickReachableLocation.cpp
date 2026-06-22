// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Pick Reachable Location 비헤이비어 트리 태스크 구현)
#include "BTTask_PRPickReachableLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"

UBTTask_PRPickReachableLocation::UBTTask_PRPickReachableLocation()
{
	NodeName = TEXT("Pick Reachable Location");
}

EBTNodeResult::Type UBTTask_PRPickReachableLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	FVector OriginLocation = FVector::ZeroVector;
	if (!ResolveOriginLocation(BlackboardComponent, OriginLocation))
	{
		return EBTNodeResult::Failed;
	}

	FVector PickedLocation = FVector::ZeroVector;
	if (PickEQSLocation(ControlledPawn, PickedLocation))
	{
		return WriteGoalLocation(BlackboardComponent, PickedLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	if (bFallbackToNavmeshRandom
		&& PickReachableLocation(BlackboardComponent, OriginLocation, PickedLocation))
	{
		return WriteGoalLocation(BlackboardComponent, PickedLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	if (bFallbackToOrigin)
	{
		return WriteGoalLocation(BlackboardComponent, OriginLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	return EBTNodeResult::Failed;
}

FString UBTTask_PRPickReachableLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nOrigin Key: %s\nMove Goal Key: %s\nEQS: %s\nEQS Fallback Navmesh: %s\nRadius: %.1f\nMin Distance: %.1f\nAttempts: %d\nFallback To Origin: %s"),
		*Super::GetStaticDescription(),
		*OriginLocationKey.ToString(),
		*MoveGoalLocationKey.ToString(),
		*GetNameSafe(EQSQueryConfig.QueryTemplate.Get()),
		bFallbackToNavmeshRandom ? TEXT("true") : TEXT("false"),
		PickRadius,
		MinDistanceFromOrigin,
		MaxPickAttempts,
		bFallbackToOrigin ? TEXT("true") : TEXT("false"));
}

bool UBTTask_PRPickReachableLocation::ResolveOriginLocation(const UBlackboardComponent* BlackboardComponent, FVector& OutOriginLocation) const
{
	if (!IsValid(BlackboardComponent) || OriginLocationKey == NAME_None)
	{
		return false;
	}

	if (BlackboardComponent->GetKeyID(OriginLocationKey) == FBlackboard::InvalidKey)
	{
		return false;
	}

	if (!BlackboardComponent->IsVectorValueSet(OriginLocationKey))
	{
		return false;
	}

	OutOriginLocation = BlackboardComponent->GetValueAsVector(OriginLocationKey);
	return true;
}

bool UBTTask_PRPickReachableLocation::PickEQSLocation(APawn* ControlledPawn, FVector& OutPickedLocation) const
{
	return PREnemyEQSSelectionUtils::RunLocationQuery(
		ControlledPawn,
		EQSQueryConfig.QueryTemplate.Get(),
		EQSQueryConfig.FloatParams,
		EQSQueryConfig.QueryRunMode,
		EQSQueryConfig.CandidateSelectionMode,
		EQSQueryConfig.TopCandidateCount,
		EQSQueryConfig.TopScoreCandidateRatio,
		OutPickedLocation);
}

bool UBTTask_PRPickReachableLocation::PickReachableLocation(
	const UObject* WorldContextObject,
	const FVector& OriginLocation,
	FVector& OutPickedLocation) const
{
	if (!IsValid(WorldContextObject))
	{
		return false;
	}

	if (PickRadius <= KINDA_SMALL_NUMBER)
	{
		OutPickedLocation = OriginLocation;
		return true;
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
	if (IsValid(NavigationData) && FilterClass != nullptr)
	{
		QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavigationData, FilterClass);
	}

	const int32 AttemptCount = FMath::Max(MaxPickAttempts, 1);
	for (int32 AttemptIndex = 0; AttemptIndex < AttemptCount; ++AttemptIndex)
	{
		FNavLocation CandidateLocation;
		const bool bHasCandidate = NavigationSystem->GetRandomReachablePointInRadius(
			OriginLocation,
			PickRadius,
			CandidateLocation,
			NavigationData,
			QueryFilter);

		if (!bHasCandidate)
		{
			continue;
		}

		if (MinDistanceFromOrigin > 0.0f
			&& FVector::Dist2D(OriginLocation, CandidateLocation.Location) < MinDistanceFromOrigin)
		{
			continue;
		}

		OutPickedLocation = CandidateLocation.Location;
		return true;
	}

	return false;
}

bool UBTTask_PRPickReachableLocation::WriteGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const
{
	if (!IsValid(BlackboardComponent) || MoveGoalLocationKey == NAME_None)
	{
		return false;
	}

	if (BlackboardComponent->GetKeyID(MoveGoalLocationKey) == FBlackboard::InvalidKey)
	{
		return false;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, GoalLocation);
	return true;
}
