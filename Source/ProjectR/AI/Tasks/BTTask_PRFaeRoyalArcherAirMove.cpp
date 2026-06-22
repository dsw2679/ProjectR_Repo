// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Air Move 비헤이비어 트리 태스크 구현)
#include "BTTask_PRFaeRoyalArcherAirMove.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "DrawDebugHelpers.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "ProjectR/AI/Data/PRFaeRoyalArcherCombatDataAsset.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	static TAutoConsoleVariable<int32> CVarPRFaeRoyalArcherDebugAirMoveGoalSearch(
		TEXT("pr.FaeRoyalArcher.DebugAirMoveGoalSearch"),
		0,
		TEXT("Fae Royal Archer 공중 이동 목표 후보와 점수를 화면에 표시한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarPRFaeRoyalArcherDebugAirMoveGoalSearchDuration(
		TEXT("pr.FaeRoyalArcher.DebugAirMoveGoalSearchDuration"),
		1.0f,
		TEXT("Fae Royal Archer 공중 이동 목표 후보 화면 디버그 유지 시간이다."),
		ECVF_Default);

	// 공중 이동 목표 후보 화면 디버그 활성 여부
	bool IsRoyalArcherAirMoveGoalSearchDebugEnabled()
	{
		return CVarPRFaeRoyalArcherDebugAirMoveGoalSearch.GetValueOnGameThread() > 0;
	}

	// 공중 이동 목표 후보 화면 디버그 유지 시간
	float GetRoyalArcherAirMoveGoalSearchDebugDuration()
	{
		return FMath::Max(CVarPRFaeRoyalArcherDebugAirMoveGoalSearchDuration.GetValueOnGameThread(), 0.0f);
	}

	// 공중 이동 목표 후보 위치와 점수 표시
	void DrawRoyalArcherAirMoveGoalCandidateDebug(
		const APawn* ControlledPawn,
		const FVector& CandidateGoal,
		float CandidateScore,
		bool bBestCandidate)
	{
		if (!IsValid(ControlledPawn))
		{
			return;
		}

		UWorld* World = ControlledPawn->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		const float DrawDuration = GetRoyalArcherAirMoveGoalSearchDebugDuration();
		const FColor CandidateColor = bBestCandidate ? FColor::Green : FColor::Yellow;
		const float SphereRadius = bBestCandidate ? 72.0f : 48.0f;
		const float SphereThickness = bBestCandidate ? 3.0f : 1.5f;
		const FVector ScoreTextLocation = CandidateGoal + FVector(0.0f, 0.0f, SphereRadius + 24.0f);

		DrawDebugSphere(World, CandidateGoal, SphereRadius, 16, CandidateColor, false, DrawDuration, 0, SphereThickness);
		DrawDebugString(
			World,
			ScoreTextLocation,
			FString::Printf(TEXT("Score %.2f"), CandidateScore),
			nullptr,
			CandidateColor,
			DrawDuration,
			true,
			bBestCandidate ? 1.1f : 0.9f);
	}

	// 2D 방향이 비어 있을 때 사용할 안전 방향을 계산한다.
	FVector GetRoyalArcherSafeDirection2D(const FVector& Direction, const FVector& FallbackDirection)
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

	// 타겟 기준 후보 각도 목록을 만든다.
	void BuildRoyalArcherLosCandidateAngles(float BaseArcDegrees, float DirectionSign, TArray<float>& OutAngles)
	{
		OutAngles.Reset();
		OutAngles.Add(BaseArcDegrees * DirectionSign);
		OutAngles.Add(-BaseArcDegrees * DirectionSign);
		OutAngles.Add(0.0f);
		OutAngles.Add(60.0f * DirectionSign);
		OutAngles.Add(-60.0f * DirectionSign);
		OutAngles.Add(120.0f * DirectionSign);
		OutAngles.Add(-120.0f * DirectionSign);
	}
}

// ===== UBTTaskNode Interface =====

