// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Pattern 기본 구조 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_BossPatternBase.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_BossPatternBase::UPRGameplayAbility_BossPatternBase()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(FGameplayTag()));

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
}

APRBossBaseCharacter* UPRGameplayAbility_BossPatternBase::GetBossAvatarCharacter() const
{
	return Cast<APRBossBaseCharacter>(GetAvatarActorFromActorInfo());
}

bool UPRGameplayAbility_BossPatternBase::CanRunBossPattern() const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	return IsValid(BossCharacter)
		&& IsValid(AbilitySystemComponent)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning);
}

AActor* UPRGameplayAbility_BossPatternBase::GetBossPatternTarget() const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return nullptr;
	}

	const UPREnemyThreatComponent* ThreatComponent = BossCharacter->GetEnemyThreatComponent();
	return IsValid(ThreatComponent) ? ThreatComponent->GetAttackTarget() : nullptr;
}

void UPRGameplayAbility_BossPatternBase::BeginBossPatternAttackCommit()
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	UPREnemyThreatComponent* ThreatComponent = IsValid(BossCharacter)
		? BossCharacter->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	AActor* TargetActor = ThreatComponent->GetAttackTarget();
	const FVector TargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : FVector::ZeroVector;
	ThreatComponent->BeginAttackCommit(TargetActor, TargetLocation);
}

void UPRGameplayAbility_BossPatternBase::EndBossPatternAttackCommit()
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	UPREnemyThreatComponent* ThreatComponent = IsValid(BossCharacter)
		? BossCharacter->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	ThreatComponent->EndAttackCommit();
}
