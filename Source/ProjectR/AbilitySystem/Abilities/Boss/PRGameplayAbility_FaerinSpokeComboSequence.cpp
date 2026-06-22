// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Spoke 콤보 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinSpokeComboSequence.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Animation/Notifies/PRAnimNotifyMotionWarpTargetUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinWeaponVisualComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

// ===== Ability 생명주기 =====

UPRGameplayAbility_FaerinSpokeComboSequence::UPRGameplayAbility_FaerinSpokeComboSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_SpokeCombo));
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	const UAbilitySystemComponent* OwnerASC = ActorInfo != nullptr
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;
	if (IsValid(OwnerASC) && OwnerASC->HasMatchingGameplayTag(PRGameplayTags::State_Boss_Faerin_ForcedFollowUp))
	{
		return true;
	}

	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!InitializeSpokeCombo() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBossPatternAttackCommit();
	ActiveTarget = GetBossPatternTarget();
	if (!IsValid(ActiveTarget))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveStage = EPRFaerinSpokeComboStage::Opening;
	ActiveDirection = ResolveFirstDirection();
	bSpokeComboFinished = false;
	bPendingEndCancelled = false;
	ActiveDamageWindow = EPRFaerinSpokeComboDamageWindow::None;
	bHasPreviousBladeTracePoint = false;
	SlamLoopElapsedSeconds = 0.0f;
	SlamLoopMaxSeconds = 0.0f;
	LastSlamTrackingUpdateTime = 0.0f;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();

	const FName OpeningSectionName = GetOpeningSectionName(ActiveDirection);
	BindComboWindowEvent();
	BindMeleeHitWindowEvents();
	if (ActiveDirection == EPRFaerinSpokeComboDirection::None
		|| !IsValidComboSection(OpeningSectionName)
		|| !PlayComboMontage(OpeningSectionName))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RefreshAttackTargetMotionWarp(true);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bSpokeComboFinished = true;
	ClearSpokeComboTimers();
	EndNotifiedDamageWindow();
	EndMeleeHitWindowEventTasks();
	EndComboWindowEventTask();
	StopSpokeComboMovement();
	StopSpokeComboMontage();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveTarget = nullptr;
	WeaponVisualComponent = nullptr;
	ActiveStage = EPRFaerinSpokeComboStage::None;
	ActiveDirection = EPRFaerinSpokeComboDirection::None;
	DamagedActors.Reset();
	PreviousBladeTracePoint = FVector::ZeroVector;

	EndBossPatternAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ===== 초기화 =====

bool UPRGameplayAbility_FaerinSpokeComboSequence::InitializeSpokeCombo()
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !CanRunBossPattern())
	{
		return false;
	}

	ActiveTarget = GetBossPatternTarget();
	if (!IsValid(ActiveTarget))
	{
		return false;
	}

	WeaponVisualComponent = BossCharacter->FindComponentByClass<UPRFaerinWeaponVisualComponent>();
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->ResolveConfiguredComponents();
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}

	return IsValid(SpokeComboMontage);
}

EPRFaerinSpokeComboDirection UPRGameplayAbility_FaerinSpokeComboSequence::ResolveFirstDirection() const
{
	EPRFaerinSpokeComboDirection PreferredDirection = EPRFaerinSpokeComboDirection::Right;
	switch (FirstDirectionPolicy)
	{
	case EPRFaerinSpokeComboFirstDirectionPolicy::Random:
		PreferredDirection = FMath::RandBool()
			? EPRFaerinSpokeComboDirection::Right
			: EPRFaerinSpokeComboDirection::Left;
		break;
	case EPRFaerinSpokeComboFirstDirectionPolicy::ForceRight:
		PreferredDirection = EPRFaerinSpokeComboDirection::Right;
		break;
	case EPRFaerinSpokeComboFirstDirectionPolicy::ForceLeft:
		PreferredDirection = EPRFaerinSpokeComboDirection::Left;
		break;
	case EPRFaerinSpokeComboFirstDirectionPolicy::TargetSide:
	default:
	{
		const AActor* AvatarActor = GetAvatarActorFromActorInfo();
		const AActor* TargetActor = ActiveTarget.Get();
		if (IsValid(AvatarActor) && IsValid(TargetActor))
		{
			FVector DirectionToTarget = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
			DirectionToTarget.Z = 0.0f;
			if (DirectionToTarget.Normalize())
			{
				const float SideDot = FVector::DotProduct(AvatarActor->GetActorRightVector(), DirectionToTarget);
				PreferredDirection = SideDot >= 0.0f
					? EPRFaerinSpokeComboDirection::Right
					: EPRFaerinSpokeComboDirection::Left;
			}
		}
		break;
	}
	}

	if (IsValidComboSection(GetOpeningSectionName(PreferredDirection)))
	{
		return PreferredDirection;
	}

	const EPRFaerinSpokeComboDirection OppositeDirection = GetOppositeDirection(PreferredDirection);
	if (IsValidComboSection(GetOpeningSectionName(OppositeDirection)))
	{
		return OppositeDirection;
	}

	return EPRFaerinSpokeComboDirection::None;
}

