// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 플레이어 인력(Shift) 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinShiftSequence.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinShiftSequence, Log, All);

UPRGameplayAbility_FaerinShiftSequence::UPRGameplayAbility_FaerinShiftSequence()
{
}

void UPRGameplayAbility_FaerinShiftSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !CanRunBossPattern())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBossPatternAttackCommit();

	bShiftApplied = false;
	bShiftResolved = false;
	bShiftMoveInProgress = false;
	bFinishWhenShiftMoveCompletes = false;
	bShiftSequenceFinished = false;
	bOwnerInvulnerabilityApplied = false;
	ActiveShiftTarget = nullptr;
	ActiveShiftWarningVFXKey = NAME_None;
	SmoothShiftElapsedSeconds = 0.0f;
	LastSmoothShiftUpdateTime = 0.0f;

	ApplyOwnerInvulnerabilityIfNeeded();
	SpawnShiftWarningVFX();

	if (!RegisterCharacterEventListener() && bRequireCharacterEventForShift)
	{
		UE_LOG(LogPRFaerinShiftSequence, Warning,
			TEXT("Faerin shift cannot wait for CharacterEvent because router is missing. Ability=%s, Event=%s"),
			*GetNameSafe(this),
			*ShiftCharacterEventName.ToString());
		FinishShiftSequence(true);
		return;
	}

	if (!IsValid(ShiftMontage))
	{
		if (!bRequireCharacterEventForShift)
		{
			FVector Destination = FVector::ZeroVector;
			if (ResolveShiftDestination(Destination))
			{
				const bool bShiftStartedOrApplied = ApplyShiftToTarget(Destination);
				if (!bShiftStartedOrApplied)
				{
					FinishShiftSequence(true);
					return;
				}

				if (bShiftMoveInProgress)
				{
					bShiftResolved = true;
					bFinishWhenShiftMoveCompletes = true;
					return;
				}

				bShiftApplied = true;
				bShiftResolved = true;
			}
		}

		FinishShiftSequence(!bShiftResolved);
		return;
	}

	ActiveShiftMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ShiftMontage,
		FMath::Max(ShiftMontagePlayRate, UE_SMALL_NUMBER),
		ShiftMontageStartSection);

	if (!IsValid(ActiveShiftMontageTask))
	{
		FinishShiftSequence(true);
		return;
	}

	ActiveShiftMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageCompleted);
	ActiveShiftMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageBlendOut);
	ActiveShiftMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted);
	ActiveShiftMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted);
	ActiveShiftMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_FaerinShiftSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnregisterCharacterEventListener();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SmoothShiftTimerHandle);
	}

	if (IsValid(ActiveShiftMontageTask))
	{
		ActiveShiftMontageTask->EndTask();
		ActiveShiftMontageTask = nullptr;
	}

	bShiftApplied = false;
	bShiftResolved = false;
	bShiftMoveInProgress = false;
	bFinishWhenShiftMoveCompletes = false;
	bShiftSequenceFinished = false;
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(ActiveShiftTarget.Get()))
	{
		PlayerCharacter->SetExternalForcedMoveCameraLagOverride(false);
	}
	ActiveShiftTarget = nullptr;
	ActiveShiftWarningVFXKey = NAME_None;
	SmoothShiftElapsedSeconds = 0.0f;
	LastSmoothShiftUpdateTime = 0.0f;

	RemoveOwnerInvulnerabilityIfNeeded();

	EndBossPatternAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_FaerinShiftSequence::RegisterCharacterEventListener()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	ActiveEventRouter = AvatarActor->FindComponentByClass<UPRFaerinCharacterEventRouterComponent>();
	if (!IsValid(ActiveEventRouter))
	{
		return false;
	}

	ActiveEventRouter->RegisterFaerinEventListener(
		this,
		FFaerinCharacterEventSignature::FDelegate::CreateUObject(
			this,
			&UPRGameplayAbility_FaerinShiftSequence::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_FaerinShiftSequence::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

void UPRGameplayAbility_FaerinShiftSequence::HandleFaerinCharacterEvent(FName EventName)
{
	if (bShiftResolved || bShiftMoveInProgress || EventName != ShiftCharacterEventName)
	{
		return;
	}

	AActor* TargetActor = GetBossPatternTarget();
	if (ShouldTargetAvoidShift(TargetActor))
	{
		bShiftResolved = true;
		return;
	}

	FVector Destination = FVector::ZeroVector;
	if (!ResolveShiftDestination(Destination))
	{
		UE_LOG(LogPRFaerinShiftSequence, Warning,
			TEXT("Faerin shift destination failed. Ability=%s, Event=%s"),
			*GetNameSafe(this),
			*EventName.ToString());
		FinishShiftSequence(true);
		return;
	}

	const bool bShiftStartedOrApplied = ApplyShiftToTarget(Destination);
	if (!bShiftStartedOrApplied)
	{
		FinishShiftSequence(true);
		return;
	}

	if (!bShiftMoveInProgress)
	{
		bShiftApplied = true;
	}

	bShiftResolved = true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ShouldTargetAvoidShift(AActor* TargetActor) const
{
	if (!bDodgingTargetAvoidsShift || !IsValid(TargetActor))
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	return IsValid(TargetAbilitySystem) && TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dodging);
}

void UPRGameplayAbility_FaerinShiftSequence::SpawnShiftWarningVFX()
{
	if (!IsValid(ShiftWarningNiagaraSystem))
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority() || !IsValid(TargetActor))
	{
		return;
	}

	FVector WarningLocation = FVector::ZeroVector;
	if (!ResolveShiftWarningLocation(TargetActor, WarningLocation))
	{
		return;
	}

	const FSoftObjectPath WarningSystemPath(ShiftWarningNiagaraSystem.Get());
	ActiveShiftWarningVFXKey = FName(*FString::Printf(
		TEXT("FaerinShiftWarning_%s_%llu"),
		*GetNameSafe(this),
		static_cast<unsigned long long>(FPlatformTime::Cycles64())));

	FaerinCharacter->Multicast_SpawnFaerinTrackedWorldNiagara(
		ActiveShiftWarningVFXKey,
		WarningSystemPath,
		WarningLocation,
		ShiftWarningNiagaraRotationOffset,
		ShiftWarningNiagaraScale,
		ShiftWarningNiagaraLifeSeconds);
}

void UPRGameplayAbility_FaerinShiftSequence::StopActiveShiftWarningVFX()
{
	if (!bDestroyWorldShiftWarningVFXOnHit || ActiveShiftWarningVFXKey == NAME_None)
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority())
	{
		ActiveShiftWarningVFXKey = NAME_None;
		return;
	}

	FaerinCharacter->Multicast_StopFaerinTrackedNiagara(ActiveShiftWarningVFXKey);
	ActiveShiftWarningVFXKey = NAME_None;
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveShiftWarningLocation(AActor* TargetActor, FVector& OutLocation) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	if (!IsValid(World) || !IsValid(AvatarActor))
	{
		OutLocation = TargetLocation + ShiftWarningLocationOffset;
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinShiftWarningGroundTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(TargetActor);

	const FVector TraceStart = TargetLocation + FVector(0.0f, 0.0f, ShiftWarningGroundTraceUpOffset);
	const FVector TraceEnd = TargetLocation - FVector(0.0f, 0.0f, ShiftWarningGroundTraceDownOffset);

	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ShiftWarningGroundTraceChannel,
		QueryParams))
	{
		OutLocation = HitResult.ImpactPoint + ShiftWarningLocationOffset;
		return true;
	}

	OutLocation = TargetLocation + ShiftWarningLocationOffset;
	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveShiftDestination(FVector& OutDestination) const
{
	bool bResolved = false;

	if (DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery)
	{
		bResolved = ResolveQueryShiftDestination(OutDestination);
		if (!bResolved && bFallbackToDirectionalDestinationWhenQueryFails)
		{
			bResolved = ResolveDirectionalShiftDestination(OutDestination);
		}
	}
	else
	{
		bResolved = ResolveDirectionalShiftDestination(OutDestination);
	}

	if (!bResolved)
	{
		return false;
	}

	if (bProjectDestinationToGround)
	{
		FVector ProjectedDestination = OutDestination;
		if (ProjectShiftDestinationToGround(OutDestination, ProjectedDestination))
		{
			OutDestination = ProjectedDestination;
		}
		else if (bRequireGroundProjection)
		{
			return false;
		}
	}

	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveQueryShiftDestination(FVector& OutDestination) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !IsValid(ShiftDestinationQuery.Get()))
	{
		return false;
	}

	return PREnemyEQSSelectionUtils::RunLocationQuery(
		BossCharacter,
		ShiftDestinationQuery.Get(),
		FloatParams,
		ShiftQueryRunMode,
		CandidateSelectionMode,
		TopCandidateCount,
		TopScoreCandidateRatio,
		OutDestination);
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveDirectionalShiftDestination(FVector& OutDestination) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector Direction = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		Direction = BossCharacter->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	OutDestination = BossCharacter->GetActorLocation() + Direction * DirectionalDistanceFromBoss;
	OutDestination.Z = TargetActor->GetActorLocation().Z;
	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ProjectShiftDestinationToGround(
	const FVector& InDestination,
	FVector& OutDestination) const
{
	const UWorld* World = GetWorld();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(World) || !IsValid(AvatarActor))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinShiftGroundTrace), false, AvatarActor);
	const FVector TraceStart = InDestination + FVector(0.0f, 0.0f, GroundTraceUpOffset);
	const FVector TraceEnd = InDestination - FVector(0.0f, 0.0f, GroundTraceDownOffset);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GroundTraceChannel, QueryParams))
	{
		return false;
	}

	OutDestination = HitResult.ImpactPoint + GroundLocationOffset;
	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ApplyShiftToTarget(const FVector& Destination)
{
	if (TargetMoveMode == EPRFaerinShiftMoveMode::SmoothPull && SmoothPullDuration > 0.0f)
	{
		return StartSmoothTargetShift(Destination);
	}

	return ApplyInstantShiftToTarget(Destination);
}

