// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCloneCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Animation/Notifies/PRAnimNotifyMotionWarpTargetUtils.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinWeaponVisualComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "TimerManager.h"

namespace
{
	constexpr float DefaultCloneRotationRate = 720.0f;
}

APRFaerinCloneCharacter::APRFaerinCloneCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.0f);

	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseTickOptimization = false;
	bIsRespawnable = false;
	bCanEnterGroggyState = false;
	bUseWorldHealthBar = true;
	CharacterID = TEXT("FaerinClone");

	GetCharacterMovement()->MaxWalkSpeed = 420.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, DefaultCloneRotationRate, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

/*~ AActor Interface ~*/

void APRFaerinCloneCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bCloneResolved)
	{
		return;
	}

	switch (CloneState)
	{
	case EPRFaerinCloneState::Spawning:
		TickSpawning(DeltaTime);
		break;
	case EPRFaerinCloneState::ChasingTarget:
		TickChasingTarget(DeltaTime);
		break;
	case EPRFaerinCloneState::SpokeCombo:
		TickSpokeCombo(DeltaTime);
		break;
	case EPRFaerinCloneState::ReturningToOwner:
		TickReturningToOwner(DeltaTime);
		break;
	case EPRFaerinCloneState::Inactive:
	case EPRFaerinCloneState::Merged:
	case EPRFaerinCloneState::Killed:
	default:
		break;
	}
}

void APRFaerinCloneCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinCloneCharacter, CloneState);
}

void APRFaerinCloneCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSpokeComboEvents();
	ClearCloneSpawnDissolveVisual();
	Super::EndPlay(EndPlayReason);
}

// ===== 공개 함수 =====

void APRFaerinCloneCharacter::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	Super::OnPostDamageApplied(Context);

	if (!HasAuthority() || bCloneResolved || !IsValid(CommonSet))
	{
		return;
	}

	if (CommonSet->GetHealth() <= UE_SMALL_NUMBER)
	{
		HandleCloneKilled();
	}
}

void APRFaerinCloneCharacter::InitializeFaerinClone(
	APRFaerinCharacter* InOwnerFaerin,
	AActor* InAssignedTarget,
	const FPRFaerinCloneRuntimeConfig& InRuntimeConfig)
{
	if (!HasAuthority() || !IsValid(InOwnerFaerin) || !IsValid(InAssignedTarget))
	{
		return;
	}

	OwnerFaerin = InOwnerFaerin;
	AssignedTarget = InAssignedTarget;
	RuntimeConfig = InRuntimeConfig;
	bCloneResolved = false;
	bCanApplyHealOnMerge = false;
	StateElapsedSeconds = 0.0f;

	InitializeCloneAbilitySystem();

	if (IsValid(ThreatComponent))
	{
		ThreatComponent->ForceCurrentTarget(AssignedTarget);
	}

	WeaponVisualComponent = FindComponentByClass<UPRFaerinWeaponVisualComponent>();
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->ResolveConfiguredComponents();
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = FMath::Max(RuntimeConfig.ChaseMoveSpeed, 0.0f);
		MovementComponent->StopMovementImmediately();
	}

	CloneState = EPRFaerinCloneState::Spawning;
	SetActorTickEnabled(true);
	ForceNetUpdate();

	Multicast_PlayCloneSpawnDissolveVisual(
		RuntimeConfig.SpawnDissolveNiagaraSystem,
		RuntimeConfig.SpawnDissolveDuration,
		RuntimeConfig.SpawnDissolveStartValue,
		RuntimeConfig.SpawnDissolveEndValue,
		RuntimeConfig.SpawnDissolveScalarParameterName,
		RuntimeConfig.SpawnNiagaraDissolveParameterName,
		RuntimeConfig.SpawnDissolveTexture,
		RuntimeConfig.SpawnDissolveTextureUV,
		RuntimeConfig.SpawnDissolveTickInterval);
}

// ===== 상태 태그 =====

void APRFaerinCloneCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, bTagExists);

	if (HasAuthority()
		&& bTagExists
		&& ChangedTag.MatchesTag(PRGameplayTags::State_Dead)
		&& !bCloneResolved)
	{
		HandleCloneKilled();
	}
}

// ===== 상태 머신 =====

void APRFaerinCloneCharacter::TickSpawning(float DeltaTime)
{
	StateElapsedSeconds += DeltaTime;
	if (StateElapsedSeconds >= FMath::Max(RuntimeConfig.SpawnIntroSeconds, 0.0f))
	{
		BeginChasingTarget();
	}
}

void APRFaerinCloneCharacter::TickChasingTarget(float DeltaTime)
{
	StateElapsedSeconds += DeltaTime;
	if (!IsValid(OwnerFaerin) || OwnerFaerin->IsDead())
	{
		DestroyCloneWithoutHeal();
		return;
	}

	if (!IsAssignedTargetAttackable())
	{
		BeginReturningToOwner(false);
		return;
	}

	const float DistanceToTarget = CalculateDistanceToTarget();
	if (DistanceToTarget <= FMath::Max(RuntimeConfig.SpokeStartRange, 0.0f))
	{
		BeginSpokeCombo();
		return;
	}

	if (RuntimeConfig.ChaseMaxSeconds > 0.0f && StateElapsedSeconds >= RuntimeConfig.ChaseMaxSeconds)
	{
		BeginReturningToOwner(false);
		return;
	}

	MoveTowardAssignedTarget(DeltaTime);
}

