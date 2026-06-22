// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Crash(지면충돌) 드롭 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinCrashDropSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinCrashDropSequence::UPRGameplayAbility_FaerinCrashDropSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_CrashDropSequence));
	CrashDamageMode = EPRFaerinCrashDamageMode::GlobalPlayers;
}
