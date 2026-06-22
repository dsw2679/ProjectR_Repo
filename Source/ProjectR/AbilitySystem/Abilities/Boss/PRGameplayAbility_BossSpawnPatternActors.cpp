// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 스폰 Pattern Actors 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_BossSpawnPatternActors.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossSpawnPatternActors, Log, All);

UPRGameplayAbility_BossSpawnPatternActors::UPRGameplayAbility_BossSpawnPatternActors()
{
}

void UPRGameplayAbility_BossSpawnPatternActors::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanRunBossPattern() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpawnedPatternActors.Reset();
	ResetSharedEnvQueryResultCache();

	for (const FPRBossPatternActorSpawnConfig& SpawnConfig : PatternActorSpawnConfigs)
	{
		if (APRBossPatternActor* SpawnedActor = SpawnPatternActor(SpawnConfig))
		{
			SpawnedPatternActors.Add(SpawnedActor);
		}
	}

	TArray<APRBossPatternActor*> SpawnedActorsForEvent;
	SpawnedActorsForEvent.Reserve(SpawnedPatternActors.Num());
	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActorsForEvent.Add(SpawnedActor);
		}
	}

	BP_OnPatternActorsSpawned(SpawnedActorsForEvent);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UPRGameplayAbility_BossSpawnPatternActors::BuildPatternActorSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform)
{
	if (SpawnConfig.SpawnLocationMode != EPRBossPatternSpawnLocationMode::EnvQuery)
	{
		return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
	}

	FVector BaseLocation = FVector::ZeroVector;
	if (!ResolveEnvQueryBaseLocation(SpawnConfig, BaseLocation))
	{
		if (SpawnConfig.bFallbackToOriginOffsetWhenQueryFails)
		{
			return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
		}

		return false;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();
	if (!IsValid(BossCharacter))
	{
		return false;
	}

	FRotator OffsetRotation = BossCharacter->GetActorRotation();
	if (SpawnConfig.bFaceTarget && IsValid(PatternTarget))
	{
		const FVector DirectionToTarget = PatternTarget->GetActorLocation() - BaseLocation;
		if (!DirectionToTarget.IsNearlyZero())
		{
			OffsetRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			if (SpawnConfig.bUseYawOnlyFacing)
			{
				OffsetRotation.Pitch = 0.0f;
				OffsetRotation.Roll = 0.0f;
			}
		}
	}

	const FVector SpawnLocation = ApplySpawnLocationOffset(SpawnConfig, BaseLocation, OffsetRotation);
	const FRotator SpawnRotation = BuildSpawnRotation(SpawnConfig, SpawnLocation, OffsetRotation);
	OutSpawnTransform = FTransform(SpawnRotation + SpawnConfig.RotationOffset, SpawnLocation);
	return true;
}

bool UPRGameplayAbility_BossSpawnPatternActors::BuildOriginOffsetSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();

	if (!IsValid(BossCharacter))
	{
		return false;
	}

	const USceneComponent* OriginComponent = ResolveSpawnOriginComponent(SpawnConfig);
	if (!IsValid(OriginComponent))
	{
		return false;
	}

	const FTransform OriginTransform = OriginComponent->GetComponentTransform();
	const FRotator OriginRotation = OriginTransform.GetRotation().Rotator();
	const FVector SpawnLocation = ApplySpawnLocationOffset(
		SpawnConfig,
		OriginComponent->GetComponentLocation(),
		OriginRotation);
	const FRotator BaseRotation = SpawnConfig.bUseWorldSpaceRotation ? FRotator::ZeroRotator : OriginRotation;
	const FRotator SpawnRotation = BuildSpawnRotation(SpawnConfig, SpawnLocation, BaseRotation);

	OutSpawnTransform = FTransform(SpawnRotation + SpawnConfig.RotationOffset, SpawnLocation);
	return true;
}

