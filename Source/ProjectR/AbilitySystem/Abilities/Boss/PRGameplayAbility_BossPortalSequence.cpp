// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 시퀀스 어빌리티 구현)
#include "PRGameplayAbility_BossPortalSequence.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectR/AI/Boss/PRBossPortalActor.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossPortalSequence, Log, All);

UPRGameplayAbility_BossPortalSequence::UPRGameplayAbility_BossPortalSequence()
{
}

void UPRGameplayAbility_BossPortalSequence::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanRunBossPattern() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBossPatternAttackCommit();

	SpawnedPatternActors.Reset();
	SpawnedPortalRefs.Reset();
	bPortalActorsSpawned = false;

	if (SpawnTimingMode == EPRBossPortalSpawnTimingMode::ImmediateOnActivate)
	{
		if (!SpawnConfiguredPortals())
		{
			UE_LOG(LogPRBossPortalSequence, Warning,
				TEXT("BossPortalSequence failed to spawn portals on activate. Ability=%s, ConfigCount=%d"),
				*GetNameSafe(this),
				PatternActorSpawnConfigs.Num());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		StartPortalSequenceFinishTimer();
		return;
	}

	if (!RegisterCharacterEventListener())
	{
		UE_LOG(LogPRBossPortalSequence, Warning,
			TEXT("BossPortalSequence cannot wait for CharacterEvent because router is missing. Ability=%s"),
			*GetNameSafe(this));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bool bStartedSummonMontageTask = false;
	if (IsValid(PortalSummonMontage))
	{
		ActivePortalSummonMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			PortalSummonMontage,
			FMath::Max(PortalSummonMontagePlayRate, UE_SMALL_NUMBER),
			PortalSummonMontageStartSection);

		if (IsValid(ActivePortalSummonMontageTask))
		{
			ActivePortalSummonMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_BossPortalSequence::HandlePortalSummonMontageCompleted);
			ActivePortalSummonMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_BossPortalSequence::HandlePortalSummonMontageInterrupted);
			ActivePortalSummonMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_BossPortalSequence::HandlePortalSummonMontageInterrupted);
			ActivePortalSummonMontageTask->ReadyForActivation();
			bStartedSummonMontageTask = true;
		}
	}

	if (!bStartedSummonMontageTask || !bEndAbilityOnSummonMontageCompleted)
	{
		StartPortalSequenceFinishTimer();
	}
}

void UPRGameplayAbility_BossPortalSequence::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalSequenceTimerHandle);
	}

	UnregisterCharacterEventListener();

	if (IsValid(ActivePortalSummonMontageTask))
	{
		ActivePortalSummonMontageTask->EndTask();
		ActivePortalSummonMontageTask = nullptr;
	}

	if (bExpireSpawnedPortalsOnEnd)
	{
		ExpireSpawnedPortals();
	}

	EndBossPatternAttackCommit();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_BossPortalSequence::SpawnConfiguredPortals()
{
	if (bPortalActorsSpawned)
	{
		return false;
	}

	ResetSharedEnvQueryResultCache();

	AActor* PatternTarget = GetBossPatternTarget();
	for (const FPRBossPatternActorSpawnConfig& SpawnConfig : PatternActorSpawnConfigs)
	{
		APRBossPatternActor* SpawnedActor = SpawnPatternActor(SpawnConfig);
		if (!IsValid(SpawnedActor))
		{
			continue;
		}

		SpawnedPatternActors.Add(SpawnedActor);

		APRBossPortalActor* SpawnedPortal = Cast<APRBossPortalActor>(SpawnedActor);
		if (!IsValid(SpawnedPortal))
		{
			UE_LOG(LogPRBossPortalSequence, Warning,
				TEXT("BossPortalSequence spawned non-portal actor. Ability=%s, Actor=%s"),
				*GetNameSafe(this),
				*GetNameSafe(SpawnedActor));
			continue;
		}

		SpawnedPortal->SetAndLockPortalTarget(PatternTarget);
		SpawnedPortalRefs.Add(SpawnedPortal);
	}

	TArray<APRBossPatternActor*> SpawnedActorsForEvent;
	SpawnedActorsForEvent.Reserve(SpawnedPatternActors.Num());
	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActorsForEvent.Add(SpawnedActor);
		}
	}

	bPortalActorsSpawned = true;
	StartSpawnedPortals();
	BP_OnPatternActorsSpawned(SpawnedActorsForEvent);
	return SpawnedPortalRefs.Num() > 0;
}

