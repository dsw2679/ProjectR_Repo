// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (근접 피격 반응 연동 구현)
// Author: 배유찬 (근접 데미지 계산 및 파이프라인 연동)
// Author: 손승우 (해머 근접 공격 판정 및 모션 워핑 연동 구현)
// Author: 이건주 (Penitent 몬스터 근접 배리어 패턴 구현)
#include "PRGameplayAbility_EnemyMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyMeleeAttack::UPRGameplayAbility_EnemyMeleeAttack()
{
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);

	DefaultHitConfig.DamageMultiplier = 1.0f;
	DefaultHitConfig.PoiseDamage = 0.0f;
	DefaultHitConfig.CollisionConfig.CollisionSource = EPREnemyAttackCollisionSource::SocketOrBone;
	DefaultHitConfig.CollisionConfig.AttackRange = 220.0f;
	DefaultHitConfig.CollisionConfig.AttackRadius = 75.0f;
	DefaultHitConfig.CollisionConfig.TraceChannel = ECC_Pawn;
}

void UPRGameplayAbility_EnemyMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		// 공격 실행 상태 태그
		AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_Enemy_Attacking);
	}

	DamagedActors.Reset();
	bMeleeAttackFinished = false;
	bMeleeHitTriggered = false;
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
	PreviousMeleeWindowPhysicsBodyTracePoints.Reset();

	if (bUseDefaultHitConfig)
	{
		ApplyEnemyAttackHitConfig(DefaultHitConfig);
	}

	if (ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor))
	{
		// 공격 시작 시 BT 이동 잔여 속도 제거
		SourceCharacter->GetCharacterMovement()->StopMovementImmediately();
	}

	BeginThreatAttackCommit();
	RefreshAttackFacing(bFaceTargetOnAbilityStart);

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bHasAttackMontage = IsValid(AttackMontage);
	if (bHasAttackMontage)
	{
		// Ability 종료와 몽타주 정리/네트워크 처리를 함께 다루는 GAS 몽타주 태스크
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
			MontageStartSection);

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	if (bUseAnimationNotifyForHit)
	{
		const bool bOnlyTriggerOnce = !bAllowMultipleMeleeHits;
		constexpr bool bOnlyMatchExact = true;

		// 한 프레임 타격 Notify 이벤트
		ActiveMeleeHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeHit,
			nullptr,
			bOnlyTriggerOnce,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitEventTask))
		{
			ActiveMeleeHitEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitGameplayEvent);
			ActiveMeleeHitEventTask->ReadyForActivation();
		}

		// 구간 판정 시작 이벤트
		ActiveMeleeHitWindowBeginEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowBeginEventTask))
		{
			ActiveMeleeHitWindowBeginEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowBeginGameplayEvent);
			ActiveMeleeHitWindowBeginEventTask->ReadyForActivation();
		}

		// 구간 판정 갱신 이벤트
		ActiveMeleeHitWindowTickEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowTickEventTask))
		{
			ActiveMeleeHitWindowTickEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowTickGameplayEvent);
			ActiveMeleeHitWindowTickEventTask->ReadyForActivation();
		}

		// 구간 판정 종료 이벤트
		ActiveMeleeHitWindowEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowEndEventTask))
		{
			ActiveMeleeHitWindowEndEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowEndGameplayEvent);
			ActiveMeleeHitWindowEndEventTask->ReadyForActivation();
		}
	}

	const bool bUseTimedHit = !bHasAttackMontage
		|| !bUseAnimationNotifyForHit
		|| bAllowTimedHitFallbackWhenMontagePlays;

	// 몽타주 없음 또는 fallback 허용 시 WindupTime 타이머 사용
	if (bUseTimedHit && WindupTime <= 0.0f)
	{
		TriggerMeleeHitOnce();
	}
	else if (bUseTimedHit)
	{
		World->GetTimerManager().SetTimer(HitTimerHandle, this,
			&UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitOnce, WindupTime, false);
	}

	const float TimerFinishDelay = FMath::Max(WindupTime + RecoveryTime, 0.01f);
	const float MontageFinishDelay = bHasAttackMontage && bUseMontageDurationForFinish
		? (AttackMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)) + 0.1f
		: 0.0f;
	const float FinishDelay = FMath::Max(TimerFinishDelay, MontageFinishDelay);
	World->GetTimerManager().SetTimer(FinishTimerHandle, this,
		&UPRGameplayAbility_EnemyMeleeAttack::FinishMeleeAttack, FinishDelay, false);
}

