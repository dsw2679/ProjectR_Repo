// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 비 포털 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinRainPortalSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Boss/PRBossPortalActor.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinRainProjectileManager.h"
#include "ProjectR/PRGameplayTags.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// 경량 rain 투사체 경로 전역 토글. 데이터 플래그와 별개로 런타임에 강제 켜고 끌 수 있다. (롤백용)
	TAutoConsoleVariable<int32> CVarFaerinRainPortalUseLightweight(
		TEXT("pr.Faerin.RainPortal.UseLightweight"),
		0,
		TEXT("1이면 Faerin Rain Portal이 경량 ISM 매니저 투사체 경로를 사용한다. 0이면 기존 액터 투사체 경로."),
		ECVF_Default);
}

UPRGameplayAbility_FaerinRainPortalSequence::UPRGameplayAbility_FaerinRainPortalSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_RainPortalSequence));
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Boss_Faerin_RainPortal);

	PortalPatternType = EPRBossPortalPatternType::Torrent;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::ImmediateOnActivate;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;
	DownwardPortalRotation = FVector::DownVector.Rotation();
}

void UPRGameplayAbility_FaerinRainPortalSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(RainPortalActorClass) || PortalCount <= 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RebuildRainPortalSpawnConfigs();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinRainPortalSequence::StartSpawnedPortals()
{
	if (!bStartPortalsAfterSpawn)
	{
		return;
	}

	// 경량 경로가 켜져 있으면 매니저를 1개 스폰하고, 각 포털을 경량 경로로 전환한다.
	const bool bLightweight = ResolveUseLightweightRainProjectile();
	APRFaerinRainProjectileManager* RainManager = bLightweight ? SpawnRainProjectileManager() : nullptr;

	const int32 ResolvedGroupSize = FMath::Max(PortalStartGroupSize, 1);
	int32 PortalIndex = 0;
	for (APRBossPortalActor* SpawnedPortal : SpawnedPortalRefs)
	{
		if (!IsValid(SpawnedPortal))
		{
			continue;
		}

		if (bLightweight && IsValid(RainManager))
		{
			SpawnedPortal->SetLightweightRainProjectile(true, RainManager, LightweightProjectileLifetime);
		}

		const int32 GroupIndex = PortalIndex / ResolvedGroupSize;
		const float StartDelay = FMath::Max(PortalStartGroupInterval, 0.0f) * static_cast<float>(GroupIndex);
		SpawnedPortal->StartPortalTelegraphAfterDelay(StartDelay);
		++PortalIndex;
	}
}

void UPRGameplayAbility_FaerinRainPortalSequence::RebuildRainPortalSpawnConfigs()
{
	PatternActorSpawnConfigs.Reset();

	const int32 ResolvedPortalCount = FMath::Max(PortalCount, 1);
	for (int32 PortalIndex = 0; PortalIndex < ResolvedPortalCount; ++PortalIndex)
	{
		FPRBossPatternActorSpawnConfig& SpawnConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
		SpawnConfig.PatternActorClass = RainPortalActorClass;
		SpawnConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
		SpawnConfig.SpawnOrigin = RainPortalSpawnOrigin;
		SpawnConfig.LocalOffset = BuildRainPortalOffset(PortalIndex, ResolvedPortalCount);
		SpawnConfig.bUseWorldSpaceOffset = true;
		SpawnConfig.RotationOffset = DownwardPortalRotation;
		SpawnConfig.bUseWorldSpaceRotation = true;
		SpawnConfig.bFaceTarget = false;
	}
}

FVector UPRGameplayAbility_FaerinRainPortalSequence::BuildRainPortalOffset(
	const int32 PortalIndex,
	const int32 ResolvedPortalCount) const
{
	const float OuterRadius = FMath::Max(SpawnAreaRadius, 0.0f);

	if (ResolvedPortalCount <= 1)
	{
		return FVector(0.0f, 0.0f, SpawnHeight);
	}

	constexpr float GoldenAngleRadians = 2.39996323f;
	const float NormalizedIndex = static_cast<float>(PortalIndex) / static_cast<float>(ResolvedPortalCount - 1);
	const float Radius = FMath::Sqrt(NormalizedIndex) * OuterRadius;
	const float Angle = static_cast<float>(PortalIndex) * GoldenAngleRadians;
	return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SpawnHeight);
}

bool UPRGameplayAbility_FaerinRainPortalSequence::ResolveUseLightweightRainProjectile() const
{
	if (bUseLightweightRainProjectile)
	{
		return true;
	}

	return CVarFaerinRainPortalUseLightweight.GetValueOnGameThread() != 0;
}

APRFaerinRainProjectileManager* UPRGameplayAbility_FaerinRainPortalSequence::SpawnRainProjectileManager()
{
	if (!IsValid(RainProjectileManagerClass))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FaerinRainPortal: 경량 경로가 켜졌지만 RainProjectileManagerClass가 비어 있어 기존 액터 투사체 경로로 동작한다."));
		return nullptr;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!IsValid(World) || !AvatarActor->HasAuthority())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return World->SpawnActor<APRFaerinRainProjectileManager>(
		RainProjectileManagerClass,
		AvatarActor->GetActorTransform(),
		SpawnParameters);
}
