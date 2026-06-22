// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Crash(지면충돌) 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinCrashSequence.h"

#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "UObject/SoftObjectPath.h"

UPRGameplayAbility_FaerinCrashSequence::UPRGameplayAbility_FaerinCrashSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_CrashSequence));
	bRequireCharacterEventRouter = true;
}

void UPRGameplayAbility_FaerinCrashSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	DamagedActorsThisCrash.Reset();
	bCrashDamageApplied = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinCrashSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	DamagedActorsThisCrash.Reset();
	bCrashDamageApplied = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_FaerinCrashSequence::NativeOnStageCharacterEvent(
	const FPRFaerinStagedMontageStage& Stage,
	const FName EventName)
{
	(void)Stage;

	if (EventName != CrashAreaEventName)
	{
		return;
	}

	if (bApplyCrashDamageOncePerActivation && bCrashDamageApplied)
	{
		return;
	}

	SpawnCrashImpactVFX();
	ApplyCrashAreaDamage();
	bCrashDamageApplied = true;
}

FVector UPRGameplayAbility_FaerinCrashSequence::ResolveCrashAreaCenter() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return FVector::ZeroVector;
	}

	return AvatarActor->GetActorTransform().TransformPositionNoScale(CrashAreaLocalOffset);
}

void UPRGameplayAbility_FaerinCrashSequence::SpawnCrashImpactVFX() const
{
	if (!IsValid(CrashImpactNiagaraSystem))
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority())
	{
		return;
	}

	FVector ImpactLocation = FVector::ZeroVector;
	if (!ResolveCrashImpactLocation(ImpactLocation))
	{
		return;
	}

	const FSoftObjectPath ImpactSystemPath(CrashImpactNiagaraSystem.Get());
	FaerinCharacter->Multicast_SpawnFaerinWorldNiagara(
		ImpactSystemPath,
		ImpactLocation,
		CrashImpactNiagaraRotationOffset,
		CrashImpactNiagaraScale,
		CrashImpactNiagaraLifeSeconds);
}

bool UPRGameplayAbility_FaerinCrashSequence::ResolveCrashImpactLocation(FVector& OutLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	UWorld* World = GetWorld();
	const FVector BossLocation = AvatarActor->GetActorLocation();
	if (!IsValid(World))
	{
		OutLocation = BossLocation + CrashImpactLocationOffset;
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinCrashImpactGroundTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	const FVector TraceStart = BossLocation + FVector(0.0f, 0.0f, CrashImpactGroundTraceUpOffset);
	const FVector TraceEnd = BossLocation - FVector(0.0f, 0.0f, CrashImpactGroundTraceDownOffset);

	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		CrashImpactGroundTraceChannel,
		QueryParams))
	{
		OutLocation = HitResult.ImpactPoint + CrashImpactLocationOffset;
		return true;
	}

	OutLocation = BossLocation + CrashImpactLocationOffset;
	return true;
}

void UPRGameplayAbility_FaerinCrashSequence::ApplyCrashAreaDamage()
{
	if (CrashDamageMode == EPRFaerinCrashDamageMode::GlobalPlayers)
	{
		ApplyCrashGlobalPlayerDamage();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(World) || CrashDamageRadius <= 0.0f)
	{
		return;
	}

	const FVector CrashCenter = ResolveCrashAreaCenter();
	const FCollisionShape CrashShape = FCollisionShape::MakeSphere(CrashDamageRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinCrashAreaDamage), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.bFindInitialOverlaps = true;

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(
		OverlapResults,
		CrashCenter,
		FQuat::Identity,
		CrashOverlapChannel,
		CrashShape,
		QueryParams);

	if (bDrawDebugCrashArea)
	{
		DrawDebugSphere(World, CrashCenter, CrashDamageRadius, 32, FColor::Red, false, CrashDebugDrawDuration);
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* HitActor = OverlapResult.GetActor();
		const TWeakObjectPtr<AActor> HitActorKey(HitActor);
		if (!IsValid(HitActor) || HitActor == AvatarActor || DamagedActorsThisCrash.Contains(HitActorKey))
		{
			continue;
		}

		ApplyAttackPowerDamage(HitActor, CrashDamageMultiplier, CrashPoiseDamage);
		DamagedActorsThisCrash.Add(HitActorKey);
	}
}

void UPRGameplayAbility_FaerinCrashSequence::ApplyCrashGlobalPlayerDamage()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		APawn* TargetPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
		const TWeakObjectPtr<AActor> TargetKey(TargetPawn);
		if (!IsValid(TargetPawn)
			|| TargetPawn == AvatarActor
			|| DamagedActorsThisCrash.Contains(TargetKey))
		{
			continue;
		}

		ApplyAttackPowerDamage(TargetPawn, CrashDamageMultiplier, CrashPoiseDamage);
		DamagedActorsThisCrash.Add(TargetKey);
	}
}