UBTTask_PRFaeRoyalArcherAirMove::UBTTask_PRFaeRoyalArcherAirMove()
{
	NodeName = TEXT("Fae Royal Archer Air Move");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_PRFaeRoyalArcherAirMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = nullptr;
	APawn* ControlledPawn = nullptr;
	AActor* CurrentTarget = nullptr;
	UPRFaeRoyalArcherCombatDataAsset* CombatDataAsset = nullptr;
	if (!ResolveContext(OwnerComp, BlackboardComponent, ControlledPawn, CurrentTarget, CombatDataAsset))
	{
		return EBTNodeResult::Failed;
	}

	if (IsDisabledByGameplayState(ControlledPawn))
	{
		return EBTNodeResult::Failed;
	}

	if (!BuildAirMoveGoal(ControlledPawn, CurrentTarget, BlackboardComponent, *CombatDataAsset, ActiveGoalLocation))
	{
		return EBTNodeResult::Failed;
	}

	if (HasBlackboardKey(BlackboardComponent, MoveGoalLocationKey))
	{
		BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, ActiveGoalLocation);
	}

	if (bSetTacticalModeOnStart && HasBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		EPRTacticalMode ResolvedTacticalMode = TacticalModeOnStart;
		if (MoveMode == EPRFaeRoyalArcherAirMoveMode::ReturnHome)
		{
			ResolvedTacticalMode = EPRTacticalMode::Return;
		}
		else if (MoveMode == EPRFaeRoyalArcherAirMoveMode::SearchLastKnown)
		{
			ResolvedTacticalMode = EPRTacticalMode::Alert;
		}
		else if (MoveMode == EPRFaeRoyalArcherAirMoveMode::PatrolHome)
		{
			ResolvedTacticalMode = EPRTacticalMode::Patrol;
		}

		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(ResolvedTacticalMode));
	}

	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
	}

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
			MovementComponent->MaxFlySpeed = CombatDataAsset->AirMoveSpeed;
		}
	}

	ApplyPresentationContext(OwnerComp, CurrentTarget, *CombatDataAsset);

	if (UWorld* World = OwnerComp.GetWorld())
	{
		EndTimeSeconds = World->GetTimeSeconds() + FMath::Max(CombatDataAsset->AirMoveTimeout, 0.05f);
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_PRFaeRoyalArcherAirMove::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanupAirMove(OwnerComp, EBTNodeResult::Aborted);
	return EBTNodeResult::Aborted;
}