void APRFaerinCloneCharacter::TickSpokeCombo(float DeltaTime)
{
	SpokeComboElapsedSeconds += DeltaTime;
	if (!IsValid(OwnerFaerin) || OwnerFaerin->IsDead())
	{
		CompleteSpokeCombo(false);
		return;
	}

	if (ActiveSpokeStage == EPRFaerinCloneSpokeStage::SlamLoop)
	{
		SlamLoopElapsedSeconds += DeltaTime;
		const float DistanceToTarget = CalculateDistanceToTarget();
		if (SlamLoopElapsedSeconds >= RuntimeConfig.SlamMinimumLoopSeconds
			&& DistanceToTarget <= RuntimeConfig.SlamEndTriggerDistance)
		{
			BeginSlamEnd(false);
			return;
		}

		if (SlamLoopMaxSeconds > 0.0f && SlamLoopElapsedSeconds >= SlamLoopMaxSeconds)
		{
			BeginSlamEnd(false);
			return;
		}

		if (RuntimeConfig.bUseCodeDrivenSlamTrackingMovement)
		{
			MoveSlamTowardTarget(DeltaTime);
		}
	}

	RefreshAttackTargetMotionWarp(true);

	if (SpokeComboElapsedSeconds >= FMath::Max(RuntimeConfig.ComboFailsafeSeconds, 0.1f))
	{
		CompleteSpokeCombo(false);
	}
}

void APRFaerinCloneCharacter::TickReturningToOwner(float DeltaTime)
{
	StateElapsedSeconds += DeltaTime;
	if (!IsValid(OwnerFaerin) || OwnerFaerin->IsDead())
	{
		DestroyCloneWithoutHeal();
		return;
	}

	const float ReturnDelay = FMath::Max(RuntimeConfig.ReturnDelayAfterSpokeCombo, 0.0f);
	if (StateElapsedSeconds < ReturnDelay)
	{
		FaceLocation(OwnerFaerin->GetActorLocation(), DeltaTime);
		return;
	}

	const float ReturnElapsed = StateElapsedSeconds - ReturnDelay;
	if (RuntimeConfig.ReturnMaxSeconds > 0.0f && ReturnElapsed >= RuntimeConfig.ReturnMaxSeconds)
	{
		DestroyCloneWithoutHeal();
		return;
	}

	const USkeletalMeshComponent* OwnerMesh = OwnerFaerin->GetMesh();
	FVector MergeLocation = OwnerFaerin->GetActorLocation();
	if (IsValid(OwnerMesh)
		&& RuntimeConfig.MergeSocketName != NAME_None
		&& OwnerMesh->DoesSocketExist(RuntimeConfig.MergeSocketName))
	{
		const FTransform SocketTransform = OwnerMesh->GetSocketTransform(RuntimeConfig.MergeSocketName, RTS_World);
		MergeLocation = SocketTransform.TransformPosition(RuntimeConfig.MergeLocationOffset);
	}
	else
	{
		MergeLocation = OwnerFaerin->GetActorTransform().TransformPosition(RuntimeConfig.MergeLocationOffset);
	}

	const float DistanceToMerge = FVector::Dist(GetActorLocation(), MergeLocation);
	if (DistanceToMerge <= FMath::Max(RuntimeConfig.ReturnAcceptanceRadius, 0.0f))
	{
		CompleteMerge();
		return;
	}

	const FVector OldLocation = GetActorLocation();
	const FVector NewLocation = FMath::VInterpConstantTo(
		OldLocation,
		MergeLocation,
		DeltaTime,
		FMath::Max(RuntimeConfig.ReturnSpeed, 0.0f));
	SetActorLocation(NewLocation, false);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->Velocity = DeltaTime > UE_SMALL_NUMBER
			? (GetActorLocation() - OldLocation) / DeltaTime
			: FVector::ZeroVector;
	}
	FaceLocation(MergeLocation, DeltaTime);
}

void APRFaerinCloneCharacter::BeginChasingTarget()
{
	if (!IsAssignedTargetAttackable())
	{
		BeginReturningToOwner(false);
		return;
	}

	CloneState = EPRFaerinCloneState::ChasingTarget;
	StateElapsedSeconds = 0.0f;
	FPREnemyMovePresentationConfig PresentationConfig;
	PresentationConfig.bMaintainTargetFocus = true;
	PresentationConfig.bUseCombatMovePose = true;
	PresentationConfig.bUseCombatAimOffset = false;
	PresentationConfig.bUseCombatStrafeState = false;
	PresentationConfig.bUseControllerDesiredRotation = false;
	PresentationConfig.bOrientRotationToMovement = true;
	PresentationConfig.MoveSpeedOverride = FMath::Max(RuntimeConfig.ChaseMoveSpeed, 0.0f);
	PresentationConfig.RotationYawRate = DefaultCloneRotationRate;
	ApplyCombatMovePresentationContext(PresentationConfig);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = FMath::Max(RuntimeConfig.ChaseMoveSpeed, 0.0f);
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
	}
	ForceNetUpdate();
}

void APRFaerinCloneCharacter::BeginSpokeCombo()
{
	if (!IsValidComboSection(RuntimeConfig.RightOpeningSectionName)
		&& !IsValidComboSection(RuntimeConfig.LeftOpeningSectionName))
	{
		BeginReturningToOwner(false);
		return;
	}

	StopCloneMovement();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->ForceCurrentTarget(AssignedTarget);
		ThreatComponent->BeginAttackCommit(AssignedTarget, IsValid(AssignedTarget) ? AssignedTarget->GetActorLocation() : FVector::ZeroVector);
	}

	ActiveSpokeDirection = ResolveFirstDirection();
	const FName OpeningSectionName = GetOpeningSectionName(ActiveSpokeDirection);
	if (ActiveSpokeDirection == EPRFaerinCloneSpokeDirection::None || !IsValidComboSection(OpeningSectionName))
	{
		BeginReturningToOwner(false);
		return;
	}

	CloneState = EPRFaerinCloneState::SpokeCombo;
	ActiveSpokeStage = EPRFaerinCloneSpokeStage::Opening;
	ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::None;
	bPendingSlamEndCancelled = false;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
	StateElapsedSeconds = 0.0f;
	SpokeComboElapsedSeconds = 0.0f;
	SlamLoopElapsedSeconds = 0.0f;
	SlamLoopMaxSeconds = 0.0f;

	BindSpokeComboEvents();
	Multicast_PlayCloneSpokeMontage(RuntimeConfig.SpokeComboMontage, RuntimeConfig.MontagePlayRate, OpeningSectionName);
	RefreshAttackTargetMotionWarp(true);
	ForceNetUpdate();
}

