// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 손승우 (로열 아처 공중 투사체 사격 제어 구현)
// Author: 이건주 (Penitent 원거리 배리어 사격 패턴 구현)
#include "PRGameplayAbility_EnemyProjectileAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPREnemyProjectileAttack, Log, All);

UPRGameplayAbility_EnemyProjectileAttack::UPRGameplayAbility_EnemyProjectileAttack()
{
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
}

bool UPRGameplayAbility_EnemyProjectileAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!CooldownBlockedTag.IsValid() || ActorInfo == nullptr)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(CooldownBlockedTag))
	{
		// 재사용 차단 태그 검사
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cooldown);
		}
		return false;
	}

	return true;
}

void UPRGameplayAbility_EnemyProjectileAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !IsValid(GetCurrentThreatTarget()) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		// 공격 실행 상태 태그
		AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_Enemy_Attacking);
	}

	bProjectileFired = false;
	bProjectileAttackFinished = false;
	bWaitingProjectileFireNotify = false;
	bCooldownEligibleAfterMontageStart = false;
	SelectedAttackMontage = SelectAttackMontage();

	if (ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor))
	{
		// 캐스팅 시작 시 BT 이동 잔여 속도를 제거한다.
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

	const bool bHasAttackMontage = IsValid(SelectedAttackMontage);
	if (bHasAttackMontage)
	{
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			SelectedAttackMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
			MontageStartSection);

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
			bCooldownEligibleAfterMontageStart = true;
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}
	else
	{
		bCooldownEligibleAfterMontageStart = true;
	}

	if (bHasAttackMontage && bUseAnimationNotifyForFire)
	{
		constexpr bool bOnlyTriggerOnce = true;
		constexpr bool bOnlyMatchExact = true;

		ActiveProjectileFireEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyProjectileFire,
			nullptr,
			bOnlyTriggerOnce,
			bOnlyMatchExact);

		if (IsValid(ActiveProjectileFireEventTask))
		{
			bWaitingProjectileFireNotify = true;
			ActiveProjectileFireEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyProjectileAttack::HandleProjectileFireGameplayEvent);
			ActiveProjectileFireEventTask->ReadyForActivation();
		}
	}

	const bool bUseTimedFire = !bHasAttackMontage || !bUseAnimationNotifyForFire;
	if (bUseTimedFire && WindupTime <= 0.0f)
	{
		FireProjectile();
	}
	else if (bUseTimedFire)
	{
		World->GetTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&UPRGameplayAbility_EnemyProjectileAttack::FireProjectile,
			WindupTime,
			false);
	}

	const float TimerFinishDelay = FMath::Max(WindupTime + RecoveryTime, 0.01f);
	const float MontageFinishDelay = bHasAttackMontage && bUseMontageDurationForFinish
		? (SelectedAttackMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)) + 0.1f
		: 0.0f;
	const float FinishDelay = FMath::Max(TimerFinishDelay, MontageFinishDelay);
	World->GetTimerManager().SetTimer(
		FinishTimerHandle,
		this,
		&UPRGameplayAbility_EnemyProjectileAttack::FinishProjectileAttack,
		FinishDelay,
		false);
}

void UPRGameplayAbility_EnemyProjectileAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(FinishTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveProjectileFireEventTask))
	{
		ActiveProjectileFireEventTask->EndTask();
		ActiveProjectileFireEventTask = nullptr;
	}

	if (bCooldownEligibleAfterMontageStart)
	{
		ApplyConfiguredCooldown();
	}

	EndThreatAttackCommit();
	bProjectileFired = false;
	bProjectileAttackFinished = false;
	bWaitingProjectileFireNotify = false;
	bCooldownEligibleAfterMontageStart = false;
	SelectedAttackMontage = nullptr;

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		// 공격 실행 상태 태그 정리
		AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Attacking);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyProjectileAttack::TriggerProjectileFireFromAnimation()
{
	FireProjectile();
}

void UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageCompleted()
{
	if (bUseMontageDurationForFinish)
	{
		FinishProjectileAttack();
	}
}

