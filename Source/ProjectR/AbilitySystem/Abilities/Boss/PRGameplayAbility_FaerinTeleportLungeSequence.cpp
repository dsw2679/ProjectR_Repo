// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 순간이동 Lunge 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinTeleportLungeSequence.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinWeaponVisualComponent.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	const FName StartLungeEventName = TEXT("StartLunge");
	const FName StopLungeEventName = TEXT("StopLunge");
}

UPRGameplayAbility_FaerinTeleportLungeSequence::UPRGameplayAbility_FaerinTeleportLungeSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_TeleportLungeSequence));
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	DamagedActors.Reset();
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	StraightLungeDirection = FVector::ZeroVector;
	LungeElapsedTime = 0.0f;
	bLungeMovementActive = false;
	bLungeHitWindowActive = false;
	bLungeSequenceFinished = false;

	WeaponVisualComponent = BossCharacter->FindComponentByClass<UPRFaerinWeaponVisualComponent>();
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->ResolveConfiguredComponents();
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}

	RegisterCharacterEventListener();
	CacheMovementDefaults(BossCharacter);
	BossCharacter->GetCharacterMovement()->StopMovementImmediately();
	FaceCurrentTarget();

	if (bPlayTeleportOutStage && IsValid(TeleportOutMontage))
	{
		PlayStageMontage(TeleportOutMontage, EPRFaerinTeleportLungeStage::TeleportOut);
		return;
	}

	PlayTeleportInStage();
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopLungeMovementAndHitWindow();
	UnregisterCharacterEventListener();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (ACharacter* BossCharacter = Cast<ACharacter>(GetBossAvatarCharacter()))
	{
		RestoreMovementDefaults(BossCharacter);
	}

	DamagedActors.Reset();
	WeaponVisualComponent = nullptr;
	ActiveLungeStage = EPRFaerinTeleportLungeStage::None;

	EndBossPatternAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_FaerinTeleportLungeSequence::RegisterCharacterEventListener()
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
			&UPRGameplayAbility_FaerinTeleportLungeSequence::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::HandleFaerinCharacterEvent(FName EventName)
{
	if (!bUseCharacterEventsForHitWindow)
	{
		return;
	}

	if (EventName == StartLungeEventName)
	{
		BeginLungeMovement();
		return;
	}

	if (EventName == StopLungeEventName)
	{
		if (ActiveLungeStage == EPRFaerinTeleportLungeStage::LungeEnd)
		{
			StopLungeMovementAndHitWindow();
			return;
		}

		PlayLungeEndStage();
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::FaceCurrentTarget() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return;
	}

	FVector Direction = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	AvatarActor->SetActorRotation(Direction.Rotation());
}

