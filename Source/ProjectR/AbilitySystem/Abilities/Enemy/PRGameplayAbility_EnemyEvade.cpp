// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Evade 상태/패턴 어빌리티 구현)
#include "PRGameplayAbility_EnemyEvade.h"

#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyEvade::UPRGameplayAbility_EnemyEvade()
{
	FGameplayTagContainer EvadeAbilityTags;
	EvadeAbilityTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	EvadeAbilityTags.AddTag(PRGameplayTags::Ability_Enemy_Evade);
	SetAssetTags(EvadeAbilityTags);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Enemy_Evade);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);

	CooldownTagsContainer.AddTag(PRGameplayTags::Cooldown_Enemy_Evade);
}

bool UPRGameplayAbility_EnemyEvade::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return CanSpendAttackPressureCost(ActorInfo);
}

void UPRGameplayAbility_EnemyEvade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CanSpendAttackPressureCost(ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* EvadeMontage = SelectEvadeMontage();
	if (!IsValid(EvadeMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		EvadeMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TryConsumeAttackPressureCost())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bEvadeFinished = false;

	if (bStopMovementOnActivate)
	{
		if (UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	if (bStopAIPathOnActivate)
	{
		if (AAIController* AIController = Cast<AAIController>(SourceCharacter->GetController()))
		{
			AIController->StopMovement();
		}
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyEvade::HandleEvadeMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyEvade::HandleEvadeMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyEvade::HandleEvadeMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyEvade::HandleEvadeMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_EnemyEvade::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UPRGameplayAbility_EnemyEvade::GetCooldownTags() const
{
	return &CooldownTagsContainer;
}

void UPRGameplayAbility_EnemyEvade::HandleEvadeMontageCompleted()
{
	FinishEvade(false);
}

void UPRGameplayAbility_EnemyEvade::HandleEvadeMontageInterrupted()
{
	FinishEvade(true);
}

UAnimMontage* UPRGameplayAbility_EnemyEvade::SelectEvadeMontage() const
{
	switch (DirectionSelectionMode)
	{
	case EPREnemyEvadeDirectionSelectionMode::Left:
		return IsValid(LeftEvadeMontage) ? LeftEvadeMontage.Get() : RightEvadeMontage.Get();
	case EPREnemyEvadeDirectionSelectionMode::Right:
		return IsValid(RightEvadeMontage) ? RightEvadeMontage.Get() : LeftEvadeMontage.Get();
	case EPREnemyEvadeDirectionSelectionMode::AwayFromTarget:
		return SelectMontageAwayFromTarget();
	case EPREnemyEvadeDirectionSelectionMode::Random:
	default:
		return SelectRandomMontage();
	}
}

UAnimMontage* UPRGameplayAbility_EnemyEvade::SelectMontageAwayFromTarget() const
{
	const ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const AActor* TargetActor = ResolveCurrentTargetActor();
	if (!IsValid(SourceCharacter) || !IsValid(TargetActor))
	{
		return SelectRandomMontage();
	}

	const FVector ToTarget = (TargetActor->GetActorLocation() - SourceCharacter->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return SelectRandomMontage();
	}

	const float TargetRightDot = FVector::DotProduct(ToTarget, SourceCharacter->GetActorRightVector().GetSafeNormal2D());
	const bool bTargetOnRightSide = TargetRightDot >= 0.0f;
	UAnimMontage* PreferredMontage = bTargetOnRightSide ? LeftEvadeMontage.Get() : RightEvadeMontage.Get();
	if (IsValid(PreferredMontage))
	{
		return PreferredMontage;
	}

	return bTargetOnRightSide ? RightEvadeMontage.Get() : LeftEvadeMontage.Get();
}

UAnimMontage* UPRGameplayAbility_EnemyEvade::SelectRandomMontage() const
{
	const bool bHasLeftMontage = IsValid(LeftEvadeMontage);
	const bool bHasRightMontage = IsValid(RightEvadeMontage);

	if (bHasLeftMontage && bHasRightMontage)
	{
		return FMath::RandBool() ? LeftEvadeMontage.Get() : RightEvadeMontage.Get();
	}

	return bHasLeftMontage ? LeftEvadeMontage.Get() : RightEvadeMontage.Get();
}

AActor* UPRGameplayAbility_EnemyEvade::ResolveCurrentTargetActor() const
{
	const APawn* SourcePawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourcePawn))
	{
		return nullptr;
	}

	const AAIController* AIController = Cast<AAIController>(SourcePawn->GetController());
	if (!IsValid(AIController))
	{
		return nullptr;
	}

	const UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || CurrentTargetKey == NAME_None)
	{
		return nullptr;
	}

	return Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
}

UBlackboardComponent* UPRGameplayAbility_EnemyEvade::ResolveBlackboardComponent(const FGameplayAbilityActorInfo* ActorInfo) const
{
	APawn* SourcePawn = nullptr;
	if (ActorInfo != nullptr)
	{
		SourcePawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	}

	if (!IsValid(SourcePawn))
	{
		SourcePawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	}

	if (!IsValid(SourcePawn))
	{
		return nullptr;
	}

	AAIController* AIController = Cast<AAIController>(SourcePawn->GetController());
	if (!IsValid(AIController))
	{
		return nullptr;
	}

	return AIController->GetBlackboardComponent();
}

bool UPRGameplayAbility_EnemyEvade::CanSpendAttackPressureCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!bConsumeAttackPressureOnActivate || AttackPressureCost <= 0.0f)
	{
		return true;
	}

	const UBlackboardComponent* BlackboardComponent = ResolveBlackboardComponent(ActorInfo);
	const bool bHasPressureKey = IsValid(BlackboardComponent)
		&& AttackPressureKey != NAME_None
		&& BlackboardComponent->GetKeyID(AttackPressureKey) != FBlackboard::InvalidKey;
	if (!bHasPressureKey)
	{
		return !bFailIfAttackPressureUnavailable;
	}

	return BlackboardComponent->GetValueAsFloat(AttackPressureKey) + UE_KINDA_SMALL_NUMBER >= AttackPressureCost;
}

bool UPRGameplayAbility_EnemyEvade::TryConsumeAttackPressureCost() const
{
	if (!bConsumeAttackPressureOnActivate || AttackPressureCost <= 0.0f)
	{
		return true;
	}

	UBlackboardComponent* BlackboardComponent = ResolveBlackboardComponent();
	const bool bHasPressureKey = IsValid(BlackboardComponent)
		&& AttackPressureKey != NAME_None
		&& BlackboardComponent->GetKeyID(AttackPressureKey) != FBlackboard::InvalidKey;
	if (!bHasPressureKey)
	{
		return !bFailIfAttackPressureUnavailable;
	}

	const float PreviousPressure = BlackboardComponent->GetValueAsFloat(AttackPressureKey);
	if (PreviousPressure + UE_KINDA_SMALL_NUMBER < AttackPressureCost)
	{
		return false;
	}

	const float NextPressure = FMath::Max(PreviousPressure - AttackPressureCost, 0.0f);
	BlackboardComponent->SetValueAsFloat(AttackPressureKey, NextPressure);

	if (PREnemyAIDebug::IsAttackPressureLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[AttackPressure] Consume Reason=Evade Prev=%.2f Next=%.2f Cost=%.2f Pawn=%s"),
			PreviousPressure,
			NextPressure,
			AttackPressureCost,
			*GetNameSafe(GetAvatarActorFromActorInfo()));
	}

	return true;
}

void UPRGameplayAbility_EnemyEvade::FinishEvade(bool bWasCancelled)
{
	if (bEvadeFinished)
	{
		return;
	}

	bEvadeFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
