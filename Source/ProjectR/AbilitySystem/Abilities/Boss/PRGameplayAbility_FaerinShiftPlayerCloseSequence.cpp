// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 플레이어 인력(Shift) Player Close 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinShiftPlayerCloseSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinShiftPlayerCloseSequence::UPRGameplayAbility_FaerinShiftPlayerCloseSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_ShiftPlayerClose));

	ShiftCharacterEventName = TEXT("ShiftPlayerClose");
	bRequireCharacterEventForShift = true;

	DestinationMode = EPRFaerinShiftDestinationMode::DirectionFromBossToTarget;
	bFallbackToDirectionalDestinationWhenQueryFails = true;
	DirectionalDistanceFromBoss = 500.0f;

	TargetMoveMode = EPRFaerinShiftMoveMode::SmoothPull;
	SmoothPullDuration = 0.3f;
	SmoothPullTickInterval = 0.016f;
	SmoothPullEaseExponent = 2.0f;
	bSweepTargetMove = false;
	bStopTargetMovementAfterShift = true;
	bFaceBossAfterShift = true;
	bDodgingTargetAvoidsShift = true;
	bMirrorTargetMoveToOwningClient = true;
	ShiftImpactPoiseDamage = 100.0f;
	bGrantOwnerInvulnerabilityDuringShift = true;
	OwnerInvulnerabilityMinimumPhase = EPRBossPhase::Phase3;
}
