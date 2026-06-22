// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 탄막 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinPortalBarrageSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Boss/PRBossPortalActor.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinPortalBarrageSequence::UPRGameplayAbility_FaerinPortalBarrageSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_PortalBarrageSequence));
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Boss_Faerin_PortalBarrage);

	PortalPatternType = EPRBossPortalPatternType::Barrage;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::OnCharacterEvent;
	SpawnCharacterEventName = TEXT("SummonPortal_Torrent");
	bEndAbilityAfterEventSpawn = false;
	bEndAbilityOnSummonMontageCompleted = true;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;
}

void UPRGameplayAbility_FaerinPortalBarrageSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(LinePortalActorClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RebuildLinePortalSpawnConfigs(ResolveLineModeForActivation());
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinPortalBarrageSequence::StartSpawnedPortals()
{
	if (!bStartPortalsAfterSpawn)
	{
		return;
	}

	const int32 ResolvedGroupSize = FMath::Max(PortalStartGroupSize, 1);
	int32 PortalIndex = 0;
	for (APRBossPortalActor* SpawnedPortal : SpawnedPortalRefs)
	{
		if (!IsValid(SpawnedPortal))
		{
			continue;
		}

		const int32 GroupIndex = PortalIndex / ResolvedGroupSize;
		const float StartDelay = FMath::Max(PortalStartGroupInterval, 0.0f) * static_cast<float>(GroupIndex);
		SpawnedPortal->StartPortalTelegraphAfterDelay(StartDelay);
		++PortalIndex;
	}
}

EPRFaerinPortalBarrageLineMode UPRGameplayAbility_FaerinPortalBarrageSequence::ResolveLineModeForActivation()
{
	if (LineMode == EPRFaerinPortalBarrageLineMode::Random)
	{
		return FMath::RandBool()
			? EPRFaerinPortalBarrageLineMode::Horizontal
			: EPRFaerinPortalBarrageLineMode::Vertical;
	}

	if (LineMode == EPRFaerinPortalBarrageLineMode::Alternating)
	{
		const EPRFaerinPortalBarrageLineMode ResolvedLineMode = bNextAlternatingLineUsesHorizontal
			? EPRFaerinPortalBarrageLineMode::Horizontal
			: EPRFaerinPortalBarrageLineMode::Vertical;
		bNextAlternatingLineUsesHorizontal = !bNextAlternatingLineUsesHorizontal;
		return ResolvedLineMode;
	}

	return LineMode;
}

void UPRGameplayAbility_FaerinPortalBarrageSequence::RebuildLinePortalSpawnConfigs(
	EPRFaerinPortalBarrageLineMode ResolvedLineMode)
{
	const bool bUseHorizontalLine = ResolvedLineMode != EPRFaerinPortalBarrageLineMode::Vertical;
	const int32 DirectionSign = NextFireSideSign >= 0 ? 1 : -1;
	if (bAlternateFireSide)
	{
		NextFireSideSign *= -1;
	}

	const FVector LineAxis = bUseHorizontalLine ? FVector::YAxisVector : FVector::XAxisVector;
	const FVector FireDirection = bUseHorizontalLine
		? FVector::XAxisVector * static_cast<float>(DirectionSign)
		: FVector::YAxisVector * static_cast<float>(DirectionSign);
	const FVector SideOffset = -FireDirection * FMath::Max(LineSideDistance, 0.0f);
	const float HalfIndex = (static_cast<float>(FMath::Max(LinePortalCount, 1)) - 1.0f) * 0.5f;

	PatternActorSpawnConfigs.Reset();
	for (int32 PortalIndex = 0; PortalIndex < FMath::Max(LinePortalCount, 1); ++PortalIndex)
	{
		const float LineOffset = (static_cast<float>(PortalIndex) - HalfIndex) * FMath::Max(LinePortalSpacing, 0.0f);

		FPRBossPatternActorSpawnConfig& SpawnConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
		SpawnConfig.PatternActorClass = LinePortalActorClass;
		SpawnConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
		SpawnConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
		SpawnConfig.LocalOffset = SideOffset + (LineAxis * LineOffset) + (FVector::UpVector * LinePortalHeight);
		SpawnConfig.bUseWorldSpaceOffset = true;
		SpawnConfig.RotationOffset = FireDirection.Rotation();
		SpawnConfig.bUseWorldSpaceRotation = true;
		SpawnConfig.bFaceTarget = false;
	}
}