EPRFaerinSpokeComboDirection UPRGameplayAbility_FaerinSpokeComboSequence::GetOppositeDirection(
	EPRFaerinSpokeComboDirection Direction) const
{
	if (Direction == EPRFaerinSpokeComboDirection::Right)
	{
		return EPRFaerinSpokeComboDirection::Left;
	}

	if (Direction == EPRFaerinSpokeComboDirection::Left)
	{
		return EPRFaerinSpokeComboDirection::Right;
	}

	return EPRFaerinSpokeComboDirection::None;
}

FName UPRGameplayAbility_FaerinSpokeComboSequence::GetOpeningSectionName(EPRFaerinSpokeComboDirection Direction) const
{
	if (Direction == EPRFaerinSpokeComboDirection::Right)
	{
		return RightOpeningSectionName;
	}

	if (Direction == EPRFaerinSpokeComboDirection::Left)
	{
		return LeftOpeningSectionName;
	}

	return NAME_None;
}

FName UPRGameplayAbility_FaerinSpokeComboSequence::GetFollowUpSectionName(EPRFaerinSpokeComboDirection Direction) const
{
	if (Direction == EPRFaerinSpokeComboDirection::Right)
	{
		return RightFollowUpSectionName;
	}

	if (Direction == EPRFaerinSpokeComboDirection::Left)
	{
		return LeftFollowUpSectionName;
	}

	return NAME_None;
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::IsValidComboSection(FName SectionName) const
{
	return IsValid(SpokeComboMontage)
		&& SectionName != NAME_None
		&& SpokeComboMontage->IsValidSectionName(SectionName);
}

// ===== 몽타주 =====

bool UPRGameplayAbility_FaerinSpokeComboSequence::PlayComboMontage(FName StartSectionName)
{
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SpokeComboMontage,
		FMath::Max(MontagePlayRate, 0.01f),
		StartSectionName);

	if (!IsValid(ActiveMontageTask))
	{
		return false;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageBlendOut);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	ConfigureComboSectionFlow();
	return true;
}