void APRFaerinCloneCharacter::CompleteSpokeCombo(bool bCompletedNormally)
{
	if (CloneState != EPRFaerinCloneState::SpokeCombo || bCloneResolved)
	{
		return;
	}

	UnbindSpokeComboEvents();
	EndNotifiedDamageWindow();
	StopCloneMovement();
	StopSpokeComboMontage();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->EndAttackCommit();
	}

	ActiveSpokeStage = EPRFaerinCloneSpokeStage::None;
	ActiveSpokeDirection = EPRFaerinCloneSpokeDirection::None;
	ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::None;
	BeginReturningToOwner(bCompletedNormally);
}

void APRFaerinCloneCharacter::BeginReturningToOwner(bool bShouldHealOnMerge)
{
	if (bCloneResolved)
	{
		return;
	}

	UnbindSpokeComboEvents();
	EndNotifiedDamageWindow();
	StopSpokeComboMontage();
	StopCloneMovement();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->ForceClearAttackCommit();
	}

	CloneState = EPRFaerinCloneState::ReturningToOwner;
	bCanApplyHealOnMerge = bShouldHealOnMerge;
	StateElapsedSeconds = 0.0f;
	FPREnemyMovePresentationConfig PresentationConfig;
	PresentationConfig.bMaintainTargetFocus = false;
	PresentationConfig.bUseCombatMovePose = true;
	PresentationConfig.bUseCombatAimOffset = false;
	PresentationConfig.bUseCombatStrafeState = false;
	PresentationConfig.bUseControllerDesiredRotation = false;
	PresentationConfig.bOrientRotationToMovement = true;
	PresentationConfig.MoveSpeedOverride = FMath::Max(RuntimeConfig.ReturnSpeed, 0.0f);
	PresentationConfig.RotationYawRate = DefaultCloneRotationRate;
	ApplyCombatMovePresentationContext(PresentationConfig);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = FMath::Max(RuntimeConfig.ReturnSpeed, 0.0f);
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
	}
	ForceNetUpdate();
}

void APRFaerinCloneCharacter::CompleteMerge()
{
	if (bCloneResolved)
	{
		return;
	}

	CloneState = EPRFaerinCloneState::Merged;
	bCloneResolved = true;
	StopCloneMovement();
	ClearCloneSpawnDissolveVisual();

	if (bCanApplyHealOnMerge && IsValid(OwnerFaerin))
	{
		OwnerFaerin->ApplyFaerinCloneMergeHeal(
			RuntimeConfig.MergeHealAmount,
			RuntimeConfig.MergeHealMaxHealthRatio,
			RuntimeConfig.MergeHealNiagaraSystem,
			RuntimeConfig.MergeHealNiagaraSocketName,
			RuntimeConfig.MergeHealNiagaraLocationOffset,
			RuntimeConfig.MergeHealNiagaraRotationOffset,
			RuntimeConfig.MergeHealNiagaraScale,
			RuntimeConfig.MergeHealNiagaraLifeSeconds);
	}

	Destroy();
}

void APRFaerinCloneCharacter::HandleCloneKilled()
{
	if (bCloneResolved)
	{
		return;
	}

	CloneState = EPRFaerinCloneState::Killed;
	bCloneResolved = true;
	UnbindSpokeComboEvents();
	EndNotifiedDamageWindow();
	StopCloneMovement();
	StopSpokeComboMontage();
	ClearCloneSpawnDissolveVisual();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->ForceClearAttackCommit();
	}

	Multicast_SpawnCloneWorldNiagara(
		RuntimeConfig.DestroyedNiagaraSystem,
		GetActorLocation(),
		GetActorRotation(),
		RuntimeConfig.WorldNiagaraScale,
		RuntimeConfig.WorldNiagaraLifeSeconds);
	Destroy();
}

void APRFaerinCloneCharacter::DestroyCloneWithoutHeal()
{
	if (bCloneResolved)
	{
		return;
	}

	bCloneResolved = true;
	UnbindSpokeComboEvents();
	EndNotifiedDamageWindow();
	StopCloneMovement();
	StopSpokeComboMontage();
	ClearCloneSpawnDissolveVisual();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->ForceClearAttackCommit();
	}
	Destroy();
}

// ===== 초기화 =====

void APRFaerinCloneCharacter::InitializeCloneAbilitySystem()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	const float BaseCloneMaxHealth = FMath::Max(RuntimeConfig.CloneMaxHealth, 1.0f);
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Common::GetMaxHealthAttribute(),
		BaseCloneMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Common::GetHealthAttribute(),
		BaseCloneMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute(),
		FMath::Max(RuntimeConfig.CloneMovementSpeedMultiplier, 0.0f));
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Common::GetArmorAttribute(),
		FMath::Max(RuntimeConfig.CloneArmor, 0.0f));
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Enemy::GetMaxGroggyGaugeAttribute(),
		FMath::Max(RuntimeConfig.CloneMaxGroggyGauge, 0.0f));
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Enemy::GetGroggyGaugeAttribute(),
		0.0f);
	AbilitySystemComponent->SetNumericAttributeBase(
		UPRAttributeSet_Enemy::GetAttackPowerAttribute(),
		FMath::Max(RuntimeConfig.CloneAttackPower, 0.0f));

	// 런타임 설정 기반 분신 체력 스케일 기준값
	BaseMaxHealth = BaseCloneMaxHealth;
	BindPlayerCountChanged();
	ApplyHealthScale();

	InitializeEnemyWorldHealthBar();
}

bool APRFaerinCloneCharacter::IsAssignedTargetAttackable() const
{
	if (!IsValid(AssignedTarget) || UPRCombatStatics::GetActorTeam(AssignedTarget) != EPRTeam::Player)
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AssignedTarget);
	return IsValid(TargetASC)
		&& !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

