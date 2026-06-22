// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Staged Montage Pattern 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinStagedMontagePattern.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinStagedMontagePattern, Log, All);

// ===== 생성 =====

UPRGameplayAbility_FaerinStagedMontagePattern::UPRGameplayAbility_FaerinStagedMontagePattern()
{
}

// ===== UGameplayAbility Interface =====

void UPRGameplayAbility_FaerinStagedMontagePattern::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!CanRunBossPattern()
		|| !IsValid(BossCharacter)
		|| !BossCharacter->HasAuthority()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBossPatternAttackCommit();

	if (MontageStages.IsEmpty())
	{
		UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
			TEXT("Faerin staged pattern has no stages. Ability=%s"),
			*GetNameSafe(this));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bStagedPatternFinished = false;
	bAdvancingStageByFixedTimer = false;
	ActiveStageIndex = INDEX_NONE;

	if (RequiresCharacterEventListener() && !RegisterCharacterEventListener())
	{
		UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
			TEXT("Faerin staged pattern requires CharacterEvent router but router is missing. Ability=%s"),
			*GetNameSafe(this));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PlayStage(0))
	{
		FinishStagedPattern(true);
	}
}

void UPRGameplayAbility_FaerinStagedMontagePattern::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	ClearStageEventTimeout();
	ClearStageFixedAdvanceTimer();
	UnregisterCharacterEventListener();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveStageIndex = INDEX_NONE;
	bStagedPatternFinished = false;
	bAdvancingStageByFixedTimer = false;

	EndBossPatternAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ===== CharacterEvent =====

bool UPRGameplayAbility_FaerinStagedMontagePattern::RegisterCharacterEventListener()
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
			&UPRGameplayAbility_FaerinStagedMontagePattern::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

bool UPRGameplayAbility_FaerinStagedMontagePattern::RequiresCharacterEventListener() const
{
	if (bRequireCharacterEventRouter)
	{
		return true;
	}

	for (const FPRFaerinStagedMontageStage& Stage : MontageStages)
	{
		if (Stage.bWaitForCharacterEventToAdvance)
		{
			return true;
		}
	}

	return false;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleFaerinCharacterEvent(const FName EventName)
{
	if (bStagedPatternFinished || !MontageStages.IsValidIndex(ActiveStageIndex))
	{
		return;
	}

	const FPRFaerinStagedMontageStage& ActiveStage = MontageStages[ActiveStageIndex];
	NativeOnStageCharacterEvent(ActiveStage, EventName);
	BP_OnStageCharacterEvent(ActiveStage.StageName, EventName);

	if (!ActiveStage.bWaitForCharacterEventToAdvance || ActiveStage.AdvanceEventName != EventName)
	{
		return;
	}

	ClearStageEventTimeout();
	ClearStageFixedAdvanceTimer();
	AdvanceToNextStage();
}

// ===== Stage =====

bool UPRGameplayAbility_FaerinStagedMontagePattern::PlayStage(const int32 StageIndex)
{
	if (!MontageStages.IsValidIndex(StageIndex))
	{
		return false;
	}

	const FPRFaerinStagedMontageStage& Stage = MontageStages[StageIndex];
	if (!IsValid(Stage.Montage))
	{
		UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
			TEXT("Faerin staged pattern stage montage is missing. Ability=%s, StageIndex=%d, StageName=%s"),
			*GetNameSafe(this),
			StageIndex,
			*Stage.StageName.ToString());
		return false;
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ClearStageEventTimeout();
	ClearStageFixedAdvanceTimer();
	ActiveStageIndex = StageIndex;

	if (Stage.bStopMovementBeforeStage)
	{
		StopBossMovement();
	}

	if (Stage.bApplyDestinationBeforeStage && !ApplyStageDestination(Stage))
	{
		UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
			TEXT("Faerin staged pattern stage destination failed. Ability=%s, StageIndex=%d, StageName=%s"),
			*GetNameSafe(this),
			StageIndex,
			*Stage.StageName.ToString());
		return false;
	}

	if (Stage.bFaceTargetBeforeStage)
	{
		FaceCurrentTarget();
	}

	NativeOnStageStarted(Stage);
	BP_OnStageStarted(Stage.StageName);

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Stage.Montage,
		FMath::Max(Stage.MontagePlayRate, UE_SMALL_NUMBER),
		Stage.MontageStartSection,
		true,
		1.0f,
		Stage.MontageStartTimeSeconds);

	if (!IsValid(ActiveMontageTask))
	{
		return false;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageBlendOut);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();

	StartStageEventTimeout(Stage);
	StartStageFixedAdvanceTimer(Stage);
	return true;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::AdvanceToNextStage()
{
	if (bStagedPatternFinished)
	{
		return;
	}

	const int32 NextStageIndex = ActiveStageIndex + 1;
	if (!MontageStages.IsValidIndex(NextStageIndex))
	{
		FinishStagedPattern(false);
		return;
	}

	if (!PlayStage(NextStageIndex))
	{
		FinishStagedPattern(true);
	}
}

