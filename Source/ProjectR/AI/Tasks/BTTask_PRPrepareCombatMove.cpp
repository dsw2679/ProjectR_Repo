// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Prepare 전투 Move 비헤이비어 트리 태스크 구현)
#include "BTTask_PRPrepareCombatMove.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "ProjectR/AI/Data/PRBossCombatDataAsset.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasMoveGoalBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	const FPREnemyMoveQueryConfig* GetMoveQueryConfig(
		const UPRCombatMoveDataAsset* CombatDataAsset,
		EPRCombatMoveQueryType SelectionMode)
	{
		if (!IsValid(CombatDataAsset))
		{
			return nullptr;
		}

		if (const UPRBossCombatDataAsset* BossCombatDataAsset = Cast<UPRBossCombatDataAsset>(CombatDataAsset))
		{
			if (SelectionMode == EPRCombatMoveQueryType::PatternFallbackReposition
				|| SelectionMode == EPRCombatMoveQueryType::Strafe)
			{
				return &BossCombatDataAsset->PatternFallbackRepositionConfig;
			}

			return nullptr;
		}

		const UPREnemyCombatDataAsset* EnemyCombatDataAsset = Cast<UPREnemyCombatDataAsset>(CombatDataAsset);
		if (!IsValid(EnemyCombatDataAsset))
		{
			return nullptr;
		}

		switch (SelectionMode)
		{
		case EPRCombatMoveQueryType::ApproachMeleeRange:
			return &EnemyCombatDataAsset->ApproachMeleeRangeConfig;
		case EPRCombatMoveQueryType::FastApproach:
			return &EnemyCombatDataAsset->FastApproachConfig;
		case EPRCombatMoveQueryType::PatternFallbackReposition:
		case EPRCombatMoveQueryType::Strafe:
		default:
			return &EnemyCombatDataAsset->CombatStrafeConfig;
		}
	}

	const UPRCombatMoveDataAsset* ResolveCombatDataAsset(const APawn* ControlledPawn, const UPRCombatMoveDataAsset* OverrideAsset)
	{
		if (IsValid(OverrideAsset))
		{
			return OverrideAsset;
		}

		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return nullptr;
		}

		return EnemyInterface->GetCombatDataAsset();
	}

	bool RunMoveGoalEQS(APawn* ControlledPawn, const FPREnemyMoveQueryConfig& Config, FVector& OutLocation)
	{
		return PREnemyEQSSelectionUtils::RunLocationQuery(
			ControlledPawn,
			Config.QueryTemplate.Get(),
			Config.FloatParams,
			Config.QueryRunMode,
			Config.CandidateSelectionMode,
			Config.TopCandidateCount,
			Config.TopScoreCandidateRatio,
			OutLocation);
	}
}

UBTTask_PRPrepareCombatMove::UBTTask_PRPrepareCombatMove()
{
	NodeName = TEXT("Prepare Combat Move");
}

EBTNodeResult::Type UBTTask_PRPrepareCombatMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent)
		|| !IsValid(ControlledPawn)
		|| !HasMoveGoalBlackboardKey(BlackboardComponent, CurrentTargetKey)
		|| !HasMoveGoalBlackboardKey(BlackboardComponent, MoveGoalLocationKey))
	{
		return EBTNodeResult::Failed;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	if (!IsValid(CurrentTarget))
	{
		return EBTNodeResult::Failed;
	}

	const UPRCombatMoveDataAsset* ResolvedCombatDataAsset = ResolveCombatDataAsset(ControlledPawn, CombatDataAsset);
	const FPREnemyMoveQueryConfig* MoveQueryConfig = GetMoveQueryConfig(ResolvedCombatDataAsset, SelectionMode);
	if (MoveQueryConfig == nullptr || !IsValid(MoveQueryConfig->QueryTemplate.Get()))
	{
		return EBTNodeResult::Failed;
	}

	FVector MoveGoalLocation = FVector::ZeroVector;
	if (!RunMoveGoalEQS(ControlledPawn, *MoveQueryConfig, MoveGoalLocation))
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Warning,
				TEXT("[CombatMove] Query failed Mode=%s Pawn=%s"),
				*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
				*GetNameSafe(ControlledPawn));
		}
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, MoveGoalLocation);

	if (bSetTacticalModeOnSuccess && HasMoveGoalBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(AIController))
		{
			EnemyAIController->ApplyTacticalModeState(
				TacticalModeOnSuccess,
				CurrentTarget,
				&MoveQueryConfig->PresentationConfig);
		}
		else
		{
			BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSuccess));
		}
	}
	else if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(AIController))
	{
		EnemyAIController->ApplyCombatMovePresentationContext(CurrentTarget, MoveQueryConfig->PresentationConfig);
	}

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[CombatMove] Success Mode=%s Goal=%s TacticalModeWrite=%s Pawn=%s"),
			*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
			*MoveGoalLocation.ToCompactString(),
			bSetTacticalModeOnSuccess
				? *StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalModeOnSuccess))
				: TEXT("None"),
			*GetNameSafe(ControlledPawn));
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRPrepareCombatMove::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
		*MoveGoalLocationKey.ToString());
}
