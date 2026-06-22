// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 여부 연동 패턴 선택 확률 보정 구현)
// Author: 손승우 (보스 및 일반 몬스터의 거리 기반 패턴 선택 알고리즘 구현)
// Author: 이건주 (Penitent 몬스터 패턴 선택 분기 구현)
#include "BTTask_PRSelectEnemyPattern.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"

namespace
{
	// 직전 패턴 태그 복원
	FGameplayTag ResolveLastUsedPatternTag(const UBlackboardComponent* BlackboardComponent, const FName LastUsedAbilityTagKey)
	{
		if (!PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, LastUsedAbilityTagKey))
		{
			return FGameplayTag();
		}

		const FName LastUsedAbilityTagName = BlackboardComponent->GetValueAsName(LastUsedAbilityTagKey);
		if (LastUsedAbilityTagName == NAME_None)
		{
			return FGameplayTag();
		}

		return FGameplayTag::RequestGameplayTag(LastUsedAbilityTagName, false);
	}
}

UBTTask_PRSelectEnemyPattern::UBTTask_PRSelectEnemyPattern()
{
	NodeName = TEXT("Select Enemy Pattern");
}

EBTNodeResult::Type UBTTask_PRSelectEnemyPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!IsValid(BlackboardComponent)
		|| !PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
			OwnerComp,
			CurrentTargetKey,
			HasLOSKey,
			ChargePathClearKey,
			TacticalModeKey,
			AttackPressureKey,
			PatternRuntime))
	{
		return EBTNodeResult::Failed;
	}

	TArray<const FPRPatternRule*> MatchedRules;
	float TotalWeight = 0.0f;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		bCheckAbilityCanActivate,
		EPRPatternContextMatchMode::FullMatch,
		MatchedRules,
		&TotalWeight);

	if (MatchedRules.IsEmpty() || TotalWeight <= 0.0f)
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Verbose,
				TEXT("[Pattern] NoCandidate Category=%s Group=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
				*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
				PatternGroupFilter.IsValid() ? *PatternGroupFilter.ToString() : TEXT("None"),
				PatternRuntime.PatternContext.DistanceToTarget,
				PatternRuntime.PatternContext.CurrentAttackPressure,
				*GetNameSafe(PatternRuntime.ControlledPawn));
		}
		return EBTNodeResult::Failed;
	}

	// 직전 사용 패턴 가중치 낮추기
	const FGameplayTag LastUsedPatternTag = bLowerLastUsedPatternWeight
		? ResolveLastUsedPatternTag(BlackboardComponent, LastUsedAbilityTagKey)
		: FGameplayTag();
	const float EffectiveLastUsedPatternWeightMultiplier = FMath::Clamp(LastUsedPatternWeightMultiplier, 0.01f, 1.0f);

	// 보정 총합 계산
	float AdjustedTotalWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		const bool bIsLastUsedPattern = LastUsedPatternTag.IsValid() && PatternRule->AbilityTag == LastUsedPatternTag;
		AdjustedTotalWeight += bIsLastUsedPattern
			? PatternRule->SelectionWeight * EffectiveLastUsedPatternWeightMultiplier
			: PatternRule->SelectionWeight;
	}

	if (AdjustedTotalWeight <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	const float PickValue = FMath::FRandRange(0.0f, AdjustedTotalWeight);
	float AccumulatedWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		// 보정 가중치 누적
		const bool bIsLastUsedPattern = LastUsedPatternTag.IsValid() && PatternRule->AbilityTag == LastUsedPatternTag;
		const float AdjustedWeight = bIsLastUsedPattern
			? PatternRule->SelectionWeight * EffectiveLastUsedPatternWeightMultiplier
			: PatternRule->SelectionWeight;
		AccumulatedWeight += AdjustedWeight;
		if (PickValue <= AccumulatedWeight)
		{
			BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, PatternRule->AbilityTag.GetTagName());
			if (PREnemyAIDebug::IsPatternLogEnabled())
			{
				UE_LOG(
					LogPREnemyAI,
					Verbose,
					TEXT("[Pattern] Selected Category=%s Group=%s Tag=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
					*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
					PatternGroupFilter.IsValid() ? *PatternGroupFilter.ToString() : TEXT("None"),
					*PatternRule->AbilityTag.ToString(),
					PatternRuntime.PatternContext.DistanceToTarget,
					PatternRuntime.PatternContext.CurrentAttackPressure,
					*GetNameSafe(PatternRuntime.ControlledPawn));
			}

			if (bSetTacticalModeOnSelection && PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, TacticalModeKey))
			{
				if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
				{
					EnemyAIController->ApplyTacticalModeState(TacticalModeOnSelection, PatternRuntime.CurrentTarget);
				}
				else
				{
					BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
				}
			}

			return EBTNodeResult::Succeeded;
		}
	}

	BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, MatchedRules.Last()->AbilityTag.GetTagName());
	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Pattern] SelectedFallback Category=%s Group=%s Tag=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
			*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
			PatternGroupFilter.IsValid() ? *PatternGroupFilter.ToString() : TEXT("None"),
			*MatchedRules.Last()->AbilityTag.ToString(),
			PatternRuntime.PatternContext.DistanceToTarget,
			PatternRuntime.PatternContext.CurrentAttackPressure,
			*GetNameSafe(PatternRuntime.ControlledPawn));
	}

	if (bSetTacticalModeOnSelection && PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
		{
			EnemyAIController->ApplyTacticalModeState(TacticalModeOnSelection, PatternRuntime.CurrentTarget);
		}
		else
		{
			BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
		}
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSelectEnemyPattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nCategory: %s\nGroup: %s\nWrite Key: %s\nLower Last Used: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
		PatternGroupFilter.IsValid() ? *PatternGroupFilter.ToString() : TEXT("None"),
		*SelectedAbilityTagKey.ToString(),
		bLowerLastUsedPatternWeight
			? *FString::Printf(TEXT("%s x%.2f"), *LastUsedAbilityTagKey.ToString(), LastUsedPatternWeightMultiplier)
			: TEXT("false"));
}
