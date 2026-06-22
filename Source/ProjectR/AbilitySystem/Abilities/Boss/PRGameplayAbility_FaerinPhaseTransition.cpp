// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보스 페이즈 전환용 BGM 제어 연동)
// Author: 손승우 (페어린 보스 페이즈 전환 시퀀스 및 AI 연동 구현)
#include "PRGameplayAbility_FaerinPhaseTransition.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Components/PRFaerinGodFallComponent.h"
#include "ProjectR/AI/Data/PRFaerinGodFallDataAsset.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/PRGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinPhaseTransition, Log, All);

// ===== 생성 =====

UPRGameplayAbility_FaerinPhaseTransition::UPRGameplayAbility_FaerinPhaseTransition()
{
	SetAssetTags(PRBossAbility::MakePhaseTransitionAssetTags(PRGameplayTags::Ability_Boss_Faerin_PhaseTransition));
}

// ===== UGameplayAbility Interface =====

void UPRGameplayAbility_FaerinPhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter)
		|| !FaerinCharacter->HasAuthority()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PendingTargetPhase = ResolveBossPhaseFromEvent(TriggerEventData);
	bPhaseTransitionCommitted = false;
	bGodFallCastStarted = false;
	bGodFallEntrySucceeded = false;
	bBodyMontageSequenceFinished = false;
	bPausedBrainLogicForPhaseTransition = false;
	bBGMPhasePreviewed = false;
	bStandardPhaseTransitionMontageActive = false;
	PausedBrainComponent.Reset();
	bSuppressBodyMontageCallbacks = false;
	BodyMontageQueue.Reset();
	ActiveBodyMontageIndex = INDEX_NONE;

	if (PendingTargetPhase == EPRBossPhase::Phase2)
	{
		ActiveGodFallComponent = FaerinCharacter->GetGodFallComponent();
		if (!IsValid(ActiveGodFallComponent))
		{
			UE_LOG(LogPRFaerinPhaseTransition, Warning,
				TEXT("Faerin Phase2 transition failed because GodFallComponent is missing. Boss=%s"),
				*GetNameSafe(FaerinCharacter));
			RollbackPhaseTransition();
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		UPRFaerinGodFallDataAsset* GodFallData = ActiveGodFallComponent->GetGodFallData();
		if (IsValid(GodFallData))
		{
			if (IsValid(GodFallData->BossBodyChargeMontage))
			{
				BodyMontageQueue.Add(GodFallData->BossBodyChargeMontage);
			}
			if (IsValid(GodFallData->BossBodyTiltPullMontage))
			{
				BodyMontageQueue.Add(GodFallData->BossBodyTiltPullMontage);
			}
			if (IsValid(GodFallData->BossBodyDropMontage))
			{
				BodyMontageQueue.Add(GodFallData->BossBodyDropMontage);
			}
		}

		AActor* PatternTarget = nullptr;
		if (UPREnemyThreatComponent* ThreatComponent = FaerinCharacter->GetEnemyThreatComponent())
		{
			PatternTarget = ThreatComponent->GetAttackTarget();
		}

		ActiveGodFallComponent->OnGodFallEntryFinished.AddUObject(
			this,
			&UPRGameplayAbility_FaerinPhaseTransition::HandleGodFallEntryFinished);
		ActiveGodFallComponent->OnGodFallCastStarted.AddUObject(
			this,
			&UPRGameplayAbility_FaerinPhaseTransition::HandleGodFallCastStarted);

		PauseOwnerBrainForPhaseTransition(FaerinCharacter);

		if (!ActiveGodFallComponent->StartGodFallEntry(PatternTarget))
		{
			ActiveGodFallComponent->OnGodFallEntryFinished.RemoveAll(this);
			ActiveGodFallComponent->OnGodFallCastStarted.RemoveAll(this);
			RollbackPhaseTransition();
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		return;
	}

	if (TryStartStandardPhaseTransitionMontage(FaerinCharacter))
	{
		return;
	}

	if (!CommitPendingPhaseTransition())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UPRGameplayAbility_FaerinPhaseTransition::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (IsValid(ActiveGodFallComponent))
	{
		ActiveGodFallComponent->OnGodFallEntryFinished.RemoveAll(this);
		ActiveGodFallComponent->OnGodFallCastStarted.RemoveAll(this);
		if (bWasCancelled && !bPhaseTransitionCommitted)
		{
			ActiveGodFallComponent->CancelGodFallEntry();
			ActiveGodFallComponent->CancelGodFallHazards();
		}
		ActiveGodFallComponent = nullptr;
	}

	bSuppressBodyMontageCallbacks = true;
	ClearBodyMontageTask();
	BodyMontageQueue.Reset();
	ActiveBodyMontageIndex = INDEX_NONE;
	bGodFallCastStarted = false;
	bGodFallEntrySucceeded = false;
	bBodyMontageSequenceFinished = false;
	bStandardPhaseTransitionMontageActive = false;
	bSuppressBodyMontageCallbacks = false;
	ResumeOwnerBrainForPhaseTransition();

	if (bWasCancelled && !bPhaseTransitionCommitted)
	{
		RollbackPhaseTransition();
	}

	bBGMPhasePreviewed = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ===== God Fall Entry =====

void UPRGameplayAbility_FaerinPhaseTransition::HandleGodFallCastStarted()
{
	if (bSuppressBodyMontageCallbacks || bGodFallCastStarted)
	{
		return;
	}

	bGodFallCastStarted = true;
	BroadcastBGMPhasePreview(PendingTargetPhase);
	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo()))
	{
		FaerinCharacter->BroadcastBossBGMPatternCue(PRGameplayTags::Pattern_Boss_Faerin_Godfall);
	}

	if (BodyMontageQueue.IsEmpty())
	{
		bBodyMontageSequenceFinished = true;
		TryCommitPhase2GodFallTransition();
		return;
	}

	PlayNextBodyMontage();
}