void UPRGameplayAbility_EnemyProjectileAttack::HandleAttackMontageInterrupted()
{
	if (bProjectileAttackFinished)
	{
		return;
	}

	bProjectileAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPRGameplayAbility_EnemyProjectileAttack::HandleProjectileFireGameplayEvent(FGameplayEventData Payload)
{
	if (Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyProjectileFire)
	{
		return;
	}

	bWaitingProjectileFireNotify = false;
	FireProjectile();
}

UAnimMontage* UPRGameplayAbility_EnemyProjectileAttack::SelectAttackMontage() const
{
	TArray<UAnimMontage*> ValidAttackMontages;
	for (const TObjectPtr<UAnimMontage>& AttackMontageCandidate : AttackMontages)
	{
		if (IsValid(AttackMontageCandidate.Get()))
		{
			ValidAttackMontages.Add(AttackMontageCandidate.Get());
		}
	}

	if (!ValidAttackMontages.IsEmpty())
	{
		const int32 SelectedIndex = FMath::RandHelper(ValidAttackMontages.Num());
		return ValidAttackMontages[SelectedIndex];
	}

	return nullptr;
}

void UPRGameplayAbility_EnemyProjectileAttack::FireProjectile()
{
	if (bProjectileAttackFinished || bProjectileFired)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| !IsValid(World)
		|| !IsValid(ProjectileClass))
	{
		return;
	}

	const int32 ProjectileFireCount = FMath::Max(GetProjectileFireCount(), 1);
	for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileFireCount; ++ProjectileIndex)
	{
		const FTransform SpawnTransform = GetProjectileSpawnTransformForIndex(ProjectileIndex);
		const uint32 ProjectileId = GenerateProjectileId();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = AvatarActor;
		SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.CustomPreSpawnInitalization = [ProjectileId](AActor* Actor)
		{
			if (APRProjectileBase* Projectile = Cast<APRProjectileBase>(Actor))
			{
				Projectile->InitializeProjectile(EPRProjectileRole::Auth, ProjectileId);
			}
		};

		APRProjectileBase* SpawnedProjectile = World->SpawnActor<APRProjectileBase>(
			ProjectileClass,
			SpawnTransform,
			SpawnParameters);

		if (!IsValid(SpawnedProjectile))
		{
			continue;
		}

		SpawnedProjectile->SetProjectileInitialVelocity(
			CalculateProjectileAimDirectionForIndex(SpawnTransform.GetLocation(), ProjectileIndex),
			ProjectileSpeedOverride);

		ConfigureSpawnedProjectile(SpawnedProjectile);
		ConfigureProjectileHomingSchedule(SpawnedProjectile);

		const FGameplayEffectSpecHandle EffectSpecHandle = BuildProjectileEffectSpec();
		if (EffectSpecHandle.IsValid())
		{
			SpawnedProjectile->InitGameplayEffectSpec(EffectSpecHandle);
		}

		SpawnedProjectile->PushRepMovement(EPRRepMovementEvent::Spawn);
	}

	bProjectileFired = true;
}

void UPRGameplayAbility_EnemyProjectileAttack::ConfigureSpawnedProjectile(APRProjectileBase* SpawnedProjectile) const
{
	if (!IsValid(SpawnedProjectile) || !bIgnoreEnemyActorsForProjectile)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<APREnemyBaseCharacter> EnemyIt(World); EnemyIt; ++EnemyIt)
	{
		SpawnedProjectile->AddProjectileIgnoredActor(*EnemyIt);
	}
}

void UPRGameplayAbility_EnemyProjectileAttack::ConfigureProjectileHomingSchedule(APRProjectileBase* SpawnedProjectile) const
{
	if (!bUseInitialProjectileHoming || !IsValid(SpawnedProjectile))
	{
		return;
	}

	AActor* HomingTargetActor = GetCurrentThreatTarget();
	if (!IsValid(HomingTargetActor) || !IsValid(HomingTargetActor->GetRootComponent()))
	{
		return;
	}

	const float ResolvedHomingAcceleration = FMath::Max(InitialProjectileHomingAcceleration, 0.0f);
	if (ResolvedHomingAcceleration <= 0.0f)
	{
		return;
	}

	SpawnedProjectile->ConfigureProjectileHomingSchedule(
		HomingTargetActor,
		ResolvedHomingAcceleration,
		InitialProjectileHomingStartDelay,
		InitialProjectileHomingDuration);
}

int32 UPRGameplayAbility_EnemyProjectileAttack::GetProjectileFireCount() const
{
	return 1;
}

FTransform UPRGameplayAbility_EnemyProjectileAttack::GetProjectileSpawnTransformForIndex(int32 /*ProjectileIndex*/) const
{
	return GetProjectileSpawnTransform();
}

FVector UPRGameplayAbility_EnemyProjectileAttack::CalculateProjectileAimDirectionForIndex(const FVector& SpawnLocation, int32 /*ProjectileIndex*/) const
{
	return CalculateProjectileAimDirection(SpawnLocation);
}

FGameplayEffectSpecHandle UPRGameplayAbility_EnemyProjectileAttack::BuildProjectileEffectSpec() const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return FGameplayEffectSpecHandle();
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = ProjectileDamageEffectClass;
	if (!IsValid(ResolvedDamageEffectClass))
	{
		const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
		if (IsValid(Registry))
		{
			ResolvedDamageEffectClass = Registry->DamageGE_FromEnemy;
		}
	}

	if (!IsValid(ResolvedDamageEffectClass))
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		ResolvedDamageEffectClass,
		GetAbilityLevel(),
		EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			FMath::Max(DamageMultiplier, 0.0f));

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			FMath::Max(PoiseDamage, 0.0f));
	}

	return SpecHandle;
}

