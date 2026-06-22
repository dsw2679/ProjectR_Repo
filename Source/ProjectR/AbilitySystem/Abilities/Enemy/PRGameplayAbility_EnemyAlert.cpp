// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Alert 상태/패턴 어빌리티 구현)
#include "PRGameplayAbility_EnemyAlert.h"

#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyAlert::UPRGameplayAbility_EnemyAlert()
{
	FGameplayTagContainer AlertAbilityTags;
	AlertAbilityTags.AddTag(PRGameplayTags::Ability_Enemy_Alert);
	SetAssetTags(AlertAbilityTags);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
}

void UPRGameplayAbility_EnemyAlert::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bAlertFinished = false;

	if (bStopMovementOnAlert)
	{
		StopAvatarMovement();
	}

	UAnimMontage* SelectedAlertMontage = SelectAlertMontage();
	const bool bHasAlertMontage = IsValid(SelectedAlertMontage);
	if (bHasAlertMontage)
	{
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			SelectedAlertMontage,
			MontagePlayRate);

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAlertMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleAlertMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAlertMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleAlertMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	const float SafePlayRate = FMath::Max(MontagePlayRate, KINDA_SMALL_NUMBER);
	const float MontageDuration = bHasAlertMontage ? SelectedAlertMontage->GetPlayLength() / SafePlayRate : 0.0f;
	const float FinishDelay = FMath::Max(AlertDuration, MontageDuration);

	if (FinishDelay > 0.0f && GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AlertTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::FinishAlert, false),
			FinishDelay,
			false);
		return;
	}

	if (!bHasAlertMontage || bEndWhenMontageEnds)
	{
		FinishAlert(false);
	}
}

void UPRGameplayAbility_EnemyAlert::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AlertTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyAlert::HandleAlertMontageCompleted()
{
	if (bEndWhenMontageEnds)
	{
		FinishAlert(false);
	}
}

UAnimMontage* UPRGameplayAbility_EnemyAlert::SelectAlertMontage()
{
	TArray<UAnimMontage*> ValidAlertMontages;
	for (const TObjectPtr<UAnimMontage>& AlertMontageCandidate : AlertMontageCandidates)
	{
		if (IsValid(AlertMontageCandidate.Get()))
		{
			ValidAlertMontages.Add(AlertMontageCandidate.Get());
		}
	}

	if (ValidAlertMontages.IsEmpty())
	{
		return nullptr;
	}

	const int32 SelectedIndex = FMath::RandHelper(ValidAlertMontages.Num());
	return ValidAlertMontages[SelectedIndex];
}

void UPRGameplayAbility_EnemyAlert::HandleAlertMontageInterrupted()
{
	FinishAlert(true);
}

void UPRGameplayAbility_EnemyAlert::FinishAlert(bool bWasCancelled)
{
	if (bAlertFinished)
	{
		return;
	}

	bAlertFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_EnemyAlert::StopAvatarMovement() const
{
	ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarCharacter))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = AvatarCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (AAIController* AIController = Cast<AAIController>(AvatarCharacter->GetController()))
	{
		AIController->StopMovement();
	}
}