void UPRGameplayAbility_FaerinShiftSequence::ApplyShiftImpactToTarget(AActor* TargetActor)
{
	if (ShiftImpactPoiseDamage <= 0.0f || !IsValid(TargetActor))
	{
		return;
	}

	ApplyAttackPowerDamage(TargetActor, 0.0f, ShiftImpactPoiseDamage);
}

void UPRGameplayAbility_FaerinShiftSequence::SpawnAttachedShiftWarningVFX(AActor* TargetActor, const float SuggestedLifeSeconds) const
{
	if (!bAttachShiftWarningVFXOnHit || !IsValid(ShiftWarningNiagaraSystem) || !IsValid(TargetActor))
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority())
	{
		return;
	}

	const float LifeSeconds = ResolveAttachedShiftWarningLifeSeconds(SuggestedLifeSeconds);
	if (LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	const FSoftObjectPath WarningSystemPath(ShiftWarningNiagaraSystem.Get());
	FaerinCharacter->Multicast_SpawnFaerinAttachedNiagara(
		WarningSystemPath,
		TargetActor,
		ShiftWarningAttachedSocketName,
		ShiftWarningAttachedLocationOffset,
		ShiftWarningAttachedRotationOffset,
		ShiftWarningNiagaraScale * ShiftWarningAttachedScaleMultiplier,
		LifeSeconds);
}