void UPRGameplayAbility_FaerinSpokeComboSequence::ConfigureComboSectionFlow() const
{
	SetComboNextSection(RightOpeningSectionName, RightOpeningSectionName);
	SetComboNextSection(LeftOpeningSectionName, LeftOpeningSectionName);
	SetComboNextSection(RightFollowUpSectionName, RightFollowUpSectionName);
	SetComboNextSection(LeftFollowUpSectionName, LeftFollowUpSectionName);
	SetComboNextSection(SlamStartSectionName, SlamStartSectionName);
	SetComboNextSection(SlamLoopSectionName, SlamLoopSectionName);
	SetComboNextSection(SlamEndSectionName, NAME_None);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::SetComboNextSection(FName FromSectionName, FName ToSectionName) const
{
	if (!IsValid(SpokeComboMontage)
		|| !SpokeComboMontage->IsValidSectionName(FromSectionName)
		|| (ToSectionName != NAME_None && !SpokeComboMontage->IsValidSectionName(ToSectionName)))
	{
		return;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	USkeletalMeshComponent* MeshComponent = IsValid(BossCharacter) ? BossCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_SetNextSection(FromSectionName, ToSectionName, SpokeComboMontage);
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::JumpToComboSection(FName SectionName)
{
	if (!IsValidComboSection(SectionName))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return false;
	}

	RefreshAttackTargetMotionWarp(true);
	ASC->CurrentMontageJumpToSection(SectionName);
	return true;
}

void UPRGameplayAbility_FaerinSpokeComboSequence::BindComboWindowEvent()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	EndComboWindowEventTask();

	constexpr bool bOnlyTriggerOnce = false;
	constexpr bool bOnlyMatchExact = true;
	ActiveComboWindowEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_EnemyComboWindow,
		nullptr,
		bOnlyTriggerOnce,
		bOnlyMatchExact);

	if (IsValid(ActiveComboWindowEventTask))
	{
		ActiveComboWindowEventTask->EventReceived.AddDynamic(
			this,
			&UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboWindowGameplayEvent);
		ActiveComboWindowEventTask->ReadyForActivation();
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::EndComboWindowEventTask()
{
	if (IsValid(ActiveComboWindowEventTask))
	{
		ActiveComboWindowEventTask->EndTask();
		ActiveComboWindowEventTask = nullptr;
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboWindowGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyComboWindow
		|| ActiveStage == EPRFaerinSpokeComboStage::SlamLoop)
	{
		return;
	}

	AdvanceCurrentComboStage();
}

void UPRGameplayAbility_FaerinSpokeComboSequence::AdvanceCurrentComboStage()
{
	if (bSpokeComboFinished)
	{
		return;
	}

	switch (ActiveStage)
	{
	case EPRFaerinSpokeComboStage::Opening:
		EndNotifiedDamageWindow();
		if (ShouldBranchToSlam())
		{
			BeginSlamStart();
			return;
		}
		BeginOppositeFollowUp();
		break;
	case EPRFaerinSpokeComboStage::FollowUp:
		EndNotifiedDamageWindow();
		FinishSpokeCombo(false);
		break;
	case EPRFaerinSpokeComboStage::SlamStart:
		BeginSlamLoop();
		break;
	case EPRFaerinSpokeComboStage::SlamEnd:
		FinishSpokeCombo(bPendingEndCancelled);
		break;
	case EPRFaerinSpokeComboStage::SlamLoop:
	case EPRFaerinSpokeComboStage::None:
	default:
		break;
	}
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::ShouldBranchToSlam() const
{
	const bool bCanUseSlam = IsValidComboSection(SlamStartSectionName)
		&& IsValidComboSection(SlamLoopSectionName)
		&& IsValidComboSection(SlamEndSectionName);
	if (!bCanUseSlam)
	{
		return false;
	}

	if (BranchPolicy == EPRFaerinSpokeComboBranchPolicy::SlamOnly)
	{
		return true;
	}

	if (BranchPolicy == EPRFaerinSpokeComboBranchPolicy::OppositeOnly)
	{
		return false;
	}

	const EPRFaerinSpokeComboDirection OppositeDirection = GetOppositeDirection(ActiveDirection);
	const bool bCanUseOpposite = IsValidComboSection(GetFollowUpSectionName(OppositeDirection));
	if (!bCanUseOpposite)
	{
		return true;
	}

	return FMath::FRand() <= FMath::Clamp(SlamBranchChance, 0.0f, 1.0f);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::BeginOppositeFollowUp()
{
	const EPRFaerinSpokeComboDirection OppositeDirection = GetOppositeDirection(ActiveDirection);
	const FName FollowUpSectionName = GetFollowUpSectionName(OppositeDirection);
	if (OppositeDirection == EPRFaerinSpokeComboDirection::None
		|| !IsValidComboSection(FollowUpSectionName)
		|| !JumpToComboSection(FollowUpSectionName))
	{
		FinishSpokeCombo(false);
		return;
	}

	ActiveDirection = OppositeDirection;
	ActiveStage = EPRFaerinSpokeComboStage::FollowUp;
	EndNotifiedDamageWindow();
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
}

// ===== Slam 분기 =====

void UPRGameplayAbility_FaerinSpokeComboSequence::BeginSlamStart()
{
	if (!JumpToComboSection(SlamStartSectionName))
	{
		FinishSpokeCombo(false);
		return;
	}

	ActiveStage = EPRFaerinSpokeComboStage::SlamStart;
	ActiveDirection = EPRFaerinSpokeComboDirection::None;
	EndNotifiedDamageWindow();
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
	SlamLoopElapsedSeconds = 0.0f;
	SlamLoopMaxSeconds = ResolveSlamLoopMaxSeconds();
	LastSlamTrackingUpdateTime = 0.0f;
}

void UPRGameplayAbility_FaerinSpokeComboSequence::BeginSlamLoop()
{
	if (!JumpToComboSection(SlamLoopSectionName))
	{
		BeginSlamEnd(false);
		return;
	}

	ActiveStage = EPRFaerinSpokeComboStage::SlamLoop;
	SlamLoopElapsedSeconds = 0.0f;
	LastSlamTrackingUpdateTime = 0.0f;

	TickSlamLoop();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SlamLoopTrackingTimerHandle,
			this,
			&UPRGameplayAbility_FaerinSpokeComboSequence::TickSlamLoop,
			FMath::Max(SlamTrackingTickInterval, 0.005f),
			true);
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::TickSlamLoop()
{
	if (bSpokeComboFinished || ActiveStage != EPRFaerinSpokeComboStage::SlamLoop)
	{
		return;
	}

	if (!IsValid(ActiveTarget))
	{
		BeginSlamEnd(true);
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float DeltaTime = LastSlamTrackingUpdateTime > 0.0f
		? CurrentTime - LastSlamTrackingUpdateTime
		: FMath::Max(SlamTrackingTickInterval, 0.005f);
	LastSlamTrackingUpdateTime = CurrentTime;
	SlamLoopElapsedSeconds += FMath::Max(DeltaTime, 0.005f);
	const float DistanceToTarget = CalculateDistanceToTarget();
	if (SlamLoopElapsedSeconds >= SlamMinimumLoopSeconds && DistanceToTarget <= SlamEndTriggerDistance)
	{
		BeginSlamEnd(false);
		return;
	}

	if (SlamLoopMaxSeconds > 0.0f && SlamLoopElapsedSeconds >= SlamLoopMaxSeconds)
	{
		BeginSlamEnd(false);
		return;
	}

	if (bUseCodeDrivenSlamTrackingMovement && !MoveSlamTowardTarget(FMath::Max(DeltaTime, 0.005f)))
	{
		BeginSlamEnd(false);
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::BeginSlamEnd(bool bWasCancelled)
{
	if (bSpokeComboFinished || ActiveStage == EPRFaerinSpokeComboStage::SlamEnd)
	{
		return;
	}

	bPendingEndCancelled = bWasCancelled;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlamLoopTrackingTimerHandle);
	}

	StopSpokeComboMovement();
	SetComboNextSection(SlamLoopSectionName, SlamEndSectionName);
	SetComboNextSection(SlamEndSectionName, NAME_None);
	if (!JumpToComboSection(SlamEndSectionName))
	{
		FinishSpokeCombo(bWasCancelled);
		return;
	}

	ActiveStage = EPRFaerinSpokeComboStage::SlamEnd;
	EndNotifiedDamageWindow();
}

float UPRGameplayAbility_FaerinSpokeComboSequence::ResolveSlamLoopMaxSeconds() const
{
	const float MinSeconds = FMath::Max(SlamLoopMaxSecondsMin, 0.0f);
	const float MaxSeconds = FMath::Max(SlamLoopMaxSecondsMax, MinSeconds);
	if (MaxSeconds <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::FRandRange(MinSeconds, MaxSeconds);
}

float UPRGameplayAbility_FaerinSpokeComboSequence::CalculateDistanceToTarget() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const AActor* TargetActor = ActiveTarget.Get();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist2D(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::MoveSlamTowardTarget(float DeltaTime)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* TargetActor = ActiveTarget.Get();
	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector MoveDirection = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
	if (bConstrainSlamMovementToGround)
	{
		MoveDirection.Z = 0.0f;
	}

	const float DistanceToTarget = MoveDirection.Size();
	if (!MoveDirection.Normalize())
	{
		return false;
	}

	const float MoveDistance = FMath::Min(
		SlamTrackingMoveSpeed * DeltaTime,
		FMath::Max(DistanceToTarget - SlamEndTriggerDistance, 0.0f));
	if (MoveDistance <= 0.0f)
	{
		return true;
	}

	FHitResult SweepHit;
	BossCharacter->AddActorWorldOffset(MoveDirection * MoveDistance, true, &SweepHit);

	if (bDrawDebug)
	{
		DrawDebugLine(
			BossCharacter->GetWorld(),
			BossCharacter->GetActorLocation(),
			BossCharacter->GetActorLocation() + MoveDirection * 160.0f,
			FColor::Cyan,
			false,
			0.1f,
			0,
			2.0f);
	}

	return !SweepHit.bBlockingHit;
}

// ===== Motion Warping =====

bool UPRGameplayAbility_FaerinSpokeComboSequence::RefreshAttackTargetMotionWarp(bool bUseTargetLocation) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	USkeletalMeshComponent* MeshComponent = IsValid(BossCharacter) ? BossCharacter->GetMesh() : nullptr;
	return PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComponent, bUseTargetLocation);
}

// ===== Spoke 1타 피해 판정 =====

void UPRGameplayAbility_FaerinSpokeComboSequence::BindMeleeHitWindowEvents()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	EndMeleeHitWindowEventTasks();

	constexpr bool bOnlyMatchExact = true;
	ActiveMeleeHitWindowBeginEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin,
		nullptr,
		false,
		bOnlyMatchExact);

	if (IsValid(ActiveMeleeHitWindowBeginEventTask))
	{
		ActiveMeleeHitWindowBeginEventTask->EventReceived.AddDynamic(
			this,
			&UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowBeginGameplayEvent);
		ActiveMeleeHitWindowBeginEventTask->ReadyForActivation();
	}

	ActiveMeleeHitWindowTickEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick,
		nullptr,
		false,
		bOnlyMatchExact);

	if (IsValid(ActiveMeleeHitWindowTickEventTask))
	{
		ActiveMeleeHitWindowTickEventTask->EventReceived.AddDynamic(
			this,
			&UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowTickGameplayEvent);
		ActiveMeleeHitWindowTickEventTask->ReadyForActivation();
	}

	ActiveMeleeHitWindowEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd,
		nullptr,
		false,
		bOnlyMatchExact);

	if (IsValid(ActiveMeleeHitWindowEndEventTask))
	{
		ActiveMeleeHitWindowEndEventTask->EventReceived.AddDynamic(
			this,
			&UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowEndGameplayEvent);
		ActiveMeleeHitWindowEndEventTask->ReadyForActivation();
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::EndMeleeHitWindowEventTasks()
{
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
}

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin)
	{
		return;
	}

	BeginNotifiedDamageWindow();
}

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick)
	{
		return;
	}

	TickNotifiedDamageWindow();
}