bool UPRGameplayAbility_BossSpawnPatternActors::RunSpawnLocationQuery(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FVector& OutSpawnLocation) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return false;
	}

	if (!PREnemyEQSSelectionUtils::RunLocationQuery(
			BossCharacter,
			SpawnConfig.SpawnQueryTemplate.Get(),
			SpawnConfig.FloatParams,
			SpawnConfig.SpawnQueryRunMode,
			SpawnConfig.CandidateSelectionMode,
			SpawnConfig.TopCandidateCount,
			SpawnConfig.TopScoreCandidateRatio,
			OutSpawnLocation))
	{
		UE_LOG(LogPRBossSpawnPatternActors, Verbose,
			TEXT("Boss pattern spawn EQS failed. Ability=%s, Query=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnConfig.SpawnQueryTemplate.Get()));
		return false;
	}

	return true;
}

bool UPRGameplayAbility_BossSpawnPatternActors::ResolveEnvQueryBaseLocation(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FVector& OutBaseLocation)
{
	if (!SpawnConfig.SharedEnvQueryResultKey.IsNone())
	{
		if (const FVector* CachedLocation = SharedEnvQueryResultLocations.Find(SpawnConfig.SharedEnvQueryResultKey))
		{
			OutBaseLocation = *CachedLocation;
			return true;
		}
	}

	if (!RunSpawnLocationQuery(SpawnConfig, OutBaseLocation))
	{
		return false;
	}

	if (!SpawnConfig.SharedEnvQueryResultKey.IsNone())
	{
		SharedEnvQueryResultLocations.Add(SpawnConfig.SharedEnvQueryResultKey, OutBaseLocation);
	}

	return true;
}

FVector UPRGameplayAbility_BossSpawnPatternActors::ApplySpawnLocationOffset(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	const FVector& BaseLocation,
	const FRotator& OffsetRotation) const
{
	if (SpawnConfig.bUseWorldSpaceOffset)
	{
		return BaseLocation + SpawnConfig.LocalOffset;
	}

	return BaseLocation + OffsetRotation.RotateVector(SpawnConfig.LocalOffset);
}

FRotator UPRGameplayAbility_BossSpawnPatternActors::BuildSpawnRotation(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	const FVector& SpawnLocation,
	const FRotator& BaseRotation) const
{
	AActor* PatternTarget = GetBossPatternTarget();
	FRotator SpawnRotation = SpawnConfig.bUseWorldSpaceRotation ? FRotator::ZeroRotator : BaseRotation;
	if (SpawnConfig.bFaceTarget && IsValid(PatternTarget))
	{
		const FVector DirectionToTarget = PatternTarget->GetActorLocation() - SpawnLocation;
		if (!DirectionToTarget.IsNearlyZero())
		{
			SpawnRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			if (SpawnConfig.bUseYawOnlyFacing)
			{
				SpawnRotation.Pitch = 0.0f;
				SpawnRotation.Roll = 0.0f;
			}
		}
	}

	return SpawnRotation;
}

APRBossPatternActor* UPRGameplayAbility_BossSpawnPatternActors::SpawnPatternActor(
	const FPRBossPatternActorSpawnConfig& SpawnConfig)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !SpawnConfig.PatternActorClass)
	{
		return nullptr;
	}

	UWorld* World = BossCharacter->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!BuildPatternActorSpawnTransform(SpawnConfig, SpawnTransform))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = BossCharacter;
	SpawnParameters.Instigator = BossCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRBossPatternActor* SpawnedActor = World->SpawnActor<APRBossPatternActor>(
		SpawnConfig.PatternActorClass,
		SpawnTransform,
		SpawnParameters);

	if (IsValid(SpawnedActor))
	{
		SpawnedActor->InitializeBossPatternActor(BossCharacter, GetBossPatternTarget());
		ApplyPostSpawnAttachment(SpawnedActor, SpawnConfig);
	}

	return SpawnedActor;
}

AActor* UPRGameplayAbility_BossSpawnPatternActors::ResolveSpawnOriginActor(
	const FPRBossPatternActorSpawnConfig& SpawnConfig) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return nullptr;
	}

	AActor* PatternTarget = GetBossPatternTarget();
	if (SpawnConfig.SpawnOrigin == EPRBossPatternSpawnOrigin::Target && IsValid(PatternTarget))
	{
		return PatternTarget;
	}

	return BossCharacter;
}

USceneComponent* UPRGameplayAbility_BossSpawnPatternActors::ResolveSpawnOriginComponent(
	const FPRBossPatternActorSpawnConfig& SpawnConfig) const
{
	AActor* OriginActor = ResolveSpawnOriginActor(SpawnConfig);
	if (!IsValid(OriginActor))
	{
		return nullptr;
	}

	if (SpawnConfig.OriginComponentName.IsNone())
	{
		return OriginActor->GetRootComponent();
	}

	TArray<USceneComponent*> SceneComponents;
	OriginActor->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) && SceneComponent->GetFName() == SpawnConfig.OriginComponentName)
		{
			return SceneComponent;
		}
	}

	UE_LOG(
		LogPRBossSpawnPatternActors,
		Warning,
		TEXT("Boss pattern spawn origin component not found. Ability=%s, OriginActor=%s, ComponentName=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OriginActor),
		*SpawnConfig.OriginComponentName.ToString());
	return nullptr;
}

void UPRGameplayAbility_BossSpawnPatternActors::ApplyPostSpawnAttachment(
	APRBossPatternActor* SpawnedActor,
	const FPRBossPatternActorSpawnConfig& SpawnConfig) const
{
	if (!IsValid(SpawnedActor) || !SpawnConfig.bAttachToOriginAfterSpawn)
	{
		return;
	}

	USceneComponent* OriginComponent = ResolveSpawnOriginComponent(SpawnConfig);
	if (!IsValid(OriginComponent))
	{
		return;
	}

	SpawnedActor->AttachToComponent(OriginComponent, FAttachmentTransformRules::KeepWorldTransform);
}

void UPRGameplayAbility_BossSpawnPatternActors::ResetSharedEnvQueryResultCache()
{
	SharedEnvQueryResultLocations.Reset();
}