float UPRGameplayAbility_FaerinShiftSequence::ResolveAttachedShiftWarningLifeSeconds(const float SuggestedLifeSeconds) const
{
	if (ShiftWarningAttachedLifeSeconds > UE_SMALL_NUMBER)
	{
		return ShiftWarningAttachedLifeSeconds;
	}

	return FMath::Max(
		FMath::Max(SuggestedLifeSeconds, 0.0f) + FMath::Max(ShiftWarningAttachedLifeExtraSeconds, 0.0f),
		FMath::Max(ShiftWarningAttachedMinimumLifeSeconds, 0.0f));
}

void UPRGameplayAbility_FaerinShiftSequence::MirrorShiftMoveToOwningClient(
	AActor* TargetActor,
	const FVector& Destination,
	const FRotator& Rotation,
	float Duration) const
{
	if (!bMirrorTargetMoveToOwningClient)
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(TargetActor);
	if (!IsValid(PlayerCharacter) || PlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	PlayerCharacter->ClientStartExternalForcedMove(
		Destination,
		Rotation,
		Duration,
		SmoothPullTickInterval,
		SmoothPullEaseExponent,
		bSweepTargetMove,
		bStopTargetMovementAfterShift);
}

FRotator UPRGameplayAbility_FaerinShiftSequence::ResolveTargetRotationAfterShift(const FVector& Destination) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(TargetActor))
	{
		return FRotator::ZeroRotator;
	}

	FRotator TargetRotation = TargetActor->GetActorRotation();
	if (bFaceBossAfterShift && IsValid(BossCharacter))
	{
		const FVector ToBoss = BossCharacter->GetActorLocation() - Destination;
		TargetRotation = ToBoss.Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;
	}

	return TargetRotation;
}

