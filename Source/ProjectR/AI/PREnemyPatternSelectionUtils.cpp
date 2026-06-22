// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy Pattern Selection Utils 정적 유틸리티 함수 구현)
#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace PREnemyPatternSelectionUtils
{
	bool HasValidBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool MatchesPatternCategory(const FPRPatternRule& PatternRule, EPRPatternCategory CategoryFilter)
	{
		return CategoryFilter == EPRPatternCategory::Any
			|| PatternRule.PatternCategory == EPRPatternCategory::Any
			|| PatternRule.PatternCategory == CategoryFilter;
	}

	bool MatchesPatternGroupTag(const FPRPatternRule& PatternRule, const FGameplayTag PatternGroupFilter)
	{
		return !PatternGroupFilter.IsValid()
			|| PatternRule.PatternGroupTag.MatchesTag(PatternGroupFilter);
	}

	bool BuildPatternQueryRuntime(
		UBehaviorTreeComponent& OwnerComp,
		const FName CurrentTargetKey,
		const FName HasLOSKey,
		const FName ChargePathClearKey,
		const FName TacticalModeKey,
		const FName AttackPressureKey,
		FPREnemyPatternQueryRuntime& OutRuntime)
	{
		UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
		const AAIController* AIController = OwnerComp.GetAIOwner();
		APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
		IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (!IsValid(BlackboardComponent) || !IsValid(ControlledPawn) || EnemyInterface == nullptr)
		{
			return false;
		}

		UPRPatternDataAsset* PatternDataAsset = EnemyInterface->GetPatternDataAsset();
		AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
		if (!IsValid(PatternDataAsset) || !IsValid(CurrentTarget))
		{
			return false;
		}

		OutRuntime.ControlledPawn = ControlledPawn;
		OutRuntime.PatternDataAsset = PatternDataAsset;
		OutRuntime.AbilitySystemComponent = EnemyInterface->GetEnemyAbilitySystemComponent();
		OutRuntime.CurrentTarget = CurrentTarget;
		OutRuntime.PatternContext.DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
		OutRuntime.PatternContext.bHasLOS = HasValidBlackboardKey(BlackboardComponent, HasLOSKey)
			? BlackboardComponent->GetValueAsBool(HasLOSKey)
			: false;
		OutRuntime.PatternContext.TacticalMode = HasValidBlackboardKey(BlackboardComponent, TacticalModeKey)
			? static_cast<EPRTacticalMode>(BlackboardComponent->GetValueAsEnum(TacticalModeKey))
			: EPRTacticalMode::Idle;
		OutRuntime.PatternContext.bChargePathClear = HasValidBlackboardKey(BlackboardComponent, ChargePathClearKey)
			? BlackboardComponent->GetValueAsBool(ChargePathClearKey)
			: false;
		OutRuntime.PatternContext.CurrentAttackPressure = HasValidBlackboardKey(BlackboardComponent, AttackPressureKey)
			? BlackboardComponent->GetValueAsFloat(AttackPressureKey)
			: 0.0f;

		if (const APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(ControlledPawn))
		{
			OutRuntime.PatternContext.bIsBoss = true;
			OutRuntime.PatternContext.BossPhase = BossCharacter->GetCurrentPhase();
		}

		return true;
	}

	void CollectMatchingPatternRules(
		const FPREnemyPatternQueryRuntime& Runtime,
		EPRPatternCategory CategoryFilter,
		FGameplayTag PatternGroupFilter,
		bool bCheckAbilityCanActivate,
		EPRPatternContextMatchMode MatchMode,
		TArray<const FPRPatternRule*>& OutMatchedRules,
		float* OutTotalWeight)
	{
		OutMatchedRules.Reset();
		if (OutTotalWeight != nullptr)
		{
			*OutTotalWeight = 0.0f;
		}

		if (!IsValid(Runtime.PatternDataAsset))
		{
			return;
		}

		for (const FPRPatternRule& PatternRule : Runtime.PatternDataAsset->PatternRules)
		{
			if (!MatchesPatternCategory(PatternRule, CategoryFilter))
			{
				continue;
			}

			if (!MatchesPatternGroupTag(PatternRule, PatternGroupFilter))
			{
				continue;
			}

			if (!PatternRule.MatchesContext(Runtime.PatternContext, MatchMode))
			{
				continue;
			}

			if (bCheckAbilityCanActivate && IsValid(Runtime.AbilitySystemComponent))
			{
				FGameplayTagContainer FailureTags;
				if (!Runtime.AbilitySystemComponent->CanActivateAbilityByTag(PatternRule.AbilityTag, FailureTags))
				{
					continue;
				}
			}

			OutMatchedRules.Add(&PatternRule);
			if (OutTotalWeight != nullptr)
			{
				*OutTotalWeight += PatternRule.SelectionWeight;
			}
		}
	}
}
