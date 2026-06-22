// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Groggy 상태/패턴 어빌리티 구현)
#include "PRGameplayAbility_FaeRoyalArcherGroggy.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Character/Enemy/FaeRoyalArcher/PRFaeRoyalArcherCharacter.h"

namespace
{
	bool DoesMontageContainSection(const UAnimMontage* Montage, const FName SectionName)
	{
		return IsValid(Montage) && !SectionName.IsNone() && Montage->GetSectionIndex(SectionName) != INDEX_NONE;
	}
}

// ===== 초기화 =====

UPRGameplayAbility_FaeRoyalArcherGroggy::UPRGameplayAbility_FaeRoyalArcherGroggy()
{
	MontagePlayRate = 1.0f;
	bEndWhenMontageEnds = true;
	GroggyDuration = 1.25f;
}

// ===== UGameplayAbility Interface =====

void UPRGameplayAbility_FaeRoyalArcherGroggy::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bRoyalArcherGroggyFinished = false;

	PrepareRoyalArcherGroggyPresentation();

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		// 이동 속도를 즉시 끊어 그로기 몽타주가 이동 입력과 섞이지 않게 한다.
		Character->GetCharacterMovement()->StopMovementImmediately();

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}
	}

	if (UAnimMontage* SelectedMontage = ResolveGroggyMontageCandidate())
	{
		GroggyMontage = SelectedMontage;
	}

	const bool bHasGroggyMontage = IsValid(GroggyMontage);
	if (bHasGroggyMontage)
	{
		RoyalArcherActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			GroggyMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

		if (IsValid(RoyalArcherActiveMontageTask))
		{
			RoyalArcherActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageCompleted);
			RoyalArcherActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageCompleted);
			RoyalArcherActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageInterrupted);
			RoyalArcherActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageInterrupted);
			RoyalArcherActiveMontageTask->ReadyForActivation();
		}
	}

	const float FinishDelay = CalculateRoyalArcherGroggyFinishDelay(bHasGroggyMontage);
	if (ShouldUseSequencedGroggySections(GroggyMontage))
	{
		ScheduleGroggyRecoverSectionJump(FinishDelay);
	}

	if (UWorld* World = GetWorld())
	{
		if (FinishDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				RoyalArcherGroggyTimerHandle,
				FTimerDelegate::CreateUObject(this, &UPRGameplayAbility_FaeRoyalArcherGroggy::FinishRoyalArcherGroggy, false),
				FinishDelay,
				false);
		}
		else
		{
			FinishRoyalArcherGroggy(false);
		}
	}
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bRoyalArcherGroggyFinished = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoyalArcherGroggyTimerHandle);
		World->GetTimerManager().ClearTimer(RoyalArcherGroggyRecoverTimerHandle);
	}

	if (IsValid(RoyalArcherActiveMontageTask))
	{
		RoyalArcherActiveMontageTask->EndTask();
		RoyalArcherActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ===== 몽타주 선택 =====

UAnimMontage* UPRGameplayAbility_FaeRoyalArcherGroggy::ResolveGroggyMontageCandidate() const
{
	TArray<UAnimMontage*> ValidCandidates;
	for (UAnimMontage* Candidate : GroggyMontageCandidates)
	{
		if (IsValid(Candidate))
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.IsEmpty())
	{
		return GroggyMontage.Get();
	}

	if (!bRandomizeGroggyMontageCandidate || ValidCandidates.Num() == 1)
	{
		return ValidCandidates[0];
	}

	return ValidCandidates[FMath::RandRange(0, ValidCandidates.Num() - 1)];
}

// ===== 표현 상태 정리 =====

void UPRGameplayAbility_FaeRoyalArcherGroggy::PrepareRoyalArcherGroggyPresentation() const
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

float UPRGameplayAbility_FaeRoyalArcherGroggy::CalculateRoyalArcherGroggyFinishDelay(bool bHasGroggyMontage) const
{
	if (ShouldUseSequencedGroggySections(GroggyMontage))
	{
		// 섹션 체인 그로기는 몽타주 자연 종료가 아니라 명시적인 그로기 유지 시간으로 제어한다.
		return FMath::Max(GroggyDuration, 0.0f);
	}

	const float MontageDuration = bHasGroggyMontage && bEndWhenMontageEnds
		? GroggyMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)
		: 0.0f;

	// 공용 Groggy GA와 동일하게 최소 그로기 시간과 몽타주 길이 중 더 긴 값을 사용한다.
	return FMath::Max(GroggyDuration, MontageDuration + 0.1f);
}

bool UPRGameplayAbility_FaeRoyalArcherGroggy::ShouldUseSequencedGroggySections(const UAnimMontage* SelectedMontage) const
{
	const bool bHasLoopSection = GroggyLoopSectionName.IsNone()
		|| DoesMontageContainSection(SelectedMontage, GroggyLoopSectionName);
	const bool bHasRecoverSection = DoesMontageContainSection(SelectedMontage, GroggyRecoverSectionName);

	return bUseSequencedGroggySections
		&& IsValid(SelectedMontage)
		&& bHasLoopSection
		&& bHasRecoverSection;
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::ScheduleGroggyRecoverSectionJump(float FinishDelay)
{
	if (!ShouldUseSequencedGroggySections(GroggyMontage))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float RecoverJumpDelay = FMath::Max(FinishDelay - FMath::Max(GroggyRecoverLeadTime, 0.0f), 0.0f);
	if (RecoverJumpDelay <= 0.0f)
	{
		JumpToGroggyRecoverSection();
		return;
	}

	World->GetTimerManager().SetTimer(
		RoyalArcherGroggyRecoverTimerHandle,
		this,
		&UPRGameplayAbility_FaeRoyalArcherGroggy::JumpToGroggyRecoverSection,
		RecoverJumpDelay,
		false);
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageCompleted()
{
	if (ShouldUseSequencedGroggySections(GroggyMontage))
	{
		if (bAllowSequencedMontageCompletionToEndGroggy)
		{
			FinishRoyalArcherGroggy(false);
		}

		return;
	}

	if (bEndWhenMontageEnds)
	{
		FinishRoyalArcherGroggy(false);
	}
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::HandleRoyalArcherGroggyMontageInterrupted()
{
	FinishRoyalArcherGroggy(true);
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::JumpToGroggyRecoverSection()
{
	if (!ShouldUseSequencedGroggySections(GroggyMontage))
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = IsValid(Character) ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = IsValid(MeshComponent) ? MeshComponent->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsPlaying(GroggyMontage))
	{
		return;
	}

	AnimInstance->Montage_JumpToSection(GroggyRecoverSectionName, GroggyMontage);
}

void UPRGameplayAbility_FaeRoyalArcherGroggy::FinishRoyalArcherGroggy(bool bWasCancelled)
{
	if (bRoyalArcherGroggyFinished)
	{
		return;
	}

	bRoyalArcherGroggyFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