FTransform UPRGameplayAbility_EnemyProjectileAttack::GetProjectileSpawnTransform() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor);
	const USkeletalMeshComponent* MeshComponent = IsValid(SourceCharacter)
		? SourceCharacter->GetMesh()
		: nullptr;

	if (IsValid(MeshComponent) && ProjectileSpawnSocketName != NAME_None && MeshComponent->DoesSocketExist(ProjectileSpawnSocketName))
	{
		return MeshComponent->GetSocketTransform(ProjectileSpawnSocketName);
	}

	const FTransform ActorTransform = IsValid(AvatarActor)
		? AvatarActor->GetActorTransform()
		: FTransform::Identity;
	const FVector SpawnLocation = ActorTransform.TransformPositionNoScale(ProjectileSpawnOffset);
	return FTransform(ActorTransform.GetRotation(), SpawnLocation);
}

FVector UPRGameplayAbility_EnemyProjectileAttack::CalculateProjectileAimDirection(const FVector& SpawnLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = GetCurrentThreatTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return IsValid(AvatarActor) ? AvatarActor->GetActorForwardVector() : FVector::ForwardVector;
	}

	FVector AimLocation = TargetActor->GetActorLocation();
	const float ResolvedSpeed = ResolveProjectileSpeedForAiming();
	const float Distance = FVector::Distance(SpawnLocation, AimLocation);
	if (ResolvedSpeed > 0.0f && ProjectileTargetLead > 0.0f)
	{
		const float TravelTime = Distance / ResolvedSpeed;
		const float LeadScale = ProjectileTargetLead * 0.01f;
		AimLocation += TargetActor->GetVelocity() * TravelTime * LeadScale;
	}

	const FVector AimDirection = (AimLocation - SpawnLocation).GetSafeNormal();
	return AimDirection.IsNearlyZero() ? AvatarActor->GetActorForwardVector() : AimDirection;
}

float UPRGameplayAbility_EnemyProjectileAttack::ResolveProjectileSpeedForAiming() const
{
	if (ProjectileSpeedOverride > 0.0f)
	{
		return ProjectileSpeedOverride;
	}

	const APRProjectileBase* ProjectileCDO = IsValid(ProjectileClass)
		? ProjectileClass->GetDefaultObject<APRProjectileBase>()
		: nullptr;

	// 투사체 BP 속도 기반 조준 보정
	if (IsValid(ProjectileCDO))
	{
		const float ProjectileInitialSpeed = ProjectileCDO->GetProjectileInitialSpeed();
		if (ProjectileInitialSpeed > 0.0f)
		{
			return ProjectileInitialSpeed;
		}
	}

	return 3500.0f;
}

AActor* UPRGameplayAbility_EnemyProjectileAttack::GetCurrentThreatTarget() const
{
	const APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	const UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetAttackTarget()
		: nullptr;
}

void UPRGameplayAbility_EnemyProjectileAttack::BeginThreatAttackCommit()
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

void UPRGameplayAbility_EnemyProjectileAttack::EndThreatAttackCommit()
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

void UPRGameplayAbility_EnemyProjectileAttack::RefreshAttackFacing(bool bApplyActorRotation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = GetCurrentThreatTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return;
	}

	FVector DirectionToTarget = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}
}

void UPRGameplayAbility_EnemyProjectileAttack::FinishProjectileAttack()
{
	if (bProjectileAttackFinished)
	{
		return;
	}

	if (!bProjectileFired)
	{
		if (bWaitingProjectileFireNotify)
		{
			UE_LOG(LogPREnemyProjectileAttack, Warning,
				TEXT("투사체 발사 노티파이를 받지 못했습니다. Ability=%s, Avatar=%s, Montage=%s"),
				*GetNameSafe(this),
				*GetNameSafe(GetAvatarActorFromActorInfo()),
				*GetNameSafe(SelectedAttackMontage.Get()));
		}
		else
		{
			FireProjectile();
		}
	}

	bProjectileAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_EnemyProjectileAttack::ApplyConfiguredCooldown()
{
	if (!IsValid(CooldownEffectClass))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(ASC) || !IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(AvatarActor);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		CooldownEffectClass,
		GetAbilityLevel(),
		EffectContext);

	// 공격 시작 후 쿨다운 GE 적용
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

uint32 UPRGameplayAbility_EnemyProjectileAttack::GenerateProjectileId()
{
	const uint32 ProjectileId = NextProjectileId++;
	if (NextProjectileId == 0)
	{
		NextProjectileId = 1;
	}

	return ProjectileId;
}