void APRFaerinCloneCharacter::MoveTowardAssignedTarget(float DeltaTime)
{
	if (!IsValid(AssignedTarget) || DeltaTime <= UE_SMALL_NUMBER)
	{
		return;
	}

	FVector Direction = AssignedTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		return;
	}

	const float MoveSpeed = FMath::Max(RuntimeConfig.ChaseMoveSpeed, 0.0f);
	const FVector OldLocation = GetActorLocation();
	FHitResult SweepHit;
	AddActorWorldOffset(Direction * MoveSpeed * DeltaTime, true, &SweepHit);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->Velocity = (GetActorLocation() - OldLocation) / DeltaTime;
	}

	FaceLocation(AssignedTarget->GetActorLocation(), DeltaTime);
}

void APRFaerinCloneCharacter::FaceLocation(const FVector& TargetLocation, float DeltaTime)
{
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		return;
	}

	const FRotator TargetRotation = Direction.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		FRotator(0.0f, TargetRotation.Yaw, 0.0f),
		DeltaTime,
		10.0f);
	SetActorRotation(NewRotation);
}

// ===== SpokeCombo 분기 =====

EPRFaerinCloneSpokeDirection APRFaerinCloneCharacter::ResolveFirstDirection() const
{
	EPRFaerinCloneSpokeDirection PreferredDirection = EPRFaerinCloneSpokeDirection::Right;
	switch (RuntimeConfig.FirstDirectionPolicy)
	{
	case EPRFaerinCloneFirstDirectionPolicy::Random:
		PreferredDirection = FMath::RandBool()
			? EPRFaerinCloneSpokeDirection::Right
			: EPRFaerinCloneSpokeDirection::Left;
		break;
	case EPRFaerinCloneFirstDirectionPolicy::ForceRight:
		PreferredDirection = EPRFaerinCloneSpokeDirection::Right;
		break;
	case EPRFaerinCloneFirstDirectionPolicy::ForceLeft:
		PreferredDirection = EPRFaerinCloneSpokeDirection::Left;
		break;
	case EPRFaerinCloneFirstDirectionPolicy::TargetSide:
	default:
		if (IsValid(AssignedTarget))
		{
			FVector DirectionToTarget = AssignedTarget->GetActorLocation() - GetActorLocation();
			DirectionToTarget.Z = 0.0f;
			if (DirectionToTarget.Normalize())
			{
				const float SideDot = FVector::DotProduct(GetActorRightVector(), DirectionToTarget);
				PreferredDirection = SideDot >= 0.0f
					? EPRFaerinCloneSpokeDirection::Right
					: EPRFaerinCloneSpokeDirection::Left;
			}
		}
		break;
	}

	if (IsValidComboSection(GetOpeningSectionName(PreferredDirection)))
	{
		return PreferredDirection;
	}

	const EPRFaerinCloneSpokeDirection OppositeDirection = GetOppositeDirection(PreferredDirection);
	if (IsValidComboSection(GetOpeningSectionName(OppositeDirection)))
	{
		return OppositeDirection;
	}

	return EPRFaerinCloneSpokeDirection::None;
}

EPRFaerinCloneSpokeDirection APRFaerinCloneCharacter::GetOppositeDirection(EPRFaerinCloneSpokeDirection Direction) const
{
	if (Direction == EPRFaerinCloneSpokeDirection::Right)
	{
		return EPRFaerinCloneSpokeDirection::Left;
	}

	if (Direction == EPRFaerinCloneSpokeDirection::Left)
	{
		return EPRFaerinCloneSpokeDirection::Right;
	}

	return EPRFaerinCloneSpokeDirection::None;
}

FName APRFaerinCloneCharacter::GetOpeningSectionName(EPRFaerinCloneSpokeDirection Direction) const
{
	if (Direction == EPRFaerinCloneSpokeDirection::Right)
	{
		return RuntimeConfig.RightOpeningSectionName;
	}

	if (Direction == EPRFaerinCloneSpokeDirection::Left)
	{
		return RuntimeConfig.LeftOpeningSectionName;
	}

	return NAME_None;
}

FName APRFaerinCloneCharacter::GetFollowUpSectionName(EPRFaerinCloneSpokeDirection Direction) const
{
	if (Direction == EPRFaerinCloneSpokeDirection::Right)
	{
		return RuntimeConfig.RightFollowUpSectionName;
	}

	if (Direction == EPRFaerinCloneSpokeDirection::Left)
	{
		return RuntimeConfig.LeftFollowUpSectionName;
	}

	return NAME_None;
}

bool APRFaerinCloneCharacter::IsValidComboSection(FName SectionName) const
{
	return IsValid(RuntimeConfig.SpokeComboMontage)
		&& SectionName != NAME_None
		&& RuntimeConfig.SpokeComboMontage->IsValidSectionName(SectionName);
}

void APRFaerinCloneCharacter::ConfigureComboSectionFlow() const
{
	SetComboNextSection(RuntimeConfig.RightOpeningSectionName, RuntimeConfig.RightOpeningSectionName);
	SetComboNextSection(RuntimeConfig.LeftOpeningSectionName, RuntimeConfig.LeftOpeningSectionName);
	SetComboNextSection(RuntimeConfig.RightFollowUpSectionName, RuntimeConfig.RightFollowUpSectionName);
	SetComboNextSection(RuntimeConfig.LeftFollowUpSectionName, RuntimeConfig.LeftFollowUpSectionName);
	SetComboNextSection(RuntimeConfig.SlamStartSectionName, RuntimeConfig.SlamStartSectionName);
	SetComboNextSection(RuntimeConfig.SlamLoopSectionName, RuntimeConfig.SlamLoopSectionName);
	SetComboNextSection(RuntimeConfig.SlamEndSectionName, NAME_None);
}

