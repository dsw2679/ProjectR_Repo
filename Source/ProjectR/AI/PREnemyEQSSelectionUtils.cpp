// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy EQS Selection Utils 정적 유틸리티 함수 구현)
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"

#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

namespace
{
	struct FPREnemyEQSCandidate
	{
		int32 ItemIndex = INDEX_NONE;
		float Score = 0.0f;
	};

	void BuildSortedCandidates(const FEnvQueryResult& QueryResult, TArray<FPREnemyEQSCandidate>& OutCandidates)
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryResult.Items.Num(); ++ItemIndex)
		{
			if (!QueryResult.Items[ItemIndex].IsValid())
			{
				continue;
			}

			FPREnemyEQSCandidate Candidate;
			Candidate.ItemIndex = ItemIndex;
			Candidate.Score = QueryResult.GetItemScore(ItemIndex);
			OutCandidates.Add(Candidate);
		}

		OutCandidates.Sort([](const FPREnemyEQSCandidate& Left, const FPREnemyEQSCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
	}

	void FilterTopCandidates(
		const int32 TopCandidateCount,
		const float TopScoreCandidateRatio,
		TArray<FPREnemyEQSCandidate>& Candidates)
	{
		if (Candidates.IsEmpty())
		{
			return;
		}

		const float MaxScore = Candidates[0].Score;
		const float MinAllowedScore = MaxScore * FMath::Clamp(TopScoreCandidateRatio, 0.0f, 1.0f);
		Candidates.RemoveAll([MinAllowedScore](const FPREnemyEQSCandidate& Candidate)
		{
			return Candidate.Score < MinAllowedScore;
		});

		if (TopCandidateCount > 0 && Candidates.Num() > TopCandidateCount)
		{
			Candidates.SetNum(TopCandidateCount);
		}
	}

	bool SelectCandidateIndex(
		const EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		const TArray<FPREnemyEQSCandidate>& Candidates,
		int32& OutItemIndex)
	{
		if (Candidates.IsEmpty())
		{
			return false;
		}

		switch (CandidateSelectionMode)
		{
		case EPREnemyQueryCandidateSelectionMode::RandomTopCandidates:
		{
			const int32 PickIndex = FMath::RandRange(0, Candidates.Num() - 1);
			OutItemIndex = Candidates[PickIndex].ItemIndex;
			return true;
		}
		case EPREnemyQueryCandidateSelectionMode::WeightedRandomTopCandidates:
		{
			float TotalWeight = 0.0f;
			for (const FPREnemyEQSCandidate& Candidate : Candidates)
			{
				TotalWeight += FMath::Max(Candidate.Score, KINDA_SMALL_NUMBER);
			}

			if (TotalWeight <= 0.0f)
			{
				OutItemIndex = Candidates[0].ItemIndex;
				return true;
			}

			const float PickWeight = FMath::FRandRange(0.0f, TotalWeight);
			float AccumulatedWeight = 0.0f;
			for (const FPREnemyEQSCandidate& Candidate : Candidates)
			{
				AccumulatedWeight += FMath::Max(Candidate.Score, KINDA_SMALL_NUMBER);
				if (PickWeight <= AccumulatedWeight)
				{
					OutItemIndex = Candidate.ItemIndex;
					return true;
				}
			}

			OutItemIndex = Candidates.Last().ItemIndex;
			return true;
		}
		case EPREnemyQueryCandidateSelectionMode::BestScore:
		default:
			OutItemIndex = Candidates[0].ItemIndex;
			return true;
		}
	}
}

namespace PREnemyEQSSelectionUtils
{
	bool RunLocationQuery(
		UObject* QueryOwner,
		UEnvQuery* QueryTemplate,
		const TArray<FPREnemyEQSFloatParam>& FloatParams,
		const EEnvQueryRunMode::Type QueryRunMode,
		const EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		const int32 TopCandidateCount,
		const float TopScoreCandidateRatio,
		FVector& OutLocation)
	{
		if (!IsValid(QueryOwner) || !IsValid(QueryTemplate))
		{
			return false;
		}

		UWorld* World = QueryOwner->GetWorld();
		if (!IsValid(World))
		{
			return false;
		}

		UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(World);
		if (!IsValid(QueryManager))
		{
			return false;
		}

		FEnvQueryRequest QueryRequest(QueryTemplate, QueryOwner);
		for (const FPREnemyEQSFloatParam& FloatParam : FloatParams)
		{
			if (FloatParam.ParamName == NAME_None)
			{
				continue;
			}

			QueryRequest.SetFloatParam(FloatParam.ParamName, FloatParam.Value);
		}

		const TSharedPtr<FEnvQueryResult> QueryResult = QueryManager->RunInstantQuery(
			QueryRequest,
			ResolveQueryRunMode(CandidateSelectionMode, QueryRunMode));
		if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() <= 0)
		{
			return false;
		}

		int32 SelectedItemIndex = INDEX_NONE;
		if (!SelectItemIndex(
				*QueryResult,
				CandidateSelectionMode,
				TopCandidateCount,
				TopScoreCandidateRatio,
				SelectedItemIndex)
			|| SelectedItemIndex == INDEX_NONE)
		{
			return false;
		}

		OutLocation = QueryResult->GetItemAsLocation(SelectedItemIndex);
		return true;
	}

	EEnvQueryRunMode::Type ResolveQueryRunMode(
		const EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		const EEnvQueryRunMode::Type RequestedRunMode)
	{
		if (CandidateSelectionMode == EPREnemyQueryCandidateSelectionMode::BestScore)
		{
			return RequestedRunMode;
		}

		return EEnvQueryRunMode::AllMatching;
	}

	bool SelectItemIndex(
		const FEnvQueryResult& QueryResult,
		const EPREnemyQueryCandidateSelectionMode CandidateSelectionMode,
		const int32 TopCandidateCount,
		const float TopScoreCandidateRatio,
		int32& OutItemIndex)
	{
		TArray<FPREnemyEQSCandidate> Candidates;
		BuildSortedCandidates(QueryResult, Candidates);
		FilterTopCandidates(TopCandidateCount, TopScoreCandidateRatio, Candidates);

		return SelectCandidateIndex(CandidateSelectionMode, Candidates, OutItemIndex);
	}
}
