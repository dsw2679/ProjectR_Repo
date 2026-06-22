// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (연사 반동 애니메이션 및 경직 제어 구현)
// Author: 배유찬 (연사 투사체 사격 제어 및 탄약 소모 연동 구현)
#include "PRGA_FireFullAuto.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_RepeatFire.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

UPRGA_FireFullAuto::UPRGA_FireFullAuto()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	SetAssetTags(DefaultAbilityTags);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
}

void UPRGA_FireFullAuto::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// cost는 per-shot에서 CommitAbilityCost로 처리하므로 활성화 단계는 검증만
	if (!CheckCost(Handle, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// FireInterval 주기로 FireOneShot 반복
	if (UPRAT_RepeatFire* RepeatTask = UPRAT_RepeatFire::RepeatFire(this, CachedFireInterval, bFireOnActivate))
	{
		RepeatTask->OnPerform.AddDynamic(this, &UPRGA_FireFullAuto::FireHitScan);
		RepeatTask->ReadyForActivation();
	}

	// 인풋 릴리즈 시 종료
	if (UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this))
	{
		ReleaseTask->OnRelease.AddDynamic(this, &UPRGA_FireFullAuto::HandleInputRelease);
		ReleaseTask->ReadyForActivation();
	}
}

void UPRGA_FireFullAuto::HandleInputRelease(float TimeHeld)
{
	ResetConsecutiveShots();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