void UBTTask_PRFaeRoyalArcherAirMove::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BlackboardComponent = nullptr;
	APawn* ControlledPawn = nullptr;
	AActor* CurrentTarget = nullptr;
	UPRFaeRoyalArcherCombatDataAsset* CombatDataAsset = nullptr;
	if (!ResolveContext(OwnerComp, BlackboardComponent, ControlledPawn, CurrentTarget, CombatDataAsset)
		|| IsDisabledByGameplayState(ControlledPawn))
	{
		FinishAirMove(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!IsValid(World) || World->GetTimeSeconds() >= EndTimeSeconds)
	{
		FinishAirMove(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float AcceptanceRadius = FMath::Max(CombatDataAsset->AirMoveAcceptanceRadius, 0.0f);
	if (FVector::Dist(ControlledPawn->GetActorLocation(), ActiveGoalLocation) <= AcceptanceRadius)
	{
		if (MoveMode == EPRFaeRoyalArcherAirMoveMode::CombatStrafe
			|| MoveMode == EPRFaeRoyalArcherAirMoveMode::LosReposition
			|| MoveMode == EPRFaeRoyalArcherAirMoveMode::CloseEvade
			|| MoveMode == EPRFaeRoyalArcherAirMoveMode::SearchLastKnown
			|| MoveMode == EPRFaeRoyalArcherAirMoveMode::PatrolHome)
		{
			NextStrafeDirectionSign *= -1.0f;
		}

		FinishAirMove(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	ApplyFlyingMovement(ControlledPawn, *CombatDataAsset, DeltaSeconds);
}

FString UBTTask_PRFaeRoyalArcherAirMove::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nMode: %s\nTarget Key: %s\nGoal Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRFaeRoyalArcherAirMoveMode>()->GetNameStringByValue(static_cast<int64>(MoveMode)),
		*CurrentTargetKey.ToString(),
		*MoveGoalLocationKey.ToString());
}

// ===== 실행 컨텍스트 =====

bool UBTTask_PRFaeRoyalArcherAirMove::ResolveContext(
	UBehaviorTreeComponent& OwnerComp,
	UBlackboardComponent*& OutBlackboardComponent,
	APawn*& OutControlledPawn,
	AActor*& OutCurrentTarget,
	UPRFaeRoyalArcherCombatDataAsset*& OutCombatDataAsset) const
{
	OutBlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	OutControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	OutCurrentTarget = nullptr;
	OutCombatDataAsset = nullptr;

	if (!IsValid(OutBlackboardComponent) || !IsValid(OutControlledPawn))
	{
		return false;
	}

	if (HasBlackboardKey(OutBlackboardComponent, CurrentTargetKey))
	{
		OutCurrentTarget = Cast<AActor>(OutBlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (DoesMoveModeRequireTarget() && !IsValid(OutCurrentTarget))
	{
		return false;
	}

	if (IsValid(CombatDataAssetOverride))
	{
		OutCombatDataAsset = CombatDataAssetOverride.Get();
		return true;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(OutControlledPawn);
	if (EnemyInterface == nullptr)
	{
		return false;
	}

	OutCombatDataAsset = Cast<UPRFaeRoyalArcherCombatDataAsset>(EnemyInterface->GetCombatDataAsset());
	return IsValid(OutCombatDataAsset);
}

// ===== 목표 위치 계산 =====

bool UBTTask_PRFaeRoyalArcherAirMove::BuildAirMoveGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UBlackboardComponent* BlackboardComponent,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	FVector& OutGoalLocation)
{
	if (!IsValid(ControlledPawn))
	{
		return false;
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::ReturnHome)
	{
		OutGoalLocation = BuildReturnHomeGoal(BlackboardComponent, ControlledPawn, CombatDataAsset);
		return ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, OutGoalLocation, false);
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::SearchLastKnown)
	{
		TArray<float> CandidateAngles;
		BuildRoyalArcherLosCandidateAngles(CombatDataAsset.AirStrafeArcDegrees, NextStrafeDirectionSign, CandidateAngles);
		for (const float CandidateAngle : CandidateAngles)
		{
			const FVector CandidateGoal = BuildSearchLastKnownGoal(BlackboardComponent, ControlledPawn, CombatDataAsset, CandidateAngle);
			if (ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, CandidateGoal, false))
			{
				OutGoalLocation = CandidateGoal;
				return true;
			}
		}

		return false;
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::PatrolHome)
	{
		TArray<float> CandidateAngles;
		BuildRoyalArcherLosCandidateAngles(CombatDataAsset.AirStrafeArcDegrees, NextStrafeDirectionSign, CandidateAngles);
		for (const float CandidateAngle : CandidateAngles)
		{
			const FVector CandidateGoal = BuildPatrolHomeGoal(BlackboardComponent, ControlledPawn, CombatDataAsset, CandidateAngle);
			if (ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, CandidateGoal, false))
			{
				OutGoalLocation = CandidateGoal;
				return true;
			}
		}

		return false;
	}

	if (!IsValid(CurrentTarget))
	{
		return false;
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::CloseEvade)
	{
		OutGoalLocation = BuildCloseEvadeGoal(ControlledPawn, CurrentTarget, CombatDataAsset);
		return ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, OutGoalLocation, false);
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::LosReposition)
	{
		if (TryBuildAirCombatQueryGoal(ControlledPawn, CurrentTarget, CombatDataAsset, true, OutGoalLocation))
		{
			return true;
		}

		return TryBuildScoredAirCombatGoal(ControlledPawn, CurrentTarget, CombatDataAsset, true, OutGoalLocation);
	}

	if (TryBuildAirCombatQueryGoal(ControlledPawn, CurrentTarget, CombatDataAsset, false, OutGoalLocation))
	{
		return true;
	}

	return TryBuildScoredAirCombatGoal(ControlledPawn, CurrentTarget, CombatDataAsset, false, OutGoalLocation);
}