// ===== Slam End 피해 판정 =====

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd)
	{
		return;
	}

	EndNotifiedDamageWindow();
}

void UPRGameplayAbility_FaerinSpokeComboSequence::BeginNotifiedDamageWindow()
{
	if (bSpokeComboFinished)
	{
		return;
	}

	switch (ActiveStage)
	{
	case EPRFaerinSpokeComboStage::Opening:
	case EPRFaerinSpokeComboStage::FollowUp:
		ActiveDamageWindow = EPRFaerinSpokeComboDamageWindow::Spoke;
		break;
	case EPRFaerinSpokeComboStage::SlamEnd:
		ActiveDamageWindow = EPRFaerinSpokeComboDamageWindow::SlamEnd;
		break;
	case EPRFaerinSpokeComboStage::SlamStart:
	case EPRFaerinSpokeComboStage::SlamLoop:
	case EPRFaerinSpokeComboStage::None:
	default:
		return;
	}

	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(true);
	}

	TickNotifiedDamageWindow();
}

void UPRGameplayAbility_FaerinSpokeComboSequence::TickNotifiedDamageWindow()
{
	float DamageMultiplier = 0.0f;
	float PoiseDamage = 0.0f;
	switch (ActiveDamageWindow)
	{
	case EPRFaerinSpokeComboDamageWindow::Spoke:
		DamageMultiplier = AttackDamageMultiplier;
		PoiseDamage = AttackPoiseDamage;
		break;
	case EPRFaerinSpokeComboDamageWindow::SlamEnd:
		DamageMultiplier = SlamDamageMultiplier;
		PoiseDamage = SlamPoiseDamage;
		break;
	case EPRFaerinSpokeComboDamageWindow::None:
	default:
		return;
	}

	FVector CurrentTracePoint = FVector::ZeroVector;
	if (!GetCurrentBladeTracePoint(CurrentTracePoint))
	{
		return;
	}

	if (!bHasPreviousBladeTracePoint)
	{
		ApplyDamageTrace(CurrentTracePoint, CurrentTracePoint, DamageMultiplier, PoiseDamage);
		PreviousBladeTracePoint = CurrentTracePoint;
		bHasPreviousBladeTracePoint = true;
		return;
	}

	ApplyDamageTrace(PreviousBladeTracePoint, CurrentTracePoint, DamageMultiplier, PoiseDamage);
	PreviousBladeTracePoint = CurrentTracePoint;
}