bool UPRGameplayAbility_FaerinShiftSequence::ApplyInstantShiftToTarget(const FVector& Destination)
{
	AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const FRotator TargetRotation = ResolveTargetRotationAfterShift(Destination);
	FHitResult SweepHit;
	const bool bMoved = TargetActor->SetActorLocationAndRotation(
		Destination,
		TargetRotation,
		bSweepTargetMove,
		bSweepTargetMove ? &SweepHit : nullptr,
		ETeleportType::TeleportPhysics);

	if (!bMoved)
	{
		return false;
	}

	StopShiftedTargetMovement(TargetActor);
	ApplyShiftImpactToTarget(TargetActor);
	StopActiveShiftWarningVFX();
	SpawnAttachedShiftWarningVFX(TargetActor, 0.0f);
	MirrorShiftMoveToOwningClient(TargetActor, Destination, TargetRotation, 0.0f);

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Destination, 60.0f, 16, FColor::Cyan, false, DebugDrawDuration);
	}

	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::StartSmoothTargetShift(const FVector& Destination)
{
	AActor* TargetActor = GetBossPatternTarget();
	UWorld* World = GetWorld();
	if (!IsValid(TargetActor) || !IsValid(World))
	{
		return false;
	}

	World->GetTimerManager().ClearTimer(SmoothShiftTimerHandle);

	ActiveShiftTarget = TargetActor;
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(TargetActor))
	{
		PlayerCharacter->SetExternalForcedMoveCameraLagOverride(true);
	}
	SmoothShiftStartLocation = TargetActor->GetActorLocation();
	SmoothShiftEndLocation = Destination;
	SmoothShiftStartRotation = TargetActor->GetActorRotation();
	SmoothShiftEndRotation = ResolveTargetRotationAfterShift(Destination);
	SmoothShiftElapsedSeconds = 0.0f;
	LastSmoothShiftUpdateTime = World->GetTimeSeconds();
	bShiftMoveInProgress = true;
	bFinishWhenShiftMoveCompletes = false;

	World->GetTimerManager().SetTimer(
		SmoothShiftTimerHandle,
		this,
		&UPRGameplayAbility_FaerinShiftSequence::TickSmoothTargetShift,
		FMath::Max(SmoothPullTickInterval, 0.005f),
		true);

	ApplyShiftImpactToTarget(TargetActor);
	StopActiveShiftWarningVFX();
	SpawnAttachedShiftWarningVFX(TargetActor, SmoothPullDuration);
	MirrorShiftMoveToOwningClient(TargetActor, Destination, SmoothShiftEndRotation, SmoothPullDuration);
	TickSmoothTargetShift();
	return true;
}

void UPRGameplayAbility_FaerinShiftSequence::TickSmoothTargetShift()
{
	AActor* TargetActor = ActiveShiftTarget.Get();
	UWorld* World = GetWorld();
	if (!bShiftMoveInProgress || !IsValid(TargetActor) || !IsValid(World))
	{
		CompleteSmoothTargetShift(true);
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float DeltaTime = LastSmoothShiftUpdateTime > 0.0f
		? CurrentTime - LastSmoothShiftUpdateTime
		: FMath::Max(SmoothPullTickInterval, 0.005f);
	LastSmoothShiftUpdateTime = CurrentTime;
	SmoothShiftElapsedSeconds += FMath::Max(DeltaTime, 0.0f);

	const float Alpha = SmoothPullDuration > 0.0f
		? FMath::Clamp(SmoothShiftElapsedSeconds / SmoothPullDuration, 0.0f, 1.0f)
		: 1.0f;
	const float EasedAlpha = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		Alpha,
		FMath::Max(SmoothPullEaseExponent, 0.1f));

	const FVector NewLocation = FMath::Lerp(SmoothShiftStartLocation, SmoothShiftEndLocation, EasedAlpha);
	const FQuat NewRotation = FQuat::Slerp(
		SmoothShiftStartRotation.Quaternion(),
		SmoothShiftEndRotation.Quaternion(),
		EasedAlpha);

	FHitResult SweepHit;
	const bool bMoved = TargetActor->SetActorLocationAndRotation(
		NewLocation,
		NewRotation.Rotator(),
		bSweepTargetMove,
		bSweepTargetMove ? &SweepHit : nullptr,
		ETeleportType::TeleportPhysics);

	if (!bMoved)
	{
		CompleteSmoothTargetShift(true);
		return;
	}

	if (Alpha >= 1.0f)
	{
		CompleteSmoothTargetShift(false);
	}
}