bool UBTTask_PRFaeRoyalArcherAirMove::TryBuildAirCombatQueryGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	bool bRequireLineOfSight,
	FVector& OutGoalLocation) const
{
	if (!IsValid(ControlledPawn)
		|| !IsValid(CurrentTarget)
		|| !CombatDataAsset.bUseAirCombatPositionQuery
		|| !IsValid(CombatDataAsset.AirCombatPositionQueryConfig.QueryTemplate.Get()))
	{
		return false;
	}

	const FPREnemyMoveQueryConfig& QueryConfig = CombatDataAsset.AirCombatPositionQueryConfig;
	FVector QueryLocation = FVector::ZeroVector;
	if (!PREnemyEQSSelectionUtils::RunLocationQuery(
			const_cast<APawn*>(ControlledPawn),
			QueryConfig.QueryTemplate.Get(),
			QueryConfig.FloatParams,
			QueryConfig.QueryRunMode,
			QueryConfig.CandidateSelectionMode,
			QueryConfig.TopCandidateCount,
			QueryConfig.TopScoreCandidateRatio,
			QueryLocation))
	{
		return false;
	}

	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float MinAttackDistance = FMath::Max(CombatDataAsset.MinAttackDistance, 0.0f);
	const float MaxAttackDistance = FMath::Max(CombatDataAsset.MaxAttackDistance, MinAttackDistance);
	FVector GoalLocation = QueryLocation;
	GoalLocation.Z = TargetLocation.Z + CombatDataAsset.PreferredCombatHoverHeight;
	GoalLocation = ClampGoalAltitude(GoalLocation, TargetLocation.Z, CombatDataAsset);

	const float GoalDistance2D = FVector::Dist2D(GoalLocation, TargetLocation);
	if (GoalDistance2D < MinAttackDistance || GoalDistance2D > MaxAttackDistance)
	{
		return false;
	}

	if (!ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, GoalLocation, bRequireLineOfSight))
	{
		return false;
	}

	OutGoalLocation = GoalLocation;
	return true;
}

bool UBTTask_PRFaeRoyalArcherAirMove::TryBuildScoredAirCombatGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	bool bRequireLineOfSight,
	FVector& OutGoalLocation) const
{
	if (!IsValid(ControlledPawn) || !IsValid(CurrentTarget))
	{
		return false;
	}

	TArray<float> CandidateAngles;
	BuildRoyalArcherLosCandidateAngles(CombatDataAsset.AirStrafeArcDegrees, NextStrafeDirectionSign, CandidateAngles);

	TArray<float> DistanceOffsets = CombatDataAsset.AirPositionDistanceOffsets;
	if (DistanceOffsets.IsEmpty())
	{
		DistanceOffsets.Add(0.0f);
	}

	TArray<float> HeightOffsets = CombatDataAsset.AirPositionHeightOffsets;
	if (HeightOffsets.IsEmpty())
	{
		HeightOffsets.Add(0.0f);
	}

	bool bFoundCandidate = false;
	float BestScore = 0.0f;
	FVector BestGoalLocation = FVector::ZeroVector;
	const bool bDrawGoalSearchDebug = IsRoyalArcherAirMoveGoalSearchDebugEnabled();

	for (const float CandidateAngle : CandidateAngles)
	{
		for (const float DistanceOffset : DistanceOffsets)
		{
			for (const float HeightOffset : HeightOffsets)
			{
				const FVector CandidateGoal = BuildTargetRingGoal(
					ControlledPawn,
					CurrentTarget,
					CombatDataAsset,
					CandidateAngle,
					DistanceOffset,
					HeightOffset);

				if (!ValidateAirMoveGoal(ControlledPawn, CurrentTarget, CombatDataAsset, CandidateGoal, bRequireLineOfSight))
				{
					continue;
				}

				const bool bHasLineOfSight = bRequireLineOfSight
					|| HasLineOfSightFromGoal(ControlledPawn, CurrentTarget, CandidateGoal);
				const float CandidateScore = ScoreAirCombatGoal(
					ControlledPawn,
					CurrentTarget,
					CombatDataAsset,
					CandidateGoal,
					bHasLineOfSight);

				if (bDrawGoalSearchDebug)
				{
					DrawRoyalArcherAirMoveGoalCandidateDebug(ControlledPawn, CandidateGoal, CandidateScore, false);
				}

				if (!bFoundCandidate || CandidateScore > BestScore)
				{
					bFoundCandidate = true;
					BestScore = CandidateScore;
					BestGoalLocation = CandidateGoal;
				}
			}
		}
	}

	if (!bFoundCandidate)
	{
		return false;
	}

	if (bDrawGoalSearchDebug)
	{
		DrawRoyalArcherAirMoveGoalCandidateDebug(ControlledPawn, BestGoalLocation, BestScore, true);
	}

	OutGoalLocation = BestGoalLocation;
	return true;
}

