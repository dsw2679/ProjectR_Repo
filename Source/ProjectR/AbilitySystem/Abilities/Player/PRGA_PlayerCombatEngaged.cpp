// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (전투 상태 중 스태미나 회복 제어 구현)
// Author: 배유찬 (보스 조우 시 전투 상태 전환 보정 구현)
#include "PRGA_PlayerCombatEngaged.h"

#include "GameplayEffect.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_PlayerCombatEngaged::UPRGA_PlayerCombatEngaged()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_CombatEngaged);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData CombatEngagedTriggerData;
	CombatEngagedTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	CombatEngagedTriggerData.TriggerTag = PRGameplayTags::Event_Combat_Engaged;
	AbilityTriggers.Add(CombatEngagedTriggerData);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationPolicy = EPRAbilityActivationPolicy::None;
}

void UPRGA_PlayerCombatEngaged::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyCombatingStateEffect();
	if (TriggerEventData != nullptr)
	{
		// 전투 교전 이벤트를 보고한 보스의 HUD 조우 시작 요청
		const AActor* EventInstigator = TriggerEventData->Instigator.Get();
		if (APRBossBaseCharacter* Boss = Cast<APRBossBaseCharacter>(const_cast<AActor*>(EventInstigator)))
		{
			Boss->RequestBossEncounterBegin();
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UPRGA_PlayerCombatEngaged::ApplyCombatingStateEffect()
{
	if (!IsValid(CombatingStateEffectClass))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CombatingStateEffectClass);
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}