void UPRGameplayAbility_BossPortalSequence::StartSpawnedPortals()
{
	if (!bStartPortalsAfterSpawn)
	{
		return;
	}

	int32 StartedPortalCount = 0;
	for (APRBossPortalActor* SpawnedPortal : SpawnedPortalRefs)
	{
		if (!IsValid(SpawnedPortal))
		{
			continue;
		}

		const float StartDelay = FMath::Max(PortalStartInterval, 0.0f) * StartedPortalCount;
		SpawnedPortal->StartPortalTelegraphAfterDelay(StartDelay);
		++StartedPortalCount;
	}
}

void UPRGameplayAbility_BossPortalSequence::StartPortalSequenceFinishTimer()
{
	if (PortalSequenceDuration <= 0.0f)
	{
		if (SpawnTimingMode == EPRBossPortalSpawnTimingMode::ImmediateOnActivate || bPortalActorsSpawned)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.IsTimerActive(PortalSequenceTimerHandle))
		{
			return;
		}

		TimerManager.SetTimer(
			PortalSequenceTimerHandle,
			this,
			&UPRGameplayAbility_BossPortalSequence::FinishPortalSequence,
			PortalSequenceDuration,
			false);
	}
}

void UPRGameplayAbility_BossPortalSequence::FinishPortalSequence()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_BossPortalSequence::ExpireSpawnedPortals()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->CancelPatternActor();
		}
	}

	SpawnedPortalRefs.Reset();
	SpawnedPatternActors.Reset();
}

bool UPRGameplayAbility_BossPortalSequence::RegisterCharacterEventListener()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	ActiveEventRouter = AvatarActor->FindComponentByClass<UPRFaerinCharacterEventRouterComponent>();
	if (!IsValid(ActiveEventRouter))
	{
		return false;
	}

	ActiveEventRouter->RegisterFaerinEventListener(
		this,
		FFaerinCharacterEventSignature::FDelegate::CreateUObject(
			this,
			&UPRGameplayAbility_BossPortalSequence::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_BossPortalSequence::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

void UPRGameplayAbility_BossPortalSequence::HandleFaerinCharacterEvent(FName EventName)
{
	if (SpawnTimingMode != EPRBossPortalSpawnTimingMode::OnCharacterEvent
		|| EventName != SpawnCharacterEventName
		|| bPortalActorsSpawned)
	{
		return;
	}

	if (!SpawnConfiguredPortals())
	{
		UE_LOG(LogPRBossPortalSequence, Warning,
			TEXT("BossPortalSequence received spawn event but failed to spawn portals. Ability=%s, Event=%s, ConfigCount=%d"),
			*GetNameSafe(this),
			*EventName.ToString(),
			PatternActorSpawnConfigs.Num());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (bEndAbilityAfterEventSpawn)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!IsValid(ActivePortalSummonMontageTask))
	{
		StartPortalSequenceFinishTimer();
	}
}

void UPRGameplayAbility_BossPortalSequence::HandlePortalSummonMontageCompleted()
{
	ActivePortalSummonMontageTask = nullptr;

	if (!bPortalActorsSpawned)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (bEndAbilityOnSummonMontageCompleted)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	StartPortalSequenceFinishTimer();
}

void UPRGameplayAbility_BossPortalSequence::HandlePortalSummonMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
