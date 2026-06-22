// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 접근 돌진 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinApproachSprint.h"

#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

UPRGameplayAbility_FaerinApproachSprint::UPRGameplayAbility_FaerinApproachSprint()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_ApproachSprint));

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
}

void UPRGameplayAbility_FaerinApproachSprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	if (!IsValid(EnemyCharacter)
		|| !EnemyCharacter->HasAuthority()
		|| !ResolveApproachRequest(EnemyCharacter)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bApproachFinished = false;
	bEndingSprint = false;
	bPendingEndCancelled = false;
	LastMovementUpdateTime = 0.0f;
	BurstElapsedSeconds = 0.0f;
	CachedBurstDirection = FVector::ZeroVector;
	bHasCachedBurstDirection = false;

	FPREnemyMovePresentationConfig BurstPresentationConfig = ActiveRequest.PresentationConfig;
	BurstPresentationConfig.bMaintainTargetFocus = false;
	BurstPresentationConfig.bUseControllerDesiredRotation = false;
	BurstPresentationConfig.bOrientRotationToMovement = false;
	EnemyCharacter->ApplyCombatMovePresentationContext(BurstPresentationConfig);
	PlaySprintMontage();
	StartApproachMovement();
}

void UPRGameplayAbility_FaerinApproachSprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearApproachTimers();
	StopApproachMovement();

	if (APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter())
	{
		EnemyCharacter->ClearCombatMovePresentationContext();
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveDirectorComponent = nullptr;
	ActiveRequest = FPRFaerinApproachSprintRequest();
	CachedBurstDirection = FVector::ZeroVector;
	bApproachFinished = true;
	bEndingSprint = false;
	BurstElapsedSeconds = 0.0f;
	bHasCachedBurstDirection = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_FaerinApproachSprint::ResolveApproachRequest(APREnemyBaseCharacter* EnemyCharacter)
{
	if (!IsValid(EnemyCharacter))
	{
		return false;
	}

	ActiveDirectorComponent = EnemyCharacter->FindComponentByClass<UPRFaerinCombatDirectorComponent>();
	if (!IsValid(ActiveDirectorComponent))
	{
		return false;
	}

	if (!ActiveDirectorComponent->GetActiveApproachSprintRequest(ActiveRequest))
	{
		return false;
	}

	return ActiveRequest.bIsValid
		&& IsValid(ActiveRequest.TargetActor)
		&& ActiveRequest.StopDistance > 0.0f;
}

bool UPRGameplayAbility_FaerinApproachSprint::PlaySprintMontage()
{
	if (!IsValid(SprintMontage))
	{
		return false;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SprintMontage,
		FMath::Max(MontagePlayRate, 0.01f),
		EnterSectionName);

	if (!IsValid(ActiveMontageTask))
	{
		return false;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageBlendOut);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	ConfigureSprintLoopSections();
	return true;
}

void UPRGameplayAbility_FaerinApproachSprint::ConfigureSprintLoopSections() const
{
	if (!IsValid(SprintMontage)
		|| !SprintMontage->IsValidSectionName(EnterSectionName)
		|| !SprintMontage->IsValidSectionName(LoopSectionName))
	{
		return;
	}

	const ACharacter* EnemyCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = IsValid(EnemyCharacter) ? EnemyCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_SetNextSection(EnterSectionName, LoopSectionName, SprintMontage);
	AnimInstance->Montage_SetNextSection(LoopSectionName, LoopSectionName, SprintMontage);
}

void UPRGameplayAbility_FaerinApproachSprint::StartApproachMovement()
{
	TickApproachMovement();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DistanceCheckTimerHandle,
			this,
			&UPRGameplayAbility_FaerinApproachSprint::TickApproachMovement,
			FMath::Max(DistanceCheckInterval, 0.01f),
			true);

		if (ActiveRequest.TimeoutSeconds > 0.0f)
		{
			const float TimeoutDelaySeconds = ResolveTimeoutDelaySeconds();
			World->GetTimerManager().SetTimer(
				TimeoutTimerHandle,
				this,
				&UPRGameplayAbility_FaerinApproachSprint::HandleApproachTimeoutElapsed,
				TimeoutDelaySeconds,
				false);
		}
	}
}

float UPRGameplayAbility_FaerinApproachSprint::ResolveTimeoutDelaySeconds() const
{
	if (ActiveRequest.TimeoutSeconds <= 0.0f)
	{
		return 0.0f;
	}

	float EnterSectionLength = 0.0f;
	if (IsValid(SprintMontage) && SprintMontage->IsValidSectionName(EnterSectionName))
	{
		const int32 EnterSectionIndex = SprintMontage->GetSectionIndex(EnterSectionName);
		if (EnterSectionIndex != INDEX_NONE)
		{
			EnterSectionLength = SprintMontage->GetSectionLength(EnterSectionIndex) / FMath::Max(MontagePlayRate, 0.01f);
		}
	}

	const float MinimumMontageSafeDelay = EnterSectionLength + MinimumLoopDurationBeforeTimeout;
	return FMath::Max(ActiveRequest.TimeoutSeconds, MinimumMontageSafeDelay);
}