void APRFaerinCloneCharacter::SetComboNextSection(FName FromSectionName, FName ToSectionName) const
{
	if (!IsValid(RuntimeConfig.SpokeComboMontage)
		|| !RuntimeConfig.SpokeComboMontage->IsValidSectionName(FromSectionName)
		|| (ToSectionName != NAME_None && !RuntimeConfig.SpokeComboMontage->IsValidSectionName(ToSectionName)))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_SetNextSection(FromSectionName, ToSectionName, RuntimeConfig.SpokeComboMontage);
	}
}

bool APRFaerinCloneCharacter::JumpToComboSection(FName SectionName)
{
	if (!IsValidComboSection(SectionName))
	{
		return false;
	}

	RefreshAttackTargetMotionWarp(true);
	Multicast_JumpCloneSpokeMontageSection(SectionName);
	return true;
}

void APRFaerinCloneCharacter::AdvanceCurrentComboStage()
{
	if (CloneState != EPRFaerinCloneState::SpokeCombo)
	{
		return;
	}

	switch (ActiveSpokeStage)
	{
	case EPRFaerinCloneSpokeStage::Opening:
		EndNotifiedDamageWindow();
		if (ShouldBranchToSlam())
		{
			BeginSlamStart();
			return;
		}
		BeginOppositeFollowUp();
		break;
	case EPRFaerinCloneSpokeStage::FollowUp:
		EndNotifiedDamageWindow();
		CompleteSpokeCombo(true);
		break;
	case EPRFaerinCloneSpokeStage::SlamStart:
		BeginSlamLoop();
		break;
	case EPRFaerinCloneSpokeStage::SlamEnd:
		CompleteSpokeCombo(!bPendingSlamEndCancelled);
		break;
	case EPRFaerinCloneSpokeStage::SlamLoop:
	case EPRFaerinCloneSpokeStage::None:
	default:
		break;
	}
}

bool APRFaerinCloneCharacter::ShouldBranchToSlam() const
{
	const bool bCanUseSlam = IsValidComboSection(RuntimeConfig.SlamStartSectionName)
		&& IsValidComboSection(RuntimeConfig.SlamLoopSectionName)
		&& IsValidComboSection(RuntimeConfig.SlamEndSectionName);
	if (!bCanUseSlam)
	{
		return false;
	}

	if (RuntimeConfig.BranchPolicy == EPRFaerinCloneBranchPolicy::SlamOnly)
	{
		return true;
	}

	if (RuntimeConfig.BranchPolicy == EPRFaerinCloneBranchPolicy::OppositeOnly)
	{
		return false;
	}

	const EPRFaerinCloneSpokeDirection OppositeDirection = GetOppositeDirection(ActiveSpokeDirection);
	if (!IsValidComboSection(GetFollowUpSectionName(OppositeDirection)))
	{
		return true;
	}

	return FMath::FRand() <= FMath::Clamp(RuntimeConfig.SlamBranchChance, 0.0f, 1.0f);
}

void APRFaerinCloneCharacter::BeginOppositeFollowUp()
{
	const EPRFaerinCloneSpokeDirection OppositeDirection = GetOppositeDirection(ActiveSpokeDirection);
	const FName FollowUpSectionName = GetFollowUpSectionName(OppositeDirection);
	if (OppositeDirection == EPRFaerinCloneSpokeDirection::None || !JumpToComboSection(FollowUpSectionName))
	{
		CompleteSpokeCombo(false);
		return;
	}

	ActiveSpokeDirection = OppositeDirection;
	ActiveSpokeStage = EPRFaerinCloneSpokeStage::FollowUp;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
}

void APRFaerinCloneCharacter::BeginSlamStart()
{
	if (!JumpToComboSection(RuntimeConfig.SlamStartSectionName))
	{
		CompleteSpokeCombo(false);
		return;
	}

	ActiveSpokeStage = EPRFaerinCloneSpokeStage::SlamStart;
	ActiveSpokeDirection = EPRFaerinCloneSpokeDirection::None;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
	SlamLoopElapsedSeconds = 0.0f;
	SlamLoopMaxSeconds = ResolveSlamLoopMaxSeconds();
}

void APRFaerinCloneCharacter::BeginSlamLoop()
{
	if (!JumpToComboSection(RuntimeConfig.SlamLoopSectionName))
	{
		BeginSlamEnd(false);
		return;
	}

	ActiveSpokeStage = EPRFaerinCloneSpokeStage::SlamLoop;
	SlamLoopElapsedSeconds = 0.0f;
}

void APRFaerinCloneCharacter::BeginSlamEnd(bool bWasCancelled)
{
	if (ActiveSpokeStage == EPRFaerinCloneSpokeStage::SlamEnd)
	{
		return;
	}

	bPendingSlamEndCancelled = bWasCancelled;
	StopCloneMovement();
	SetComboNextSection(RuntimeConfig.SlamLoopSectionName, RuntimeConfig.SlamEndSectionName);
	SetComboNextSection(RuntimeConfig.SlamEndSectionName, NAME_None);
	if (!JumpToComboSection(RuntimeConfig.SlamEndSectionName))
	{
		CompleteSpokeCombo(false);
		return;
	}

	ActiveSpokeStage = EPRFaerinCloneSpokeStage::SlamEnd;
	EndNotifiedDamageWindow();
}

float APRFaerinCloneCharacter::ResolveSlamLoopMaxSeconds() const
{
	const float MinSeconds = FMath::Max(RuntimeConfig.SlamLoopMaxSecondsMin, 0.0f);
	const float MaxSeconds = FMath::Max(RuntimeConfig.SlamLoopMaxSecondsMax, MinSeconds);
	return MaxSeconds > 0.0f ? FMath::FRandRange(MinSeconds, MaxSeconds) : 0.0f;
}

float APRFaerinCloneCharacter::CalculateDistanceToTarget() const
{
	if (!IsValid(AssignedTarget))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist2D(GetActorLocation(), AssignedTarget->GetActorLocation());
}