bool UPRGameplayAbility_FaerinTeleportLungeSequence::PlayStageMontage(
	UAnimMontage* Montage,
	EPRFaerinTeleportLungeStage Stage)
{
	if (!IsValid(Montage))
	{
		return false;
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveLungeStage = Stage;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Montage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		return false;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageBlendOut);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	return true;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::PlayTeleportInStage()
{
	if (!bPlayTeleportInStage)
	{
		PlayLungeStartStage();
		return;
	}

	if (PlayStageMontage(TeleportInMontage, EPRFaerinTeleportLungeStage::TeleportIn))
	{
		return;
	}

	PlayLungeStartStage();
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::PlayLungeStartStage()
{
	if (PlayStageMontage(LungeStartMontage, EPRFaerinTeleportLungeStage::LungeStart))
	{
		return;
	}

	BeginLungeMovement();
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::PlayLungeLoopStage()
{
	PlayStageMontage(LungeLoopMontage, EPRFaerinTeleportLungeStage::LungeLoop);
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::PlayLungeEndStage()
{
	if (bLungeSequenceFinished)
	{
		return;
	}

	StopLungeMovementAndHitWindow();
	if (PlayStageMontage(LungeEndMontage, EPRFaerinTeleportLungeStage::LungeEnd))
	{
		return;
	}

	FinishLungeSequence(false);
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::BeginLungeMovement()
{
	if (bLungeMovementActive || bLungeSequenceFinished)
	{
		return;
	}

	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority())
	{
		FinishLungeSequence(true);
		return;
	}

	FaceCurrentTarget();
	CacheMovementDefaults(BossCharacter);

	if (UCharacterMovementComponent* MovementComponent = BossCharacter->GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = LungeSpeed;
		MovementComponent->MaxAcceleration = LungeAcceleration;
	}

	StraightLungeDirection = BossCharacter->GetActorForwardVector();
	if (bConstrainToGround)
	{
		StraightLungeDirection.Z = 0.0f;
	}
	StraightLungeDirection.Normalize();

	LungeElapsedTime = 0.0f;
	bLungeMovementActive = true;
	BeginLungeHitWindow();

	PlayLungeLoopStage();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LungeUpdateTimerHandle,
			this,
			&UPRGameplayAbility_FaerinTeleportLungeSequence::UpdateLungeMovement,
			FMath::Max(LungeTickInterval, 0.005f),
			true);
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::StopLungeMovementAndHitWindow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LungeUpdateTimerHandle);
	}

	bLungeMovementActive = false;
	EndLungeHitWindow();
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::UpdateLungeMovement()
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!bLungeMovementActive || !IsValid(BossCharacter) || !BossCharacter->HasAuthority())
	{
		return;
	}

	const float StepDeltaTime = FMath::Max(LungeTickInterval, 0.005f);
	LungeElapsedTime += StepDeltaTime;

	FVector MoveDirection = StraightLungeDirection;
	if (LungeElapsedTime <= TrackDuration)
	{
		if (AActor* TargetActor = GetBossPatternTarget())
		{
			MoveDirection = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
			if (bConstrainToGround)
			{
				MoveDirection.Z = 0.0f;
			}

			if (!MoveDirection.IsNearlyZero())
			{
				MoveDirection.Normalize();
				StraightLungeDirection = MoveDirection;
				BossCharacter->SetActorRotation(MoveDirection.Rotation());
			}
		}
	}

	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = BossCharacter->GetActorForwardVector();
	}

	const FVector MoveDelta = MoveDirection * LungeSpeed * StepDeltaTime;
	FHitResult SweepHit;
	BossCharacter->AddActorWorldOffset(MoveDelta, true, &SweepHit);

	UpdateLungeHitTrace();

	if (SweepHit.bBlockingHit || LungeElapsedTime >= TrackDuration + StraightDuration)
	{
		PlayLungeEndStage();
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::BeginLungeHitWindow()
{
	if (bLungeHitWindowActive)
	{
		return;
	}

	bLungeHitWindowActive = true;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(true);
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::EndLungeHitWindow()
{
	bLungeHitWindowActive = false;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}
}

bool UPRGameplayAbility_FaerinTeleportLungeSequence::GetCurrentBladeTracePoint(FVector& OutTracePoint) const
{
	if (IsValid(WeaponVisualComponent) && IsValid(WeaponVisualComponent->GetBladeHitBox()))
	{
		const FTransform HitBoxTransform = WeaponVisualComponent->GetBladeHitBox()->GetComponentTransform();
		OutTracePoint = HitBoxTransform.GetLocation() + HitBoxTransform.TransformVectorNoScale(HitTraceOffset);
		return true;
	}

	const ACharacter* BossCharacter = Cast<ACharacter>(GetBossAvatarCharacter());
	if (!IsValid(BossCharacter) || !IsValid(BossCharacter->GetMesh()))
	{
		return false;
	}

	const FName BladeBoneName = IsValid(WeaponVisualComponent)
		? WeaponVisualComponent->GetBladeBoneName()
		: FallbackBladeBoneName;
	if (BladeBoneName == NAME_None)
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComponent = BossCharacter->GetMesh();
	const bool bHasSocket = MeshComponent->DoesSocketExist(BladeBoneName);
	const bool bHasBone = MeshComponent->GetBoneIndex(BladeBoneName) != INDEX_NONE;
	if (!bHasSocket && !bHasBone)
	{
		return false;
	}

	const FTransform BladeTransform = MeshComponent->GetSocketTransform(BladeBoneName, RTS_World);
	OutTracePoint = BladeTransform.GetLocation() + BladeTransform.TransformVectorNoScale(HitTraceOffset);
	return true;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::UpdateLungeHitTrace()
{
	if (!bLungeHitWindowActive)
	{
		return;
	}

	FVector CurrentTracePoint = FVector::ZeroVector;
	if (!GetCurrentBladeTracePoint(CurrentTracePoint))
	{
		return;
	}

	if (!bHasPreviousBladeTracePoint)
	{
		ApplyLungeDamageTrace(CurrentTracePoint, CurrentTracePoint);
		PreviousBladeTracePoint = CurrentTracePoint;
		bHasPreviousBladeTracePoint = true;
		return;
	}

	ApplyLungeDamageTrace(PreviousBladeTracePoint, CurrentTracePoint);
	PreviousBladeTracePoint = CurrentTracePoint;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::ApplyLungeDamageTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaerinTeleportLunge), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	TArray<FHitResult> HitResults;
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FMath::Max(HitTraceRadius, 0.0f));
	const bool bHit = AvatarActor->GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		HitTraceChannel,
		CollisionShape,
		QueryParams);

	if (bDrawDebug)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(AvatarActor->GetWorld(), TraceStart, TraceEnd, DebugColor, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(AvatarActor->GetWorld(), TraceStart, HitTraceRadius, 16, DebugColor, false, 1.0f);
		DrawDebugSphere(AvatarActor->GetWorld(), TraceEnd, HitTraceRadius, 16, DebugColor, false, 1.0f);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!ShouldDamageActor(HitActor) || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		ApplyAttackPowerDamage(HitActor, DamageMultiplier, PoiseDamage, &HitResult);
		DamagedActors.Add(HitActor);
	}
}