void UPRGameplayAbility_FaerinSpokeComboSequence::EndNotifiedDamageWindow()
{
	ActiveDamageWindow = EPRFaerinSpokeComboDamageWindow::None;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}
}

// ===== 공통 피해 판정 =====

bool UPRGameplayAbility_FaerinSpokeComboSequence::GetCurrentBladeTracePoint(FVector& OutTracePoint) const
{
	if (IsValid(WeaponVisualComponent) && IsValid(WeaponVisualComponent->GetBladeHitBox()))
	{
		const FTransform HitBoxTransform = WeaponVisualComponent->GetBladeHitBox()->GetComponentTransform();
		OutTracePoint = HitBoxTransform.GetLocation() + HitBoxTransform.TransformVectorNoScale(HitTraceOffset);
		return true;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
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

void UPRGameplayAbility_FaerinSpokeComboSequence::ApplyDamageTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	float InDamageMultiplier,
	float InPoiseDamage)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaerinSpokeCombo), false, AvatarActor);
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

		ApplyAttackPowerDamage(HitActor, InDamageMultiplier, InPoiseDamage, &HitResult);
		DamagedActors.Add(HitActor);
	}
}

bool UPRGameplayAbility_FaerinSpokeComboSequence::ShouldDamageActor(AActor* CandidateActor) const
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

	return CandidateActor == ActiveTarget.Get();
}

