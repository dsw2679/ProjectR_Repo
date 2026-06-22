// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Phase Transition 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_BossPhaseTransition.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	EPRBossPhase ResolveBossPhaseFromEvent(const FGameplayEventData* TriggerEventData, EPRBossPhase FallbackPhase)
	{
		if (TriggerEventData == nullptr)
		{
			return FallbackPhase;
		}

		const UEnum* BossPhaseEnum = StaticEnum<EPRBossPhase>();
		if (BossPhaseEnum == nullptr)
		{
			return FallbackPhase;
		}

		const int32 EventPhaseValue = FMath::RoundToInt(TriggerEventData->EventMagnitude);
		if (!BossPhaseEnum->IsValidEnumValue(EventPhaseValue))
		{
			return FallbackPhase;
		}

		return static_cast<EPRBossPhase>(EventPhaseValue);
	}
}

UPRGameplayAbility_BossPhaseTransition::UPRGameplayAbility_BossPhaseTransition()
{
	SetAssetTags(PRBossAbility::MakePhaseTransitionAssetTags());

	// 보스가 PhaseTransition GameplayEvent를 보내면 이 Ability가 자동 실행된다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_PhaseTransition;
	AbilityTriggers.Add(TriggerData);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
}

void UPRGameplayAbility_BossPhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(GetAvatarActorFromActorInfo()))
	{
		// 현재는 전환 연출 없이 즉시 확정한다. 연출이 붙으면 해당 연출 몽타주 완료 시점으로 옮기면 된다.
		BossCharacter->CommitPhaseTransition(ResolveBossPhaseFromEvent(TriggerEventData, TargetPhase));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
