// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Has Enemy Pattern Candidate 조건 판정용 비헤이비어 트리 데코레이터 구현)
#include "BTDecorator_PRHasEnemyPatternCandidate.h"

#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"

UBTDecorator_PRHasEnemyPatternCandidate::UBTDecorator_PRHasEnemyPatternCandidate()
{
	NodeName = TEXT("Has Enemy Pattern Candidate");
}

bool UBTDecorator_PRHasEnemyPatternCandidate::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	TArray<const FPRPatternRule*> MatchedRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		bCheckAbilityCanActivate,
		MatchMode,
		MatchedRules);

	return !MatchedRules.IsEmpty();
}

FString UBTDecorator_PRHasEnemyPatternCandidate::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nCategory: %s\nGroup: %s\nMatchMode: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
		PatternGroupFilter.IsValid() ? *PatternGroupFilter.ToString() : TEXT("None"),
		*StaticEnum<EPRPatternContextMatchMode>()->GetNameStringByValue(static_cast<int64>(MatchMode)));
}