// ===== 정리 =====

void UPRGameplayAbility_FaerinSpokeComboSequence::ClearSpokeComboTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlamLoopTrackingTimerHandle);
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::StopSpokeComboMontage() const
{
	if (!IsValid(SpokeComboMontage))
	{
		return;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	USkeletalMeshComponent* MeshComponent = IsValid(BossCharacter) ? BossCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsPlaying(SpokeComboMontage))
	{
		return;
	}

	AnimInstance->Montage_Stop(FMath::Max(MontageStopBlendOutTime, 0.0f), SpokeComboMontage);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::StopSpokeComboMovement() const
{
	if (APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter())
	{
		if (UCharacterMovementComponent* MovementComponent = BossCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void UPRGameplayAbility_FaerinSpokeComboSequence::FinishSpokeCombo(bool bWasCancelled)
{
	if (bSpokeComboFinished)
	{
		return;
	}

	bSpokeComboFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

// ===== 몽타주 콜백 =====

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageCompleted()
{
	if (bSpokeComboFinished)
	{
		return;
	}

	ActiveMontageTask = nullptr;
	FinishSpokeCombo(bPendingEndCancelled);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageBlendOut()
{
	if (bSpokeComboFinished)
	{
		return;
	}

	if (ActiveStage == EPRFaerinSpokeComboStage::SlamEnd)
	{
		FinishSpokeCombo(bPendingEndCancelled);
		return;
	}

	FinishSpokeCombo(false);
}

void UPRGameplayAbility_FaerinSpokeComboSequence::HandleComboMontageInterrupted()
{
	if (!bSpokeComboFinished)
	{
		FinishSpokeCombo(true);
	}
}
