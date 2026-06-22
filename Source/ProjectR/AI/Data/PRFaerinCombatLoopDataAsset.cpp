// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 전투 Loop 데이터 에셋 구현)
#include "PRFaerinCombatLoopDataAsset.h"

#include "GameplayAbilitySpec.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

namespace
{
	EPRBossPhase ResolveValidationPhase(UPRAbilitySystemComponent* AbilitySystemComponent)
	{
		if (!IsValid(AbilitySystemComponent))
		{
			return EPRBossPhase::Phase1;
		}

		const APRBossBaseCharacter* BossAvatar = Cast<APRBossBaseCharacter>(AbilitySystemComponent->GetAvatarActor());
		return IsValid(BossAvatar)
			? BossAvatar->GetCurrentPhase()
			: EPRBossPhase::Phase1;
	}

	bool IsPatternRuleAllowedInPhase(const FPRPatternRule& PatternRule, EPRBossPhase Phase)
	{
		return !PatternRule.bRestrictBossPhases || PatternRule.AllowedBossPhases.Contains(Phase);
	}

	bool ShouldValidateAbilitySpecForPhase(
		const FGameplayTag& AbilityTag,
		const UPRPatternDataAsset* PatternDataAsset,
		EPRBossPhase ValidationPhase)
	{
		if (!IsValid(PatternDataAsset))
		{
			return true;
		}

		bool bHasMatchingRule = false;
		for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
		{
			if (!PatternRule.IsValid() || !PatternRule.AbilityTag.MatchesTagExact(AbilityTag))
			{
				continue;
			}

			bHasMatchingRule = true;
			if (IsPatternRuleAllowedInPhase(PatternRule, ValidationPhase))
			{
				return true;
			}
		}

		return !bHasMatchingRule;
	}

	bool IsPrePatternApproachRoutePhase(EPRBossPhase Phase)
	{
		return Phase == EPRBossPhase::Phase1 || Phase == EPRBossPhase::Phase2;
	}
}

const FPRFaerinPhaseLoopConfig* UPRFaerinCombatLoopDataAsset::FindPhaseConfig(EPRBossPhase Phase) const
{
	for (const FPRFaerinPhaseLoopConfig& PhaseConfig : PhaseConfigs)
	{
		if (PhaseConfig.Phase == Phase)
		{
			return &PhaseConfig;
		}
	}

	return nullptr;
}

const FPRFaerinPatternLoopMetadata* UPRFaerinCombatLoopDataAsset::FindPatternMetadata(const FGameplayTag& AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return nullptr;
	}

	for (const FPRFaerinPatternLoopMetadata& Metadata : PatternMetadata)
	{
		if (Metadata.AbilityTag.MatchesTagExact(AbilityTag))
		{
			return &Metadata;
		}
	}

	return nullptr;
}