FVector UBTTask_PRFaeRoyalArcherAirMove::BuildTargetRingGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	float SignedArcDegrees,
	float DistanceOffset,
	float HeightOffset) const
{
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector AwayFromTarget = GetRoyalArcherSafeDirection2D(PawnLocation - TargetLocation, -CurrentTarget->GetActorForwardVector());
	const FVector StrafeDirection = AwayFromTarget.RotateAngleAxis(SignedArcDegrees, FVector::UpVector).GetSafeNormal2D();

	const float MinAttackDistance = FMath::Max(CombatDataAsset.MinAttackDistance, 0.0f);
	const float MaxAttackDistance = FMath::Max(CombatDataAsset.MaxAttackDistance, MinAttackDistance);
	const float DesiredDistance = FMath::Clamp(
		CombatDataAsset.PreferredAttackDistance + DistanceOffset,
		MinAttackDistance,
		MaxAttackDistance);
	FVector GoalLocation = TargetLocation + StrafeDirection * DesiredDistance;
	GoalLocation.Z = TargetLocation.Z + CombatDataAsset.PreferredCombatHoverHeight + HeightOffset;
	return ClampGoalAltitude(GoalLocation, TargetLocation.Z, CombatDataAsset);
}

FVector UBTTask_PRFaeRoyalArcherAirMove::BuildCloseEvadeGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const
{
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector AwayFromTarget = GetRoyalArcherSafeDirection2D(PawnLocation - TargetLocation, -CurrentTarget->GetActorForwardVector());
	const FVector SideDirection = FVector::CrossProduct(FVector::UpVector, AwayFromTarget).GetSafeNormal2D() * NextStrafeDirectionSign;

	FVector GoalLocation = PawnLocation
		+ AwayFromTarget * CombatDataAsset.CloseEvadeBackDistance
		+ SideDirection * CombatDataAsset.CloseEvadeSideDistance;
	GoalLocation.Z = PawnLocation.Z + CombatDataAsset.CloseEvadeUpDistance;
	return ClampGoalAltitude(GoalLocation, TargetLocation.Z, CombatDataAsset);
}

FVector UBTTask_PRFaeRoyalArcherAirMove::BuildSearchLastKnownGoal(
	const UBlackboardComponent* BlackboardComponent,
	const APawn* ControlledPawn,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	float SignedArcDegrees) const
{
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector AnchorLocation = ReadSearchAnchorLocation(BlackboardComponent, ControlledPawn);
	const FVector AwayFromAnchor = GetRoyalArcherSafeDirection2D(PawnLocation - AnchorLocation, ControlledPawn->GetActorForwardVector());
	const FVector SearchDirection = AwayFromAnchor.RotateAngleAxis(SignedArcDegrees, FVector::UpVector).GetSafeNormal2D();

	FVector GoalLocation = AnchorLocation + SearchDirection * FMath::Max(CombatDataAsset.AirSearchRadius, 0.0f);
	GoalLocation.Z = AnchorLocation.Z + CombatDataAsset.AirSearchHoverHeight;
	return ClampGoalAltitude(GoalLocation, AnchorLocation.Z, CombatDataAsset);
}

FVector UBTTask_PRFaeRoyalArcherAirMove::BuildPatrolHomeGoal(
	const UBlackboardComponent* BlackboardComponent,
	const APawn* ControlledPawn,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	float SignedArcDegrees) const
{
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector HomeLocation = ReadHomeLocation(BlackboardComponent, ControlledPawn);
	const FVector AwayFromHome = GetRoyalArcherSafeDirection2D(PawnLocation - HomeLocation, ControlledPawn->GetActorForwardVector());
	const FVector PatrolDirection = AwayFromHome.RotateAngleAxis(SignedArcDegrees, FVector::UpVector).GetSafeNormal2D();

	FVector GoalLocation = HomeLocation + PatrolDirection * FMath::Max(CombatDataAsset.AirPatrolRadius, 0.0f);
	GoalLocation.Z = HomeLocation.Z + CombatDataAsset.AirPatrolHoverHeight;
	return ClampGoalAltitude(GoalLocation, HomeLocation.Z, CombatDataAsset);
}

FVector UBTTask_PRFaeRoyalArcherAirMove::BuildReturnHomeGoal(
	const UBlackboardComponent* BlackboardComponent,
	const APawn* ControlledPawn,
	const UPRFaeRoyalArcherCombatDataAsset& /*CombatDataAsset*/) const
{
	return ReadHomeLocation(BlackboardComponent, ControlledPawn);
}