bool APRFaerinCloneCharacter::MoveSlamTowardTarget(float DeltaTime)
{
	if (!IsValid(AssignedTarget))
	{
		return false;
	}

	FVector MoveDirection = AssignedTarget->GetActorLocation() - GetActorLocation();
	if (RuntimeConfig.bConstrainSlamMovementToGround)
	{
		MoveDirection.Z = 0.0f;
	}

	const float DistanceToTarget = MoveDirection.Size();
	if (!MoveDirection.Normalize())
	{
		return false;
	}

	const float MoveDistance = FMath::Min(
		RuntimeConfig.SlamTrackingMoveSpeed * DeltaTime,
		FMath::Max(DistanceToTarget - RuntimeConfig.SlamEndTriggerDistance, 0.0f));
	if (MoveDistance <= 0.0f)
	{
		return true;
	}

	FHitResult SweepHit;
	AddActorWorldOffset(MoveDirection * MoveDistance, true, &SweepHit);

	if (RuntimeConfig.bDrawDebug)
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + MoveDirection * 160.0f, FColor::Cyan, false, 0.1f, 0, 2.0f);
	}

	return !SweepHit.bBlockingHit;
}

bool APRFaerinCloneCharacter::RefreshAttackTargetMotionWarp(bool bUseTargetLocation) const
{
	return PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(GetMesh(), bUseTargetLocation);
}

// ===== SpokeCombo 노티파이 =====

void APRFaerinCloneCharacter::BindSpokeComboEvents()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	UnbindSpokeComboEvents();
	ComboWindowEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(
		PRCombatGameplayTags::Event_Ability_EnemyComboWindow).AddUObject(
			this,
			&APRFaerinCloneCharacter::HandleComboWindowGameplayEvent);
	MeleeHitWindowBeginEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin).AddUObject(
			this,
			&APRFaerinCloneCharacter::HandleMeleeHitWindowBeginGameplayEvent);
	MeleeHitWindowTickEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick).AddUObject(
			this,
			&APRFaerinCloneCharacter::HandleMeleeHitWindowTickGameplayEvent);
	MeleeHitWindowEndEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(
		PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd).AddUObject(
			this,
			&APRFaerinCloneCharacter::HandleMeleeHitWindowEndGameplayEvent);
}

void APRFaerinCloneCharacter::UnbindSpokeComboEvents()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (ComboWindowEventHandle.IsValid())
	{
		if (auto* Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(PRCombatGameplayTags::Event_Ability_EnemyComboWindow))
		{
			Delegate->Remove(ComboWindowEventHandle);
		}
		ComboWindowEventHandle.Reset();
	}

	if (MeleeHitWindowBeginEventHandle.IsValid())
	{
		if (auto* Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin))
		{
			Delegate->Remove(MeleeHitWindowBeginEventHandle);
		}
		MeleeHitWindowBeginEventHandle.Reset();
	}

	if (MeleeHitWindowTickEventHandle.IsValid())
	{
		if (auto* Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick))
		{
			Delegate->Remove(MeleeHitWindowTickEventHandle);
		}
		MeleeHitWindowTickEventHandle.Reset();
	}

	if (MeleeHitWindowEndEventHandle.IsValid())
	{
		if (auto* Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd))
		{
			Delegate->Remove(MeleeHitWindowEndEventHandle);
		}
		MeleeHitWindowEndEventHandle.Reset();
	}
}

void APRFaerinCloneCharacter::HandleComboWindowGameplayEvent(const FGameplayEventData* Payload)
{
	if (!HasAuthority()
		|| Payload == nullptr
		|| Payload->EventTag != PRCombatGameplayTags::Event_Ability_EnemyComboWindow
		|| ActiveSpokeStage == EPRFaerinCloneSpokeStage::SlamLoop)
	{
		return;
	}

	AdvanceCurrentComboStage();
}

void APRFaerinCloneCharacter::HandleMeleeHitWindowBeginGameplayEvent(const FGameplayEventData* Payload)
{
	if (!HasAuthority()
		|| Payload == nullptr
		|| Payload->EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin)
	{
		return;
	}

	BeginNotifiedDamageWindow();
}

void APRFaerinCloneCharacter::HandleMeleeHitWindowTickGameplayEvent(const FGameplayEventData* Payload)
{
	if (!HasAuthority()
		|| Payload == nullptr
		|| Payload->EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick)
	{
		return;
	}

	TickNotifiedDamageWindow();
}

void APRFaerinCloneCharacter::HandleMeleeHitWindowEndGameplayEvent(const FGameplayEventData* Payload)
{
	if (!HasAuthority()
		|| Payload == nullptr
		|| Payload->EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd)
	{
		return;
	}

	EndNotifiedDamageWindow();
}

// ===== 피해 판정 =====