bool UPRGameplayAbility_FaerinTeleportLungeSequence::ShouldDamageActor(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || CandidateActor == GetAvatarActorFromActorInfo())
	{
		return false;
	}

	if (!IsValid(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateActor)))
	{
		return false;
	}

	if (!bOnlyDamageThreatTarget)
	{
		return true;
	}

	return CandidateActor == GetBossPatternTarget();
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::CacheMovementDefaults(ACharacter* BossCharacter)
{
	if (bHasCachedMovementDefaults || !IsValid(BossCharacter) || !IsValid(BossCharacter->GetCharacterMovement()))
	{
		return;
	}

	bHasCachedMovementDefaults = true;
	CachedMaxWalkSpeed = BossCharacter->GetCharacterMovement()->MaxWalkSpeed;
	CachedMaxAcceleration = BossCharacter->GetCharacterMovement()->MaxAcceleration;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::RestoreMovementDefaults(ACharacter* BossCharacter)
{
	if (!bHasCachedMovementDefaults || !IsValid(BossCharacter) || !IsValid(BossCharacter->GetCharacterMovement()))
	{
		return;
	}

	BossCharacter->GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	BossCharacter->GetCharacterMovement()->MaxAcceleration = CachedMaxAcceleration;
	bHasCachedMovementDefaults = false;
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::FinishLungeSequence(bool bWasCancelled)
{
	if (bLungeSequenceFinished)
	{
		return;
	}

	bLungeSequenceFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageCompleted()
{
	if (bLungeSequenceFinished)
	{
		return;
	}

	ActiveMontageTask = nullptr;

	switch (ActiveLungeStage)
	{
	case EPRFaerinTeleportLungeStage::TeleportOut:
		PlayTeleportInStage();
		break;
	case EPRFaerinTeleportLungeStage::TeleportIn:
		PlayLungeStartStage();
		break;
	case EPRFaerinTeleportLungeStage::LungeStart:
		if (!bLungeMovementActive)
		{
			BeginLungeMovement();
		}
		break;
	case EPRFaerinTeleportLungeStage::LungeEnd:
		FinishLungeSequence(false);
		break;
	case EPRFaerinTeleportLungeStage::LungeLoop:
	case EPRFaerinTeleportLungeStage::None:
	default:
		break;
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageBlendOut()
{
	if (bLungeSequenceFinished)
	{
		return;
	}

	switch (ActiveLungeStage)
	{
	case EPRFaerinTeleportLungeStage::TeleportOut:
		PlayTeleportInStage();
		break;
	case EPRFaerinTeleportLungeStage::TeleportIn:
		PlayLungeStartStage();
		break;
	case EPRFaerinTeleportLungeStage::LungeStart:
		if (!bLungeMovementActive)
		{
			BeginLungeMovement();
		}
		break;
	case EPRFaerinTeleportLungeStage::LungeEnd:
	case EPRFaerinTeleportLungeStage::LungeLoop:
	case EPRFaerinTeleportLungeStage::None:
	default:
		break;
	}
}

void UPRGameplayAbility_FaerinTeleportLungeSequence::HandleStageMontageInterrupted()
{
	if (!bLungeSequenceFinished)
	{
		FinishLungeSequence(true);
	}
}