FVector UBTTask_PRFaeRoyalArcherAirMove::ClampGoalAltitude(
	const FVector& GoalLocation,
	float ReferenceZ,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const
{
	FVector ClampedGoalLocation = GoalLocation;
	const float MinZ = ReferenceZ + FMath::Max(CombatDataAsset.MinFlightHeightAboveReference, 0.0f);
	const float MaxZ = ReferenceZ + FMath::Max(CombatDataAsset.MaxFlightHeightAboveReference, CombatDataAsset.MinFlightHeightAboveReference);
	ClampedGoalLocation.Z = FMath::Clamp(ClampedGoalLocation.Z, MinZ, MaxZ);
	return ClampedGoalLocation;
}

float UBTTask_PRFaeRoyalArcherAirMove::ScoreAirCombatGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	const FVector& GoalLocation,
	bool bHasLineOfSight) const
{
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float PreferredDistance = FMath::Max(CombatDataAsset.PreferredAttackDistance, 1.0f);
	const float Distance2D = FVector::Dist2D(GoalLocation, TargetLocation);
	const float DistanceScore = 1.0f - FMath::Clamp(FMath::Abs(Distance2D - PreferredDistance) / PreferredDistance, 0.0f, 1.0f);

	const float MinSeparation = FMath::Max(CombatDataAsset.AirPositionMinSeparationFromCurrent, 1.0f);
	const float SeparationScore = FMath::Clamp(FVector::Dist(GoalLocation, PawnLocation) / MinSeparation, 0.0f, 1.0f);

	const float PreferredHeight = TargetLocation.Z + CombatDataAsset.PreferredCombatHoverHeight;
	const float HeightRange = FMath::Max(
		CombatDataAsset.MaxFlightHeightAboveReference - CombatDataAsset.MinFlightHeightAboveReference,
		1.0f);
	const float HeightScore = 1.0f - FMath::Clamp(FMath::Abs(GoalLocation.Z - PreferredHeight) / HeightRange, 0.0f, 1.0f);

	return DistanceScore * CombatDataAsset.AirPositionDistanceScoreWeight
		+ SeparationScore * CombatDataAsset.AirPositionSeparationScoreWeight
		+ HeightScore * CombatDataAsset.AirPositionHeightScoreWeight
		+ (bHasLineOfSight ? CombatDataAsset.AirPositionLOSScoreWeight : 0.0f);
}

// ===== 목표 위치 검증 =====

bool UBTTask_PRFaeRoyalArcherAirMove::ValidateAirMoveGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	const FVector& GoalLocation,
	bool bRequireLineOfSight) const
{
	if (!IsValid(ControlledPawn))
	{
		return false;
	}

	UWorld* World = ControlledPawn->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaeRoyalArcherAirMove), false, ControlledPawn);
	if (IsValid(CurrentTarget))
	{
		QueryParams.AddIgnoredActor(CurrentTarget);
	}

	const float ProbeRadius = FMath::Max(CombatDataAsset.AirMoveProbeRadius, 0.0f);
	if (ProbeRadius > 0.0f)
	{
		FHitResult HitResult;
		const bool bBlocked = World->SweepSingleByChannel(
			HitResult,
			ControlledPawn->GetActorLocation(),
			GoalLocation,
			FQuat::Identity,
			ObstacleTraceChannel,
			FCollisionShape::MakeSphere(ProbeRadius),
			QueryParams);

		if (bBlocked)
		{
			return false;
		}
	}

	return !bRequireLineOfSight || HasLineOfSightFromGoal(ControlledPawn, CurrentTarget, GoalLocation);
}

bool UBTTask_PRFaeRoyalArcherAirMove::HasLineOfSightFromGoal(
	const APawn* ControlledPawn,
	const AActor* CurrentTarget,
	const FVector& GoalLocation) const
{
	if (!IsValid(ControlledPawn) || !IsValid(CurrentTarget))
	{
		return false;
	}

	UWorld* World = ControlledPawn->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaeRoyalArcherAirMoveLOS), false, ControlledPawn);
	FHitResult HitResult;
	const bool bBlocked = World->LineTraceSingleByChannel(
		HitResult,
		GoalLocation,
		CurrentTarget->GetActorLocation(),
		LOSCheckTraceChannel,
		QueryParams);

	const AActor* HitActor = HitResult.GetActor();
	return !bBlocked
		|| HitActor == CurrentTarget
		|| (IsValid(HitActor) && HitActor->GetOwner() == CurrentTarget);
}

// ===== 이동 적용 =====