void UPRGameplayAbility_FaerinApproachSprint::TickApproachMovement()
{
	if (bApproachFinished || bEndingSprint)
	{
		return;
	}

	if (!IsValid(ActiveRequest.TargetActor))
	{
		BeginEndSection(true);
		return;
	}

	const float DistanceToTarget = CalculateDistanceToTarget();
	if (DistanceToTarget <= ActiveRequest.StopDistance)
	{
		BeginEndSection(false);
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float DeltaTime = LastMovementUpdateTime > 0.0f
		? CurrentTime - LastMovementUpdateTime
		: FMath::Max(DistanceCheckInterval, 0.005f);
	LastMovementUpdateTime = CurrentTime;
	BurstElapsedSeconds += FMath::Max(DeltaTime, 0.005f);

	if (!MoveDirectlyTowardTarget(FMath::Max(DeltaTime, 0.005f)))
	{
		BeginEndSection(false);
		return;
	}

	const float MaxBurstSeconds = FMath::Max(DirectionTrackingDuration, 0.0f) + FMath::Max(StraightBurstDuration, 0.0f);
	if (MaxBurstSeconds > 0.0f && BurstElapsedSeconds >= MaxBurstSeconds)
	{
		BeginEndSection(false);
		return;
	}

}

void UPRGameplayAbility_FaerinApproachSprint::HandleApproachTimeoutElapsed()
{
	if (!bApproachFinished && !bEndingSprint)
	{
		BeginEndSection(false);
	}
}

bool UPRGameplayAbility_FaerinApproachSprint::MoveDirectlyTowardTarget(float DeltaTime)
{
	ACharacter* EnemyCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = ActiveRequest.TargetActor.Get();
	if (!IsValid(EnemyCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	const float DistanceToTarget = CalculateDistanceToTarget();
	if (DistanceToTarget <= ActiveRequest.StopDistance)
	{
		return true;
	}

	if (!bHasCachedBurstDirection || BurstElapsedSeconds <= DirectionTrackingDuration)
	{
		if (!UpdateBurstDirectionFromTarget())
		{
			return false;
		}
	}

	if (!bHasCachedBurstDirection || CachedBurstDirection.IsNearlyZero())
	{
		return false;
	}

	EnemyCharacter->SetActorRotation(CachedBurstDirection.Rotation());

	const float MaxMoveDistance = FMath::Max(DistanceToTarget - ActiveRequest.StopDistance, 0.0f);
	const float MoveDistance = FMath::Min(ResolveSprintMoveSpeed() * DeltaTime, MaxMoveDistance);
	if (MoveDistance <= 0.0f)
	{
		return true;
	}

	const FVector OldLocation = EnemyCharacter->GetActorLocation();
	FHitResult SweepHit;
	EnemyCharacter->AddActorWorldOffset(CachedBurstDirection * MoveDistance, true, &SweepHit);
	if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
	{
		const FVector AppliedVelocity = DeltaTime > UE_SMALL_NUMBER
			? (EnemyCharacter->GetActorLocation() - OldLocation) / DeltaTime
			: FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->Velocity = AppliedVelocity;
	}

	return !SweepHit.bBlockingHit;
}

bool UPRGameplayAbility_FaerinApproachSprint::UpdateBurstDirectionFromTarget()
{
	ACharacter* EnemyCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = ActiveRequest.TargetActor.Get();
	if (!IsValid(EnemyCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector MoveDirection = TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation();
	if (bConstrainMovementToGround)
	{
		MoveDirection.Z = 0.0f;
	}

	if (!MoveDirection.Normalize())
	{
		return false;
	}

	CachedBurstDirection = MoveDirection;
	bHasCachedBurstDirection = true;
	return true;
}

float UPRGameplayAbility_FaerinApproachSprint::ResolveSprintMoveSpeed() const
{
	return FMath::Max(SprintMoveSpeed, 0.0f);
}

void UPRGameplayAbility_FaerinApproachSprint::BeginEndSection(bool bWasCancelled)
{
	if (bApproachFinished || bEndingSprint)
	{
		return;
	}

	bEndingSprint = true;
	bPendingEndCancelled = bWasCancelled;
	ClearApproachTimers();
	StopApproachMovement();

	if (!IsValid(SprintMontage) || !SprintMontage->IsValidSectionName(EndSectionName))
	{
		FinishApproachSprint(bWasCancelled);
		return;
	}

	const ACharacter* EnemyCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = IsValid(EnemyCharacter) ? EnemyCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsPlaying(SprintMontage))
	{
		FinishApproachSprint(bWasCancelled);
		return;
	}

	if (SprintMontage->IsValidSectionName(LoopSectionName))
	{
		AnimInstance->Montage_SetNextSection(LoopSectionName, EndSectionName, SprintMontage);
	}
	AnimInstance->Montage_JumpToSection(EndSectionName, SprintMontage);
}

void UPRGameplayAbility_FaerinApproachSprint::FinishApproachSprint(bool bWasCancelled)
{
	if (bApproachFinished)
	{
		return;
	}

	bApproachFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_FaerinApproachSprint::ClearApproachTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DistanceCheckTimerHandle);
		World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
	}
}

void UPRGameplayAbility_FaerinApproachSprint::StopApproachMovement() const
{
	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIController = IsValid(OwnerPawn) ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
	if (IsValid(AIController))
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (ACharacter* EnemyCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

float UPRGameplayAbility_FaerinApproachSprint::CalculateDistanceToTarget() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const AActor* TargetActor = ActiveRequest.TargetActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist2D(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
}

void UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageCompleted()
{
	if (bApproachFinished)
	{
		return;
	}

	ActiveMontageTask = nullptr;
	if (bEndingSprint)
	{
		FinishApproachSprint(bPendingEndCancelled);
		return;
	}

	BeginEndSection(false);
}

void UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageBlendOut()
{
	if (bApproachFinished)
	{
		return;
	}

	if (bEndingSprint)
	{
		FinishApproachSprint(bPendingEndCancelled);
		return;
	}

	BeginEndSection(false);
}

void UPRGameplayAbility_FaerinApproachSprint::HandleSprintMontageInterrupted()
{
	if (!bApproachFinished)
	{
		FinishApproachSprint(true);
	}
}
