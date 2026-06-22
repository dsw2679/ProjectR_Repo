// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Throw 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_FaerinThrowSequence.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinWeaponVisualComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinThrowSequence::UPRGameplayAbility_FaerinThrowSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_ThrowSequence));
}

void UPRGameplayAbility_FaerinThrowSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	DamagedActors.Reset();
	PreviousBladeTracePoint = FVector::ZeroVector;
	bThrowHitWindowActive = false;
	bHasPreviousBladeTracePoint = false;
	WeaponVisualComponent = nullptr;

	BindMeleeHitWindowEvents();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinThrowSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	EndThrowHitWindow();
	EndMeleeHitWindowEvents();

	DamagedActors.Reset();
	WeaponVisualComponent = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_FaerinThrowSequence::NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage)
{
	if (!ThrowStageName.IsNone() && Stage.StageName != ThrowStageName)
	{
		return;
	}

	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	WeaponVisualComponent = IsValid(BossCharacter)
		? BossCharacter->FindComponentByClass<UPRFaerinWeaponVisualComponent>()
		: nullptr;
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->ResolveConfiguredComponents();
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}
}

// ===== Notify 기반 검 판정 =====

void UPRGameplayAbility_FaerinThrowSequence::BindMeleeHitWindowEvents()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	EndMeleeHitWindowEvents();

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
			&UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowBeginGameplayEvent);
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
			&UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowTickGameplayEvent);
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
			&UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowEndGameplayEvent);
		ActiveMeleeHitWindowEndEventTask->ReadyForActivation();
	}
}

void UPRGameplayAbility_FaerinThrowSequence::EndMeleeHitWindowEvents()
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

void UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin)
	{
		return;
	}

	BeginThrowHitWindow();
}

void UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick)
	{
		return;
	}

	TickThrowHitWindow();
}

void UPRGameplayAbility_FaerinThrowSequence::HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd)
	{
		return;
	}

	EndThrowHitWindow();
}

void UPRGameplayAbility_FaerinThrowSequence::BeginThrowHitWindow()
{
	if (bThrowHitWindowActive)
	{
		return;
	}

	bThrowHitWindowActive = true;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(true);
	}

	TickThrowHitWindow();
}

void UPRGameplayAbility_FaerinThrowSequence::TickThrowHitWindow()
{
	if (!bThrowHitWindowActive)
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
		ApplyThrowDamageTrace(CurrentTracePoint, CurrentTracePoint);
		PreviousBladeTracePoint = CurrentTracePoint;
		bHasPreviousBladeTracePoint = true;
		return;
	}

	ApplyThrowDamageTrace(PreviousBladeTracePoint, CurrentTracePoint);
	PreviousBladeTracePoint = CurrentTracePoint;
}

void UPRGameplayAbility_FaerinThrowSequence::EndThrowHitWindow()
{
	bThrowHitWindowActive = false;
	bHasPreviousBladeTracePoint = false;
	PreviousBladeTracePoint = FVector::ZeroVector;

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBladeHitBoxEnabled(false);
	}
}

// ===== 검 sweep 피해 =====

bool UPRGameplayAbility_FaerinThrowSequence::GetCurrentBladeTracePoint(FVector& OutTracePoint) const
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

void UPRGameplayAbility_FaerinThrowSequence::ApplyThrowDamageTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaerinThrowSequence), false, AvatarActor);
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

bool UPRGameplayAbility_FaerinThrowSequence::ShouldDamageActor(AActor* CandidateActor) const
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