void UPRGameplayAbility_EnemyMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimerHandle);
		World->GetTimerManager().ClearTimer(FinishTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitEventTask))
	{
		ActiveMeleeHitEventTask->EndTask();
		ActiveMeleeHitEventTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitWindowBeginEventTask))
	{
		ActiveMeleeHitWindowBeginEventTask->EndTask();
		ActiveMeleeHitWindowBeginEventTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitWindowTickEventTask))
	{
		ActiveMeleeHitWindowTickEventTask->EndTask();
		ActiveMeleeHitWindowTickEventTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitWindowEndEventTask))
	{
		ActiveMeleeHitWindowEndEventTask->EndTask();
		ActiveMeleeHitWindowEndEventTask = nullptr;
	}

	DamagedActors.Reset();
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
	PreviousMeleeWindowPhysicsBodyTracePoints.Reset();

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		// 공격 실행 상태 태그 정리
		AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Attacking);
	}

	EndThreatAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitFromAnimation()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	TriggerMeleeHitOnce();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeHit)
	{
		return;
	}

	if (bMeleeHitWindowActive)
	{
		return;
	}

	TriggerMeleeHitOnce();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin)
	{
		return;
	}

	BeginMeleeHitWindow();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick)
	{
		return;
	}

	UpdateMeleeHitWindow();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd)
	{
		return;
	}

	EndMeleeHitWindow();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted()
{
	if (bUseMontageDurationForFinish)
	{
		FinishMeleeAttack();
	}
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitOnce()
{
	if (bMeleeAttackFinished || (!bAllowMultipleMeleeHits && bMeleeHitTriggered))
	{
		return;
	}

	bMeleeHitTriggered = true;
	if (bAllowMultipleMeleeHits)
	{
		DamagedActors.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimerHandle);
	}

	ExecuteMeleeHit();
}

void UPRGameplayAbility_EnemyMeleeAttack::ExecuteMeleeHit()
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
	{
		return;
	}

	RefreshAttackFacing(bFaceTargetOnMeleeHit);
	ApplyForwardLunge(SourceCharacter);

	if (UsesPhysicsAssetBodyCollision() && ExecutePhysicsBodyMeleeHit())
	{
		return;
	}

	FVector TracePoint = FVector::ZeroVector;
	if (CollisionSource == EPREnemyAttackCollisionSource::SocketOrBone
		&& GetCurrentAttackTracePoint(TracePoint))
	{
		ExecuteMeleeTrace(TracePoint, TracePoint);
		return;
	}

	const FVector Forward = SourceCharacter->GetActorForwardVector();
	const FVector TraceStart = SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeightOffset);
	const FVector TraceEnd = TraceStart + Forward * AttackRange;
	ExecuteMeleeTrace(TraceStart, TraceEnd);
}

void UPRGameplayAbility_EnemyMeleeAttack::ApplyEnemyAttackHitConfig(const FPREnemyAttackHitConfig& HitConfig)
{
	DamageMultiplier = HitConfig.DamageMultiplier;
	PoiseDamage = HitConfig.PoiseDamage;
	CollisionSource = HitConfig.CollisionConfig.CollisionSource;
	AttackRange = HitConfig.CollisionConfig.AttackRange;
	AttackRadius = HitConfig.CollisionConfig.AttackRadius;
	AttackTraceSourceName = HitConfig.CollisionConfig.AttackTraceSourceName;
	AttackTraceSourceOffset = HitConfig.CollisionConfig.AttackTraceSourceOffset;
	AttackPhysicsBodyNames = HitConfig.CollisionConfig.AttackPhysicsBodyNames;
	PhysicsBodyScale = HitConfig.CollisionConfig.PhysicsBodyScale;
	TraceChannel = HitConfig.CollisionConfig.TraceChannel;
}

void UPRGameplayAbility_EnemyMeleeAttack::BeginMeleeHitWindow()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeHitWindowActive = true;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
	PreviousMeleeWindowPhysicsBodyTracePoints.Reset();
	DamagedActors.Reset();
	RefreshAttackFacing(bFaceTargetOnMeleeHit);
}