void APRFaerinCloneCharacter::BeginNotifiedDamageWindow()
{
	if (CloneState != EPRFaerinCloneState::SpokeCombo)
	{
		return;
	}

	switch (ActiveSpokeStage)
	{
	case EPRFaerinCloneSpokeStage::Opening:
	case EPRFaerinCloneSpokeStage::FollowUp:
		ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::Spoke;
		break;
	case EPRFaerinCloneSpokeStage::SlamEnd:
		ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::SlamEnd;
		break;
	case EPRFaerinCloneSpokeStage::SlamStart:
	case EPRFaerinCloneSpokeStage::SlamLoop:
	case EPRFaerinCloneSpokeStage::None:
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

void APRFaerinCloneCharacter::TickNotifiedDamageWindow()
{
	float DamageMultiplier = 0.0f;
	float PoiseDamage = 0.0f;
	switch (ActiveDamageWindow)
	{
	case EPRFaerinCloneSpokeDamageWindow::Spoke:
		DamageMultiplier = RuntimeConfig.AttackDamageMultiplier;
		PoiseDamage = RuntimeConfig.AttackPoiseDamage;
		break;
	case EPRFaerinCloneSpokeDamageWindow::SlamEnd:
		DamageMultiplier = RuntimeConfig.SlamDamageMultiplier;
		PoiseDamage = RuntimeConfig.SlamPoiseDamage;
		break;
	case EPRFaerinCloneSpokeDamageWindow::None:
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

void APRFaerinCloneCharacter::EndNotifiedDamageWindow()
{
	ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::None;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}
}

bool APRFaerinCloneCharacter::GetCurrentBladeTracePoint(FVector& OutTracePoint) const
{
	if (IsValid(WeaponVisualComponent) && IsValid(WeaponVisualComponent->GetBladeHitBox()))
	{
		const FTransform HitBoxTransform = WeaponVisualComponent->GetBladeHitBox()->GetComponentTransform();
		OutTracePoint = HitBoxTransform.GetLocation() + HitBoxTransform.TransformVectorNoScale(RuntimeConfig.HitTraceOffset);
		return true;
	}

	const USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent) || RuntimeConfig.FallbackBladeBoneName == NAME_None)
	{
		return false;
	}

	const bool bHasSocket = MeshComponent->DoesSocketExist(RuntimeConfig.FallbackBladeBoneName);
	const bool bHasBone = MeshComponent->GetBoneIndex(RuntimeConfig.FallbackBladeBoneName) != INDEX_NONE;
	if (!bHasSocket && !bHasBone)
	{
		return false;
	}

	const FTransform BladeTransform = MeshComponent->GetSocketTransform(RuntimeConfig.FallbackBladeBoneName, RTS_World);
	OutTracePoint = BladeTransform.GetLocation() + BladeTransform.TransformVectorNoScale(RuntimeConfig.HitTraceOffset);
	return true;
}

void APRFaerinCloneCharacter::ApplyDamageTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const float InDamageMultiplier,
	const float InPoiseDamage)
{
	if (!HasAuthority())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaerinCloneSpokeCombo), false, this);
	QueryParams.AddIgnoredActor(this);
	if (IsValid(OwnerFaerin))
	{
		QueryParams.AddIgnoredActor(OwnerFaerin);
	}

	TArray<FHitResult> HitResults;
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FMath::Max(RuntimeConfig.HitTraceRadius, 0.0f));
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		RuntimeConfig.HitTraceChannel,
		CollisionShape,
		QueryParams);

	if (RuntimeConfig.bDrawDebug)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), TraceStart, RuntimeConfig.HitTraceRadius, 16, DebugColor, false, 1.0f);
		DrawDebugSphere(GetWorld(), TraceEnd, RuntimeConfig.HitTraceRadius, 16, DebugColor, false, 1.0f);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!ShouldDamageActor(HitActor) || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		ApplyCloneAttackPowerDamage(HitActor, InDamageMultiplier, InPoiseDamage, &HitResult);
		DamagedActors.Add(HitActor);
	}
}

bool APRFaerinCloneCharacter::ShouldDamageActor(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor)
		|| CandidateActor == this
		|| CandidateActor == OwnerFaerin.Get()
		|| UPRCombatStatics::GetActorTeam(CandidateActor) != EPRTeam::Player)
	{
		return false;
	}

	if (!IsValid(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateActor)))
	{
		return false;
	}

	return !RuntimeConfig.bOnlyDamageAssignedTarget || CandidateActor == AssignedTarget.Get();
}

void APRFaerinCloneCharacter::ApplyCloneAttackPowerDamage(
	AActor* TargetActor,
	const float DamageMultiplier,
	const float PoiseDamage,
	const FHitResult* HitResult)
{
	if (!IsValid(TargetActor) || UPRCombatStatics::GetActorTeam(TargetActor) == EPRTeam::Enemy || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromEnemy))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		EffectContext.AddHitResult(*HitResult, true);
	}

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		Registry->DamageGE_FromEnemy,
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_AttackMultiplier,
		FMath::Max(DamageMultiplier, 0.0f));
	SpecHandle.Data->SetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_PoiseDamage,
		FMath::Max(PoiseDamage, 0.0f));

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

// ===== 몽타주/VFX =====

void APRFaerinCloneCharacter::StopSpokeComboMontage() const
{
	if (!IsValid(RuntimeConfig.SpokeComboMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (IsValid(AnimInstance) && AnimInstance->Montage_IsPlaying(RuntimeConfig.SpokeComboMontage))
	{
		AnimInstance->Montage_Stop(FMath::Max(RuntimeConfig.MontageStopBlendOutTime, 0.0f), RuntimeConfig.SpokeComboMontage);
	}
}

void APRFaerinCloneCharacter::StopCloneMovement()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	ClearCombatMovePresentationContext();
}

void APRFaerinCloneCharacter::Multicast_SpawnCloneWorldNiagara_Implementation(
	UNiagaraSystem* NiagaraSystem,
	FVector_NetQuantize Location,
	FRotator Rotation,
	FVector Scale,
	const float LifeSeconds)
{
	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		true,
		true,
		ENCPoolMethod::None,
		false);
	if (!IsValid(NiagaraComponent) || LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [WeakNiagaraComponent]()
		{
			if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}
		}),
		LifeSeconds,
		false);
}

void APRFaerinCloneCharacter::Multicast_PlayCloneSpokeMontage_Implementation(
	UAnimMontage* Montage,
	const float PlayRate,
	FName StartSectionName)
{
	if (!IsValid(Montage) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	RuntimeConfig.SpokeComboMontage = Montage;
	RuntimeConfig.MontagePlayRate = FMath::Max(PlayRate, UE_SMALL_NUMBER);
	AnimInstance->Montage_Play(Montage, FMath::Max(PlayRate, UE_SMALL_NUMBER));
	ConfigureComboSectionFlow();
	if (Montage->IsValidSectionName(StartSectionName))
	{
		AnimInstance->Montage_JumpToSection(StartSectionName, Montage);
	}
}

void APRFaerinCloneCharacter::Multicast_JumpCloneSpokeMontageSection_Implementation(FName SectionName)
{
	if (!IsValid(RuntimeConfig.SpokeComboMontage) || !IsValid(GetMesh()) || !RuntimeConfig.SpokeComboMontage->IsValidSectionName(SectionName))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_JumpToSection(SectionName, RuntimeConfig.SpokeComboMontage);
	}
}