void UPRGameplayAbility_FaerinPhaseTransition::HandleGodFallEntryFinished(const bool bSucceeded)
{
	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!bSucceeded)
	{
		RollbackPhaseTransition();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	bGodFallEntrySucceeded = true;
	TryCommitPhase2GodFallTransition();
}

void UPRGameplayAbility_FaerinPhaseTransition::PlayNextBodyMontage()
{
	if (bSuppressBodyMontageCallbacks)
	{
		return;
	}

	++ActiveBodyMontageIndex;
	if (!BodyMontageQueue.IsValidIndex(ActiveBodyMontageIndex))
	{
		bBodyMontageSequenceFinished = true;
		if (bStandardPhaseTransitionMontageActive)
		{
			if (!CommitPendingPhaseTransition())
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
				return;
			}

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}

		if (IsValid(ActiveGodFallComponent))
		{
			ActiveGodFallComponent->NotifyGodFallBodyMontageSequenceFinished();
		}
		TryCommitPhase2GodFallTransition();
		return;
	}

	ClearBodyMontageTask();

	UAnimMontage* MontageToPlay = BodyMontageQueue[ActiveBodyMontageIndex];
	if (!IsValid(MontageToPlay))
	{
		PlayNextBodyMontage();
		return;
	}

	const bool bIsDropMontage = IsValid(ActiveGodFallComponent)
		&& IsValid(ActiveGodFallComponent->GetGodFallData())
		&& MontageToPlay == ActiveGodFallComponent->GetGodFallData()->BossBodyDropMontage;

	float PlayRate = 1.0f;
	if (bStandardPhaseTransitionMontageActive)
	{
		PlayRate = FMath::Max(StandardTransitionMontagePlayRate, 0.01f);
	}
	else if (IsValid(ActiveGodFallComponent) && IsValid(ActiveGodFallComponent->GetGodFallData()))
	{
		PlayRate = FMath::Max(ActiveGodFallComponent->GetGodFallData()->BossBodyMontagePlayRate, 0.01f);
	}

	ActiveBodyMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		PlayRate);

	if (!IsValid(ActiveBodyMontageTask))
	{
		return;
	}

	ActiveBodyMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageCompleted);
	ActiveBodyMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageCompleted);
	ActiveBodyMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageInterrupted);
	ActiveBodyMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageInterrupted);
	ActiveBodyMontageTask->ReadyForActivation();

	if (bIsDropMontage)
	{
		ActiveGodFallComponent->NotifyGodFallBodyDropMontageStarted();
	}
}

void UPRGameplayAbility_FaerinPhaseTransition::ClearBodyMontageTask()
{
	if (IsValid(ActiveBodyMontageTask))
	{
		UAbilityTask_PlayMontageAndWait* TaskToEnd = ActiveBodyMontageTask;
		ActiveBodyMontageTask = nullptr;
		TaskToEnd->EndTask();
	}
}

bool UPRGameplayAbility_FaerinPhaseTransition::TryStartStandardPhaseTransitionMontage(APRFaerinCharacter* FaerinCharacter)
{
	if (!IsValid(FaerinCharacter))
	{
		return false;
	}

	UAnimMontage* TransitionMontage = ResolveStandardPhaseTransitionMontage();
	if (!IsValid(TransitionMontage))
	{
		if (PendingTargetPhase == EPRBossPhase::Phase3 || PendingTargetPhase == EPRBossPhase::Phase4)
		{
			UE_LOG(LogPRFaerinPhaseTransition, Warning,
				TEXT("Faerin standard phase transition montage is not set. Boss=%s TargetPhase=%d"),
				*GetNameSafe(FaerinCharacter),
				static_cast<int32>(PendingTargetPhase));
		}
		return false;
	}

	BodyMontageQueue.Reset();
	BodyMontageQueue.Add(TransitionMontage);
	ActiveBodyMontageIndex = INDEX_NONE;
	bBodyMontageSequenceFinished = false;
	bStandardPhaseTransitionMontageActive = true;

	BroadcastBGMPhasePreview(PendingTargetPhase);
	PauseOwnerBrainForPhaseTransition(FaerinCharacter);
	PlayNextBodyMontage();
	return true;
}