void UPRGameplayAbility_EnemyMeleeAttack::UpdateMeleeHitWindow()
{
	if (bMeleeAttackFinished || !bMeleeHitWindowActive)
	{
		return;
	}

	if (UsesPhysicsAssetBodyCollision() && UpdatePhysicsBodyMeleeHitWindow())
	{
		return;
	}

	FVector CurrentTracePoint = FVector::ZeroVector;
	if (CollisionSource != EPREnemyAttackCollisionSource::SocketOrBone
		|| !GetCurrentAttackTracePoint(CurrentTracePoint))
	{
		ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
		{
			return;
		}

		const FVector TraceStart = SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeightOffset);
		const FVector TraceEnd = TraceStart + (SourceCharacter->GetActorForwardVector() * AttackRange);
		ExecuteMeleeTrace(TraceStart, TraceEnd);
		return;
	}

	if (!bHasPreviousMeleeWindowTracePoint)
	{
		ExecuteMeleeTrace(CurrentTracePoint, CurrentTracePoint);
		bHasPreviousMeleeWindowTracePoint = true;
		PreviousMeleeWindowTracePoint = CurrentTracePoint;
		return;
	}

	ExecuteMeleeTrace(PreviousMeleeWindowTracePoint, CurrentTracePoint);
	PreviousMeleeWindowTracePoint = CurrentTracePoint;
}

void UPRGameplayAbility_EnemyMeleeAttack::EndMeleeHitWindow()
{
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
	PreviousMeleeWindowPhysicsBodyTracePoints.Reset();
}

void UPRGameplayAbility_EnemyMeleeAttack::ExecuteMeleeTrace(const FVector& TraceStart, const FVector& TraceEnd)
{
	ExecuteMeleeTraceWithRadius(TraceStart, TraceEnd, AttackRadius);
}

bool UPRGameplayAbility_EnemyMeleeAttack::UsesPhysicsAssetBodyCollision() const
{
	return CollisionSource == EPREnemyAttackCollisionSource::PhysicsAssetBody
		&& !AttackPhysicsBodyNames.IsEmpty();
}

bool UPRGameplayAbility_EnemyMeleeAttack::ExecutePhysicsBodyMeleeHit()
{
	bool bExecutedTrace = false;
	const float BodyTraceRadius = AttackRadius * FMath::Max(PhysicsBodyScale, 0.0f);

	for (const FName BodyName : AttackPhysicsBodyNames)
	{
		FVector TracePoint = FVector::ZeroVector;
		if (!GetAttackPhysicsBodyTracePoint(BodyName, TracePoint))
		{
			continue;
		}

		ExecuteMeleeTraceWithRadius(TracePoint, TracePoint, BodyTraceRadius);
		bExecutedTrace = true;
	}

	return bExecutedTrace;
}

bool UPRGameplayAbility_EnemyMeleeAttack::UpdatePhysicsBodyMeleeHitWindow()
{
	bool bExecutedTrace = false;
	const float BodyTraceRadius = AttackRadius * FMath::Max(PhysicsBodyScale, 0.0f);

	for (const FName BodyName : AttackPhysicsBodyNames)
	{
		FVector CurrentTracePoint = FVector::ZeroVector;
		if (!GetAttackPhysicsBodyTracePoint(BodyName, CurrentTracePoint))
		{
			continue;
		}

		if (const FVector* PreviousTracePoint = PreviousMeleeWindowPhysicsBodyTracePoints.Find(BodyName))
		{
			ExecuteMeleeTraceWithRadius(*PreviousTracePoint, CurrentTracePoint, BodyTraceRadius);
		}
		else
		{
			ExecuteMeleeTraceWithRadius(CurrentTracePoint, CurrentTracePoint, BodyTraceRadius);
		}

		PreviousMeleeWindowPhysicsBodyTracePoints.Add(BodyName, CurrentTracePoint);
		bExecutedTrace = true;
	}

	return bExecutedTrace;
}

bool UPRGameplayAbility_EnemyMeleeAttack::GetAttackPhysicsBodyTracePoint(FName BodyName, FVector& OutTracePoint) const
{
	const ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || BodyName == NAME_None)
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComp = SourceCharacter->GetMesh();
	if (!IsValid(MeshComp))
	{
		return false;
	}

	if (const FBodyInstance* BodyInstance = MeshComp->GetBodyInstance(BodyName))
	{
		const FTransform BodyTransform = BodyInstance->GetUnrealWorldTransform();
		const FVector WorldOffset = BodyTransform.TransformVectorNoScale(AttackTraceSourceOffset);
		OutTracePoint = BodyTransform.GetLocation() + WorldOffset;
		return true;
	}

	const bool bHasSocket = MeshComp->DoesSocketExist(BodyName);
	const bool bHasBone = MeshComp->GetBoneIndex(BodyName) != INDEX_NONE;
	if (!bHasSocket && !bHasBone)
	{
		return false;
	}

	const FTransform SourceTransform = MeshComp->GetSocketTransform(BodyName, RTS_World);
	const FVector WorldOffset = SourceTransform.TransformVectorNoScale(AttackTraceSourceOffset);
	OutTracePoint = SourceTransform.GetLocation() + WorldOffset;
	return true;
}