void APRFaerinCloneCharacter::Multicast_PlayCloneSpawnDissolveVisual_Implementation(
	UNiagaraSystem* DissolveNiagaraSystem,
	const float DissolveDuration,
	const float DissolveStartValue,
	const float DissolveEndValue,
	FName DissolveScalarParameterName,
	FName NiagaraDissolveParameterName,
	UTexture* DissolveTexture,
	FVector2D DissolveTextureUV,
	const float DissolveTickInterval)
{
	StartCloneSpawnDissolveVisual(
		DissolveNiagaraSystem,
		DissolveDuration,
		DissolveStartValue,
		DissolveEndValue,
		DissolveScalarParameterName,
		NiagaraDissolveParameterName,
		DissolveTexture,
		DissolveTextureUV,
		DissolveTickInterval);
}

void APRFaerinCloneCharacter::StartCloneSpawnDissolveVisual(
	UNiagaraSystem* DissolveNiagaraSystem,
	const float DissolveDuration,
	const float DissolveStartValue,
	const float DissolveEndValue,
	FName DissolveScalarParameterName,
	FName NiagaraDissolveParameterName,
	UTexture* DissolveTexture,
	const FVector2D& DissolveTextureUV,
	const float DissolveTickInterval)
{
	ClearCloneSpawnDissolveVisual();

	CloneDissolveDuration = FMath::Max(DissolveDuration, 0.0f);
	CloneDissolveStartValue = DissolveStartValue;
	CloneDissolveEndValue = DissolveEndValue;
	CloneDissolveTickInterval = FMath::Max(DissolveTickInterval, 0.001f);
	CloneDissolveElapsedTime = 0.0f;
	CloneDissolveScalarParameterName = DissolveScalarParameterName != NAME_None
		? DissolveScalarParameterName
		: TEXT("DissolveAmount");
	CloneNiagaraDissolveParameterName = NiagaraDissolveParameterName != NAME_None
		? NiagaraDissolveParameterName
		: TEXT("User.DissolveAmount");

	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* CurrentMaterial = MeshComponent->GetMaterial(MaterialIndex);
			if (!IsValid(CurrentMaterial))
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
			if (!IsValid(DynamicMaterial))
			{
				DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
			}

			if (IsValid(DynamicMaterial))
			{
				DynamicMaterial->SetScalarParameterValue(CloneDissolveScalarParameterName, CloneDissolveStartValue);
				CloneDissolveDynamicMaterials.Add(DynamicMaterial);
			}
		}
	}

	if (IsValid(DissolveNiagaraSystem) && IsValid(GetMesh()))
	{
		ActiveCloneDissolveNiagara = UNiagaraFunctionLibrary::SpawnSystemAttached(
			DissolveNiagaraSystem,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true,
			true,
			ENCPoolMethod::None,
			false);

		if (IsValid(ActiveCloneDissolveNiagara))
		{
			ActiveCloneDissolveNiagara->SetVariableFloat(CloneNiagaraDissolveParameterName, CloneDissolveStartValue);
			if (IsValid(DissolveTexture))
			{
				ActiveCloneDissolveNiagara->SetVariableTexture(TEXT("User.DissolveTexture"), DissolveTexture);
			}
			ActiveCloneDissolveNiagara->SetVariableVec2(TEXT("User.DissolveTextureUV"), DissolveTextureUV);
			ActiveCloneDissolveNiagara->Activate(true);
		}
	}

	ApplyCloneSpawnDissolveVisualValue(CloneDissolveStartValue);
	if (CloneDissolveDuration <= UE_SMALL_NUMBER)
	{
		ApplyCloneSpawnDissolveVisualValue(CloneDissolveEndValue);
		CompleteCloneSpawnDissolveVisual();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CloneDissolveTickTimerHandle,
			this,
			&APRFaerinCloneCharacter::TickCloneSpawnDissolveVisual,
			CloneDissolveTickInterval,
			true);
	}
}

void APRFaerinCloneCharacter::TickCloneSpawnDissolveVisual()
{
	CloneDissolveElapsedTime += CloneDissolveTickInterval;
	const float Duration = FMath::Max(CloneDissolveDuration, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(CloneDissolveElapsedTime / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(CloneDissolveStartValue, CloneDissolveEndValue, Alpha);
	ApplyCloneSpawnDissolveVisualValue(DissolveValue);

	if (Alpha >= 1.0f)
	{
		CompleteCloneSpawnDissolveVisual();
	}
}

void APRFaerinCloneCharacter::CompleteCloneSpawnDissolveVisual()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CloneDissolveTickTimerHandle);
	}

	ApplyCloneSpawnDissolveVisualValue(CloneDissolveEndValue);
	if (IsValid(ActiveCloneDissolveNiagara))
	{
		ActiveCloneDissolveNiagara->Deactivate();
		ActiveCloneDissolveNiagara = nullptr;
	}
}

void APRFaerinCloneCharacter::ApplyCloneSpawnDissolveVisualValue(float DissolveValue)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : CloneDissolveDynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(CloneDissolveScalarParameterName, DissolveValue);
		}
	}

	if (IsValid(ActiveCloneDissolveNiagara))
	{
		ActiveCloneDissolveNiagara->SetVariableFloat(CloneNiagaraDissolveParameterName, DissolveValue);
	}
}

void APRFaerinCloneCharacter::ClearCloneSpawnDissolveVisual()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CloneDissolveTickTimerHandle);
	}

	if (IsValid(ActiveCloneDissolveNiagara))
	{
		ActiveCloneDissolveNiagara->Deactivate();
		ActiveCloneDissolveNiagara = nullptr;
	}

	CloneDissolveDynamicMaterials.Reset();
}
