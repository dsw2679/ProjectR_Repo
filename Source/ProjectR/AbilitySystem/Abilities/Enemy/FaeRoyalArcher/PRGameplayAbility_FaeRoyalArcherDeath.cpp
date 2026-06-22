// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 사망 상태/패턴 어빌리티 구현)
#include "PRGameplayAbility_FaeRoyalArcherDeath.h"

#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Character/Enemy/FaeRoyalArcher/PRFaeRoyalArcherCharacter.h"

// ===== 초기화 =====

UPRGameplayAbility_FaeRoyalArcherDeath::UPRGameplayAbility_FaeRoyalArcherDeath()
{
	MontagePlayRate = 1.0f;
	bEndAbilityWhenMontageEnds = false;
	bDisableMovementOnDeath = true;
	bDestroyActorOnDeath = true;
	DeathDestroyDelay = 2.4f;
	bUseDissolveOnDeath = true;
	DissolveTimingMode = EPRDeathDissolveTimingMode::AfterMontageStart;
	DissolveDelayAfterMontageStart = 0.8f;
	DissolveDelayAfterMontage = 0.05f;
	DissolveDuration = 1.0f;
	DissolveStartValue = 1.0f;
	DissolveEndValue = 0.0f;
}

// ===== UGameplayAbility Interface =====

void UPRGameplayAbility_FaeRoyalArcherDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	PrepareRoyalArcherDeathPresentation();

	if (UAnimMontage* SelectedMontage = ResolveDeathMontageCandidate())
	{
		DeathMontage = SelectedMontage;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// ===== 몽타주 선택 =====

UAnimMontage* UPRGameplayAbility_FaeRoyalArcherDeath::ResolveDeathMontageCandidate() const
{
	TArray<UAnimMontage*> ValidCandidates;
	for (UAnimMontage* Candidate : DeathMontageCandidates)
	{
		if (IsValid(Candidate))
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.IsEmpty())
	{
		return DeathMontage.Get();
	}

	if (!bRandomizeDeathMontageCandidate || ValidCandidates.Num() == 1)
	{
		return ValidCandidates[0];
	}

	return ValidCandidates[FMath::RandRange(0, ValidCandidates.Num() - 1)];
}

// ===== 표현 상태 정리 =====

void UPRGameplayAbility_FaeRoyalArcherDeath::PrepareRoyalArcherDeathPresentation() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (APRFaeRoyalArcherCharacter* RoyalArcher = Cast<APRFaeRoyalArcherCharacter>(AvatarActor))
	{
		RoyalArcher->ClearPerchIdlePose();
	}

	if (APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(AvatarActor))
	{
		EnemyCharacter->ClearCombatMovePresentationContext();
	}

	if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}