UAnimMontage* UPRGameplayAbility_FaerinPhaseTransition::ResolveStandardPhaseTransitionMontage() const
{
	switch (PendingTargetPhase)
	{
	case EPRBossPhase::Phase3:
		return Phase2To3TransitionMontage;
	case EPRBossPhase::Phase4:
		return Phase3To4TransitionMontage;
	default:
		return nullptr;
	}
}

bool UPRGameplayAbility_FaerinPhaseTransition::CommitPendingPhaseTransition()
{
	if (bPhaseTransitionCommitted)
	{
		return true;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority())
	{
		return false;
	}

	FaerinCharacter->CommitPhaseTransition(PendingTargetPhase);
	bPhaseTransitionCommitted = true;
	bBGMPhasePreviewed = false;

	if (UPRFaerinGodFallComponent* GodFallComponent = FaerinCharacter->GetGodFallComponent())
	{
		GodFallComponent->ApplyGodFallPhaseScaling(PendingTargetPhase);
	}

	return true;
}

void UPRGameplayAbility_FaerinPhaseTransition::TryCommitPhase2GodFallTransition()
{
	if (bPhaseTransitionCommitted || !bGodFallEntrySucceeded || !bBodyMontageSequenceFinished)
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!CommitPendingPhaseTransition())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_FaerinPhaseTransition::PauseOwnerBrainForPhaseTransition(APRFaerinCharacter* FaerinCharacter)
{
	if (!IsValid(FaerinCharacter))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = FaerinCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	AAIController* AIController = Cast<AAIController>(FaerinCharacter->GetController());
	if (!IsValid(AIController))
	{
		return;
	}

	AIController->StopMovement();
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	UBrainComponent* BrainComponent = AIController->GetBrainComponent();
	if (IsValid(BrainComponent) && !bPausedBrainLogicForPhaseTransition)
	{
		BrainComponent->PauseLogic(TEXT("Faerin Phase Transition"));
		PausedBrainComponent = BrainComponent;
		bPausedBrainLogicForPhaseTransition = true;
	}
}

void UPRGameplayAbility_FaerinPhaseTransition::ResumeOwnerBrainForPhaseTransition()
{
	if (!bPausedBrainLogicForPhaseTransition)
	{
		return;
	}

	if (UBrainComponent* BrainComponent = PausedBrainComponent.Get())
	{
		BrainComponent->ResumeLogic(TEXT("Faerin Phase Transition"));
	}

	PausedBrainComponent.Reset();
	bPausedBrainLogicForPhaseTransition = false;
}

void UPRGameplayAbility_FaerinPhaseTransition::RollbackPhaseTransition()
{
	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(FaerinCharacter) && FaerinCharacter->HasAuthority() && !bPhaseTransitionCommitted)
	{
		if (bBGMPhasePreviewed)
		{
			FaerinCharacter->BroadcastBossBGMPhasePreview(FaerinCharacter->GetCurrentPhase());
			bBGMPhasePreviewed = false;
		}

		FaerinCharacter->CommitPhaseTransition(FaerinCharacter->GetCurrentPhase());
	}
}

void UPRGameplayAbility_FaerinPhaseTransition::BroadcastBGMPhasePreview(EPRBossPhase PreviewPhase)
{
	if (bBGMPhasePreviewed)
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter) || !FaerinCharacter->HasAuthority())
	{
		return;
	}

	FaerinCharacter->BroadcastBossBGMPhasePreview(PreviewPhase);
	bBGMPhasePreviewed = true;
}

EPRBossPhase UPRGameplayAbility_FaerinPhaseTransition::ResolveBossPhaseFromEvent(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData == nullptr)
	{
		return TargetPhase;
	}

	const UEnum* BossPhaseEnum = StaticEnum<EPRBossPhase>();
	if (BossPhaseEnum == nullptr)
	{
		return TargetPhase;
	}

	const int32 EventPhaseValue = FMath::RoundToInt(TriggerEventData->EventMagnitude);
	if (!BossPhaseEnum->IsValidEnumValue(EventPhaseValue))
	{
		return TargetPhase;
	}

	return static_cast<EPRBossPhase>(EventPhaseValue);
}

void UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageCompleted()
{
	if (bSuppressBodyMontageCallbacks)
	{
		return;
	}

	PlayNextBodyMontage();
}

void UPRGameplayAbility_FaerinPhaseTransition::HandleBodyMontageInterrupted()
{
	if (bSuppressBodyMontageCallbacks)
	{
		return;
	}

	if (IsValid(ActiveGodFallComponent) && !bPhaseTransitionCommitted)
	{
		ActiveGodFallComponent->CancelGodFallEntry();
		ActiveGodFallComponent->CancelGodFallHazards();
	}

	RollbackPhaseTransition();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