// ===== Destination =====

bool UPRGameplayAbility_FaerinStagedMontagePattern::ApplyStageDestination(const FPRFaerinStagedMontageStage& Stage)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority())
	{
		return false;
	}

	FVector Destination = FVector::ZeroVector;
	if (!ResolveStageDestination(Stage.DestinationConfig, Destination))
	{
		return false;
	}

	const bool bMoved = BossCharacter->SetActorLocation(
		Destination,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (bMoved && bDrawDebugDestination)
	{
		DrawDebugSphere(GetWorld(), Destination, 80.0f, 16, FColor::Purple, false, DebugDrawDuration);
	}

	return bMoved;
}

bool UPRGameplayAbility_FaerinStagedMontagePattern::ResolveStageDestination(
	const FPRFaerinStagedMontageDestinationConfig& DestinationConfig,
	FVector& OutDestination) const
{
	bool bResolved = false;
	if (DestinationConfig.DestinationMode == EPRFaerinStageDestinationMode::EnvQuery)
	{
		bResolved = ResolveQueryDestination(DestinationConfig, OutDestination);
		if (!bResolved && DestinationConfig.bFallbackToTargetOffsetWhenQueryFails)
		{
			bResolved = ResolveTargetOffsetDestination(DestinationConfig.TargetLocalOffset, OutDestination);
		}
	}
	else if (DestinationConfig.DestinationMode == EPRFaerinStageDestinationMode::TargetOffset)
	{
		bResolved = ResolveTargetOffsetDestination(DestinationConfig.TargetLocalOffset, OutDestination);
	}

	if (!bResolved)
	{
		return false;
	}

	if (DestinationConfig.bProjectToGround)
	{
		FVector ProjectedDestination = OutDestination;
		if (ProjectDestinationToGround(DestinationConfig, OutDestination, ProjectedDestination))
		{
			OutDestination = ProjectedDestination;
		}
		else if (DestinationConfig.bRequireGroundProjection)
		{
			return false;
		}
	}

	return true;
}

bool UPRGameplayAbility_FaerinStagedMontagePattern::ResolveTargetOffsetDestination(
	const FVector& TargetLocalOffset,
	FVector& OutDestination) const
{
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(TargetActor))
	{
		return false;
	}

	OutDestination = TargetActor->GetActorTransform().TransformPosition(TargetLocalOffset);
	return true;
}

bool UPRGameplayAbility_FaerinStagedMontagePattern::ResolveQueryDestination(
	const FPRFaerinStagedMontageDestinationConfig& DestinationConfig,
	FVector& OutDestination) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !IsValid(DestinationConfig.DestinationQuery.Get()))
	{
		return false;
	}

	return PREnemyEQSSelectionUtils::RunLocationQuery(
		BossCharacter,
		DestinationConfig.DestinationQuery.Get(),
		DestinationConfig.FloatParams,
		DestinationConfig.QueryRunMode,
		DestinationConfig.CandidateSelectionMode,
		DestinationConfig.TopCandidateCount,
		DestinationConfig.TopScoreCandidateRatio,
		OutDestination);
}

bool UPRGameplayAbility_FaerinStagedMontagePattern::ProjectDestinationToGround(
	const FPRFaerinStagedMontageDestinationConfig& DestinationConfig,
	const FVector& InDestination,
	FVector& OutDestination) const
{
	const UWorld* World = GetWorld();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(World) || !IsValid(AvatarActor))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinStagedPatternGroundTrace), false, AvatarActor);
	const FVector TraceStart = InDestination + FVector(0.0f, 0.0f, DestinationConfig.GroundTraceUpOffset);
	const FVector TraceEnd = InDestination - FVector(0.0f, 0.0f, DestinationConfig.GroundTraceDownOffset);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, DestinationConfig.GroundTraceChannel, QueryParams))
	{
		return false;
	}

	OutDestination = HitResult.ImpactPoint + DestinationConfig.GroundLocationOffset;
	return true;
}

// ===== Movement =====