void UBTTask_PRFaeRoyalArcherAirMove::ApplyFlyingMovement(
	APawn* ControlledPawn,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
	float DeltaSeconds) const
{
	ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
	if (!IsValid(ControlledCharacter))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	if (MovementComponent->MovementMode != MOVE_Flying)
	{
		MovementComponent->SetMovementMode(MOVE_Flying);
	}

	MovementComponent->MaxFlySpeed = CombatDataAsset.AirMoveSpeed;

	const FVector ToGoal = ActiveGoalLocation - ControlledCharacter->GetActorLocation();
	if (ToGoal.IsNearlyZero())
	{
		return;
	}

	ControlledCharacter->AddMovementInput(ToGoal.GetSafeNormal(), 1.0f);
}

void UBTTask_PRFaeRoyalArcherAirMove::FinishAirMove(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result)
{
	CleanupAirMove(OwnerComp, Result);
	FinishLatentTask(OwnerComp, Result);
}

void UBTTask_PRFaeRoyalArcherAirMove::CleanupAirMove(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result) const
{
	APawn* ControlledPawn = IsValid(OwnerComp.GetAIOwner()) ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (bStopMovementOnFinish)
	{
		if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
		{
			if (UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
			}
		}
	}

	if (bClearPresentationOnFinish
		|| MoveMode == EPRFaeRoyalArcherAirMoveMode::ReturnHome
		|| MoveMode == EPRFaeRoyalArcherAirMoveMode::PatrolHome
		|| Result != EBTNodeResult::Succeeded)
	{
		ClearPresentationContext(OwnerComp);
	}
}

// ===== 표현 문맥 =====

void UBTTask_PRFaeRoyalArcherAirMove::ApplyPresentationContext(
	UBehaviorTreeComponent& OwnerComp,
	AActor* CurrentTarget,
	const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const
{
	APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(EnemyAIController))
	{
		return;
	}

	if (MoveMode == EPRFaeRoyalArcherAirMoveMode::ReturnHome
		|| MoveMode == EPRFaeRoyalArcherAirMoveMode::PatrolHome)
	{
		EnemyAIController->ClearCombatMovePresentationContext(true);
		return;
	}

	EnemyAIController->ApplyCombatMovePresentationContext(CurrentTarget, CombatDataAsset.AirCombatPresentationConfig);
}

void UBTTask_PRFaeRoyalArcherAirMove::ClearPresentationContext(UBehaviorTreeComponent& OwnerComp) const
{
	APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner());
	if (IsValid(EnemyAIController))
	{
		EnemyAIController->ClearCombatMovePresentationContext(true);
	}
}

// ===== 상태 및 Blackboard 유틸리티 =====

bool UBTTask_PRFaeRoyalArcherAirMove::HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const
{
	return IsValid(BlackboardComponent)
		&& KeyName != NAME_None
		&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
}

bool UBTTask_PRFaeRoyalArcherAirMove::DoesMoveModeRequireTarget() const
{
	return MoveMode == EPRFaeRoyalArcherAirMoveMode::CombatStrafe
		|| MoveMode == EPRFaeRoyalArcherAirMoveMode::LosReposition
		|| MoveMode == EPRFaeRoyalArcherAirMoveMode::CloseEvade;
}

FVector UBTTask_PRFaeRoyalArcherAirMove::ReadHomeLocation(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const
{
	if (HasBlackboardKey(BlackboardComponent, HomeLocationKey) && BlackboardComponent->IsVectorValueSet(HomeLocationKey))
	{
		return BlackboardComponent->GetValueAsVector(HomeLocationKey);
	}

	return IsValid(ControlledPawn) ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
}

FVector UBTTask_PRFaeRoyalArcherAirMove::ReadSearchAnchorLocation(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const
{
	if (HasBlackboardKey(BlackboardComponent, LastKnownTargetLocationKey) && BlackboardComponent->IsVectorValueSet(LastKnownTargetLocationKey))
	{
		return BlackboardComponent->GetValueAsVector(LastKnownTargetLocationKey);
	}

	if (HasBlackboardKey(BlackboardComponent, TargetLocationKey) && BlackboardComponent->IsVectorValueSet(TargetLocationKey))
	{
		return BlackboardComponent->GetValueAsVector(TargetLocationKey);
	}

	return ReadHomeLocation(BlackboardComponent, ControlledPawn);
}

bool UBTTask_PRFaeRoyalArcherAirMove::IsDisabledByGameplayState(const APawn* ControlledPawn) const
{
	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (EnemyInterface == nullptr)
	{
		return false;
	}

	const UPRAbilitySystemComponent* AbilitySystemComponent = EnemyInterface->GetEnemyAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	return AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning);
}