bool UPRFaerinCombatLoopDataAsset::ValidateLoopData(
	const UPRPatternDataAsset* PatternDataAsset,
	UPRAbilitySystemComponent* AbilitySystemComponent,
	TArray<FString>& OutErrors) const
{
	OutErrors.Reset();

	if (!IsValid(PatternDataAsset))
	{
		OutErrors.Add(TEXT("PatternDataAsset이 비어 있다."));
	}

	const bool bValidateCurrentAbilitySet = IsValid(AbilitySystemComponent);
	const EPRBossPhase ValidationPhase = ResolveValidationPhase(AbilitySystemComponent);

	TSet<FGameplayTag> PatternRuleTags;
	if (IsValid(PatternDataAsset))
	{
		for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
		{
			if (!PatternRule.IsValid())
			{
				continue;
			}

			PatternRuleTags.Add(PatternRule.AbilityTag);
			if (FindPatternMetadata(PatternRule.AbilityTag) == nullptr)
			{
				OutErrors.Add(FString::Printf(
					TEXT("PatternData Rule에 대응되는 Faerin Loop Metadata가 없다. AbilityTag=%s"),
					*PatternRule.AbilityTag.ToString()));
			}
		}
	}

	TSet<FGameplayTag> MetadataTags;
	for (const FPRFaerinPatternLoopMetadata& Metadata : PatternMetadata)
	{
		if (!Metadata.AbilityTag.IsValid())
		{
			OutErrors.Add(TEXT("Faerin Loop Metadata에 비어 있는 AbilityTag가 있다."));
			continue;
		}

		if (MetadataTags.Contains(Metadata.AbilityTag))
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin Loop Metadata에 중복 AbilityTag가 있다. AbilityTag=%s"),
				*Metadata.AbilityTag.ToString()));
		}
		MetadataTags.Add(Metadata.AbilityTag);

		if (IsValid(PatternDataAsset) && !PatternRuleTags.Contains(Metadata.AbilityTag))
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin Loop Metadata가 PatternData에 없는 AbilityTag를 참조한다. AbilityTag=%s"),
				*Metadata.AbilityTag.ToString()));
		}

		if (Metadata.TeleportWrapperPolicy != EPRFaerinTeleportWrapperPolicy::None
			&& Metadata.TeleportVFXConvergePolicy == EPRFaerinTeleportVFXConvergePolicy::TargetNearby
			&& Metadata.TeleportVFXNearbyMaxRadius < Metadata.TeleportVFXNearbyMinRadius)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin PatternMetadata의 Teleport VFX 근처 집결 최대 반경이 최소 반경보다 작다. AbilityTag=%s"),
				*Metadata.AbilityTag.ToString()));
		}

		if (bValidateCurrentAbilitySet
			&& ShouldValidateAbilitySpecForPhase(Metadata.AbilityTag, PatternDataAsset, ValidationPhase))
		{
			FGameplayTagContainer QueryTags;
			QueryTags.AddTag(Metadata.AbilityTag);

			TArray<FGameplayAbilitySpec*> MatchingSpecs;
			AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
			if (MatchingSpecs.IsEmpty())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin AbilitySet에서 Loop Metadata AbilityTag로 활성화 가능한 GA를 찾지 못했다. AbilityTag=%s"),
					*Metadata.AbilityTag.ToString()));
			}
		}
	}

	for (const FPRFaerinPhaseLoopConfig& PhaseConfig : PhaseConfigs)
	{
		if (PhaseConfig.bUsePrePatternApproachRoute && IsPrePatternApproachRoutePhase(PhaseConfig.Phase))
		{
			if (!PhaseConfig.ApproachAbilityTag.IsValid())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig 공격 전 sprint 접근 AbilityTag가 비어 있다. Phase=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
			}

			if (!PhaseConfig.NearTeleportAbilityTag.IsValid())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig 공격 전 근거리 텔레포트 AbilityTag가 비어 있다. Phase=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
			}

			if (PhaseConfig.Phase == ValidationPhase
				&& bValidateCurrentAbilitySet
				&& PhaseConfig.ApproachAbilityTag.IsValid())
			{
				FGameplayTagContainer QueryTags;
				QueryTags.AddTag(PhaseConfig.ApproachAbilityTag);

				TArray<FGameplayAbilitySpec*> MatchingSpecs;
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
				if (MatchingSpecs.IsEmpty())
				{
					OutErrors.Add(FString::Printf(
						TEXT("Faerin AbilitySet에서 공격 전 sprint 접근 GA를 찾지 못했다. Phase=%s, AbilityTag=%s"),
						*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase)),
						*PhaseConfig.ApproachAbilityTag.ToString()));
				}
			}

			if (PhaseConfig.Phase == ValidationPhase
				&& bValidateCurrentAbilitySet
				&& PhaseConfig.NearTeleportAbilityTag.IsValid())
			{
				FGameplayTagContainer QueryTags;
				QueryTags.AddTag(PhaseConfig.NearTeleportAbilityTag);

				TArray<FGameplayAbilitySpec*> MatchingSpecs;
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
				if (MatchingSpecs.IsEmpty())
				{
					OutErrors.Add(FString::Printf(
						TEXT("Faerin AbilitySet에서 공격 전 근거리 텔레포트 GA를 찾지 못했다. Phase=%s, AbilityTag=%s"),
						*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase)),
						*PhaseConfig.NearTeleportAbilityTag.ToString()));
				}
			}

			if (PhaseConfig.PrePatternApproachCloseMaxDistance < PhaseConfig.PrePatternApproachMinDistance
				|| PhaseConfig.PrePatternApproachMidMaxDistance < PhaseConfig.PrePatternApproachCloseMaxDistance)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig 공격 전 접근 거리 분기가 역전되어 있다. Phase=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
			}

			if (PhaseConfig.TargetBackTeleportDistance <= 0.0f)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig TargetBack 텔레포트 거리가 0 이하이다. Phase=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
			}
		}

		for (const FPRFaerinPeriodicSidePatternConfig& SidePatternConfig : PhaseConfig.PeriodicSidePatterns)
		{
			if (!SidePatternConfig.bEnabled)
			{
				continue;
			}

			if (!SidePatternConfig.AbilityTag.IsValid())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig의 주기 보조 패턴 AbilityTag가 비어 있다. Phase=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
				continue;
			}

			if (SidePatternConfig.IntervalSeconds <= 0.0f)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig의 주기 보조 패턴 간격이 0 이하라 매 루프마다 재시도될 수 있다. Phase=%s, AbilityTag=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase)),
					*SidePatternConfig.AbilityTag.ToString()));
			}

			if (SidePatternConfig.ActivationChance <= 0.0f)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin PhaseConfig의 주기 보조 패턴 확률이 0 이하라 실행되지 않는다. Phase=%s, AbilityTag=%s"),
					*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase)),
					*SidePatternConfig.AbilityTag.ToString()));
			}

			if (PhaseConfig.Phase == ValidationPhase && bValidateCurrentAbilitySet)
			{
				FGameplayTagContainer QueryTags;
				QueryTags.AddTag(SidePatternConfig.AbilityTag);

				TArray<FGameplayAbilitySpec*> MatchingSpecs;
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
				if (MatchingSpecs.IsEmpty())
				{
					OutErrors.Add(FString::Printf(
						TEXT("Faerin AbilitySet에서 주기 보조 패턴 GA를 찾지 못했다. Phase=%s, AbilityTag=%s"),
						*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase)),
						*SidePatternConfig.AbilityTag.ToString()));
				}
			}
		}
	}

	return OutErrors.IsEmpty();
}