void UPRGameplayAbility_FaerinStagedMontagePattern::FaceCurrentTarget() const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return;
	}

	const FVector Direction = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
	FRotator FacingRotation = Direction.Rotation();
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	BossCharacter->SetActorRotation(FacingRotation);
}

void UPRGameplayAbility_FaerinStagedMontagePattern::StopBossMovement() const
{
	const ACharacter* BossCharacter = Cast<ACharacter>(GetBossAvatarCharacter());
	if (!IsValid(BossCharacter))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = BossCharacter->GetCharacterMovement();
	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
	}
}

// ===== Timeout =====

void UPRGameplayAbility_FaerinStagedMontagePattern::StartStageEventTimeout(const FPRFaerinStagedMontageStage& Stage)
{
	if (!Stage.bWaitForCharacterEventToAdvance || Stage.EventWaitTimeout <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		StageEventTimeoutTimerHandle,
		this,
		&UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageEventTimeout,
		Stage.EventWaitTimeout,
		false);
}

void UPRGameplayAbility_FaerinStagedMontagePattern::ClearStageEventTimeout()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(StageEventTimeoutTimerHandle);
	}
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageEventTimeout()
{
	UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
		TEXT("Faerin staged pattern event wait timed out. Ability=%s, StageIndex=%d"),
		*GetNameSafe(this),
		ActiveStageIndex);
	FinishStagedPattern(true);
}

// ===== 종료 =====

void UPRGameplayAbility_FaerinStagedMontagePattern::StartStageFixedAdvanceTimer(
	const FPRFaerinStagedMontageStage& Stage)
{
	if (!Stage.bAdvanceAfterFixedTime || Stage.FixedAdvanceTime <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		StageFixedAdvanceTimerHandle,
		this,
		&UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageFixedAdvance,
		Stage.FixedAdvanceTime,
		false);
}

void UPRGameplayAbility_FaerinStagedMontagePattern::ClearStageFixedAdvanceTimer()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(StageFixedAdvanceTimerHandle);
	}
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageFixedAdvance()
{
	if (bStagedPatternFinished || !MontageStages.IsValidIndex(ActiveStageIndex))
	{
		return;
	}

	const FPRFaerinStagedMontageStage ActiveStage = MontageStages[ActiveStageIndex];
	ClearStageFixedAdvanceTimer();
	ClearStageEventTimeout();

	bAdvancingStageByFixedTimer = true;
	if (ActiveStage.bStopMontageOnFixedAdvance && IsValid(ActiveMontageTask))
	{
		UAbilityTask_PlayMontageAndWait* MontageTaskToEnd = ActiveMontageTask;
		ActiveMontageTask = nullptr;

		MontageTaskToEnd->EndTask();
	}

	AdvanceToNextStage();
	bAdvancingStageByFixedTimer = false;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage)
{
	(void)Stage;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::NativeOnStageCharacterEvent(
	const FPRFaerinStagedMontageStage& Stage,
	const FName EventName)
{
	(void)Stage;
	(void)EventName;
}

void UPRGameplayAbility_FaerinStagedMontagePattern::FinishStagedPattern(const bool bWasCancelled)
{
	if (bStagedPatternFinished)
	{
		return;
	}

	bStagedPatternFinished = true;

	ClearStageEventTimeout();
	ClearStageFixedAdvanceTimer();
	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageCompleted()
{
	if (bAdvancingStageByFixedTimer)
	{
		return;
	}

	if (MontageStages.IsValidIndex(ActiveStageIndex)
		&& MontageStages[ActiveStageIndex].bAdvanceAfterFixedTime
		&& MontageStages[ActiveStageIndex].FixedAdvanceTime > 0.0f)
	{
		return;
	}

	ClearStageFixedAdvanceTimer();

	if (MontageStages.IsValidIndex(ActiveStageIndex)
		&& MontageStages[ActiveStageIndex].bWaitForCharacterEventToAdvance)
	{
		if (MontageStages[ActiveStageIndex].EventWaitTimeout <= 0.0f)
		{
			UE_LOG(LogPRFaerinStagedMontagePattern, Warning,
				TEXT("Faerin staged pattern montage ended before required event. Ability=%s, StageIndex=%d"),
				*GetNameSafe(this),
				ActiveStageIndex);
			FinishStagedPattern(true);
		}
		return;
	}

	AdvanceToNextStage();
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageBlendOut()
{
	HandleStageMontageCompleted();
}

void UPRGameplayAbility_FaerinStagedMontagePattern::HandleStageMontageInterrupted()
{
	if (bAdvancingStageByFixedTimer)
	{
		return;
	}

	FinishStagedPattern(true);
}
