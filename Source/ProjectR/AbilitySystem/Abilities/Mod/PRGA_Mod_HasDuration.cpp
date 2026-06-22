// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (Mod Has Duration 구현)
#include "PRGA_Mod_HasDuration.h"

UPRGA_Mod_HasDuration::UPRGA_Mod_HasDuration()
{
	// 지속시간형 비용 정책
	SetModCostPolicy(EPRModCostPolicy::GaugeDuration);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod_HasDuration::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 지속시간 비용 확정
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 지속시간 시작 훅
	if (!GetLastAppliedModCostHandle().IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	OnDurationStarted();
	if (IsActive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
	}
}

/*~ 지속시간 처리 ~*/

void UPRGA_Mod_HasDuration::OnDurationStarted_Implementation()
{
	// 파생 클래스 확장 지점
}
