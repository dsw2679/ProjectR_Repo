// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 순간이동 다운 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinTeleportDownSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinTeleportDownSequence::UPRGameplayAbility_FaerinTeleportDownSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_TeleportDownSequence));
}