void UPRGameplayAbility_EnemyMeleeAttack::ExecuteMeleeTraceWithRadius(const FVector& TraceStart, const FVector& TraceEnd, float TraceRadius)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyMeleeAttack), false, SourceCharacter);
	QueryParams.AddIgnoredActor(SourceCharacter);
	
	TArray<FHitResult> HitResults;
	const float ClampedTraceRadius = FMath::Max(TraceRadius, 0.0f);
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ClampedTraceRadius);
	const bool bHit = SourceCharacter->GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		TraceChannel,
		CollisionShape,
		QueryParams);

	if (bDrawDebug)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(SourceCharacter->GetWorld(), TraceStart, TraceEnd, DebugColor, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(SourceCharacter->GetWorld(), TraceStart, ClampedTraceRadius, 16, DebugColor, false, 1.0f);
		DrawDebugSphere(SourceCharacter->GetWorld(), TraceEnd, ClampedTraceRadius, 16, DebugColor, false, 1.0f);
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

void UPRGameplayAbility_EnemyMeleeAttack::FinishMeleeAttack()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_EnemyMeleeAttack::ApplyForwardLunge(ACharacter* SourceCharacter) const
{
	if (!bUseForwardLunge || !IsValid(SourceCharacter) || LungeDistance <= 0.0f)
	{
		return;
	}

	const FVector MoveDelta = SourceCharacter->GetActorForwardVector() * LungeDistance;
	FHitResult SweepHit;
	SourceCharacter->AddActorWorldOffset(MoveDelta, true, &SweepHit);
}

AActor* UPRGameplayAbility_EnemyMeleeAttack::GetCurrentThreatTarget() const
{
	const APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	const UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetAttackTarget()
		: nullptr;
}

void UPRGameplayAbility_EnemyMeleeAttack::BeginThreatAttackCommit()
{
	APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	AActor* TargetActor = ThreatComponent->GetAttackTarget();
	const FVector TargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : FVector::ZeroVector;
	ThreatComponent->BeginAttackCommit(TargetActor, TargetLocation);
}

void UPRGameplayAbility_EnemyMeleeAttack::EndThreatAttackCommit()
{
	APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	ThreatComponent->EndAttackCommit();
}

void UPRGameplayAbility_EnemyMeleeAttack::RefreshAttackFacing(bool bApplyActorRotation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = GetCurrentThreatTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector DirectionToTarget = TargetLocation - AvatarLocation;
	DirectionToTarget.Z = 0.0f;
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	FRotator FacingRotation = DirectionToTarget.Rotation();
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;

	if (bApplyActorRotation)
	{
		AvatarActor->SetActorRotation(FacingRotation);
	}
}

bool UPRGameplayAbility_EnemyMeleeAttack::GetCurrentAttackTracePoint(FVector& OutTracePoint) const
{
	const ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter))
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComp = SourceCharacter->GetMesh();
	if (!IsValid(MeshComp) || AttackTraceSourceName == NAME_None)
	{
		return false;
	}

	const bool bHasSocket = MeshComp->DoesSocketExist(AttackTraceSourceName);
	const bool bHasBone = MeshComp->GetBoneIndex(AttackTraceSourceName) != INDEX_NONE;
	if (!bHasSocket && !bHasBone)
	{
		return false;
	}

	const FTransform SourceTransform = MeshComp->GetSocketTransform(AttackTraceSourceName, RTS_World);
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FVector WorldOffset = SourceTransform.TransformVectorNoScale(AttackTraceSourceOffset);
	
	OutTracePoint = SourceLocation + WorldOffset;
	return true;
}

bool UPRGameplayAbility_EnemyMeleeAttack::ShouldDamageActor(const AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || CandidateActor == GetAvatarActorFromActorInfo())
	{
		return false;
	}

	const UAbilitySystemComponent* CandidateAbilitySystem = UPRCombatStatics::FindAbilitySystemComponent(CandidateActor);
	if (!IsValid(CandidateAbilitySystem))
	{
		return false;
	}

	// 다운/사망한 플레이어는 범위에 들어와도 추가 피해 대상으로 보지 않는다.
	if (UPRCombatStatics::GetActorTeam(CandidateActor) == EPRTeam::Player
		&& (CandidateAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Down)
			|| CandidateAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dead)))
	{
		return false;
	}

	if (UPRCombatStatics::GetActorTeam(CandidateActor) == EPRTeam::Enemy)
	{
		return false;
	}

	if (!bOnlyDamageThreatTarget)
	{
		return true;
	}

	return GetCurrentThreatTarget() == CandidateActor;
}
