// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Fire Single 어빌리티 구현)
#include "PRGA_FireSingle.h"

#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

UPRGA_FireSingle::UPRGA_FireSingle()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	SetAssetTags(DefaultAbilityTags);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
}

void UPRGA_FireSingle::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	FireHitScan();
	CommitAbilityCooldown(Handle,ActorInfo,ActivationInfo,true);
	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
