// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 Pair 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinPortalPairSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinPortalPairSequence::UPRGameplayAbility_FaerinPortalPairSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_PortalPairSequence));
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Boss_Faerin_PortalPair);

	PortalPatternType = EPRBossPortalPatternType::Pair;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::OnCharacterEvent;
	SpawnCharacterEventName = TEXT("SummonPortal_Missile");
	bEndAbilityAfterEventSpawn = false;
	bEndAbilityOnSummonMontageCompleted = true;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;

	PatternActorSpawnConfigs.Reset();

	const FName SharedPairQueryKey = TEXT("PortalPair");

	FPRBossPatternActorSpawnConfig& LeftPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	LeftPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::EnvQuery;
	LeftPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
	LeftPortalConfig.LocalOffset = FVector(0.0f, 0.0f, 450.0f);
	LeftPortalConfig.bUseWorldSpaceOffset = true;
	LeftPortalConfig.SharedEnvQueryResultKey = SharedPairQueryKey;
	LeftPortalConfig.bFaceTarget = true;
	LeftPortalConfig.bUseYawOnlyFacing = true;

	FPRBossPatternActorSpawnConfig& RightPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	RightPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::EnvQuery;
	RightPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
	RightPortalConfig.LocalOffset = FVector(0.0f, 0.0f, 900.0f);
	RightPortalConfig.bUseWorldSpaceOffset = true;
	RightPortalConfig.SharedEnvQueryResultKey = SharedPairQueryKey;
	RightPortalConfig.bFaceTarget = true;
	RightPortalConfig.bUseYawOnlyFacing = true;
}
