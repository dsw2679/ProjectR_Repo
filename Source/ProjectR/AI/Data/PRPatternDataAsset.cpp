// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피격 경직 및 조준 반응 조건 연동)
// Author: 손승우 (아머드 솔저 및 일반 몬스터 공격 패턴 데이터 설계)
#include "PRPatternDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UPRPatternDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 RuleIndex = 0; RuleIndex < PatternRules.Num(); ++RuleIndex)
	{
		const FPRPatternRule& Rule = PatternRules[RuleIndex];
		if (!Rule.AbilityTag.IsValid())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]에는 유효한 AbilityTag가 필요합니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.MinRange > Rule.MaxRange)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]의 MinRange는 MaxRange보다 클 수 없습니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.SelectionWeight <= 0.0f)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]의 SelectionWeight는 0보다 커야 합니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.RequiredAttackPressure < 0.0f)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]의 RequiredAttackPressure는 0보다 작을 수 없습니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.bRestrictTacticalModes && Rule.AllowedTacticalModes.IsEmpty())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]가 전술 상태 제한을 사용하지만 AllowedTacticalModes가 비어 있습니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.bRestrictBossPhases && Rule.AllowedBossPhases.IsEmpty())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]가 보스 페이즈 제한을 사용하지만 AllowedBossPhases가 비어 있습니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