void UPRGameplayAbility_FaerinShiftSequence::CompleteSmoothTargetShift(bool bWasCancelled)
{
	if (!bShiftMoveInProgress)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SmoothShiftTimerHandle);
	}

	AActor* TargetActor = ActiveShiftTarget.Get();
	bool bFinalMoveSucceeded = true;
	if (!bWasCancelled && IsValid(TargetActor))
	{
		FHitResult SweepHit;
		bFinalMoveSucceeded = TargetActor->SetActorLocationAndRotation(
			SmoothShiftEndLocation,
			SmoothShiftEndRotation,
			bSweepTargetMove,
			bSweepTargetMove ? &SweepHit : nullptr,
			ETeleportType::TeleportPhysics);
	}
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(TargetActor))
	{
		PlayerCharacter->SetExternalForcedMoveCameraLagOverride(false);
	}

	bShiftMoveInProgress = false;
	ActiveShiftTarget = nullptr;
	SmoothShiftElapsedSeconds = 0.0f;
	LastSmoothShiftUpdateTime = 0.0f;

	if (!bWasCancelled && bFinalMoveSucceeded)
	{
		bShiftApplied = true;
		StopShiftedTargetMovement(TargetActor);
		if (bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), SmoothShiftEndLocation, 60.0f, 16, FColor::Cyan, false, DebugDrawDuration);
		}
	}
	else
	{
		bWasCancelled = true;
	}

	if (bWasCancelled || bFinishWhenShiftMoveCompletes)
	{
		FinishShiftSequence(bWasCancelled);
	}
}

void UPRGameplayAbility_FaerinShiftSequence::StopShiftedTargetMovement(AActor* TargetActor) const
{
	if (!bStopTargetMovementAfterShift)
	{
		return;
	}

	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (UCharacterMovementComponent* MovementComponent = TargetCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void UPRGameplayAbility_FaerinShiftSequence::FinishShiftSequence(bool bWasCancelled)
{
	if (bShiftSequenceFinished)
	{
		return;
	}

	bShiftSequenceFinished = true;

	if (IsValid(ActiveShiftMontageTask))
	{
		ActiveShiftMontageTask->EndTask();
		ActiveShiftMontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_FaerinShiftSequence::ApplyOwnerInvulnerabilityIfNeeded()
{
	if (!ShouldGrantOwnerInvulnerabilityForCurrentPhase())
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(OwnerASC))
	{
		return;
	}

	OwnerASC->AddLooseGameplayTag(PRGameplayTags::State_Invulnerable);
	OwnerASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Invulnerable);
	bOwnerInvulnerabilityApplied = true;
}

void UPRGameplayAbility_FaerinShiftSequence::RemoveOwnerInvulnerabilityIfNeeded()
{
	if (!bOwnerInvulnerabilityApplied)
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(OwnerASC))
	{
		OwnerASC->RemoveLooseGameplayTag(PRGameplayTags::State_Invulnerable);
		OwnerASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Invulnerable);
	}

	bOwnerInvulnerabilityApplied = false;
}

bool UPRGameplayAbility_FaerinShiftSequence::ShouldGrantOwnerInvulnerabilityForCurrentPhase() const
{
	if (!bGrantOwnerInvulnerabilityDuringShift)
	{
		return false;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority())
	{
		return false;
	}

	return static_cast<uint8>(BossCharacter->GetCurrentPhase())
		>= static_cast<uint8>(OwnerInvulnerabilityMinimumPhase);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageCompleted()
{
	if (bShiftMoveInProgress)
	{
		bFinishWhenShiftMoveCompletes = true;
		return;
	}

	if (bRequireCharacterEventForShift && !bShiftResolved)
	{
		FinishShiftSequence(true);
		return;
	}

	FinishShiftSequence(false);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageBlendOut()
{
	if (bShiftMoveInProgress)
	{
		bFinishWhenShiftMoveCompletes = true;
		return;
	}

	if (bRequireCharacterEventForShift && !bShiftResolved)
	{
		FinishShiftSequence(true);
		return;
	}

	FinishShiftSequence(false);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted()
{
	FinishShiftSequence(true);
}
