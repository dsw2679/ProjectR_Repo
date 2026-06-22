// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 다운 어빌리티 구현)
#include "PRGA_PlayerDown.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UPRGA_PlayerDown::UPRGA_PlayerDown()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Down);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData DownTriggerData;
	DownTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DownTriggerData.TriggerTag = PRGameplayTags::Event_Ability_Down;
	AbilityTriggers.Add(DownTriggerData);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Down);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UPRGA_PlayerDown::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	bDownToDeathStarted = false;
	bDownEnterMovementBlockAdded = false;
	bDownDeathFinalized = false;

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
		}
	}

	PlayDownAudio();
	PlayDownEnterMontage();
	StartDownTimer();
	//StartDownDeathEventWait();
	NotifyDownToGameMode();
}

void UPRGA_PlayerDown::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopDownTimer();
	ClearDownTimerInfo();
	StopDownAudio();
	SetDownEnterMovementBlocked(false);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bDownToDeathStarted = false;
}

void UPRGA_PlayerDown::HandleDownMontageCompleted()
{
	ActiveMontageTask = nullptr;
	SetDownEnterMovementBlocked(false);
}

void UPRGA_PlayerDown::HandleDownMontageInterrupted()
{
	ActiveMontageTask = nullptr;
	SetDownEnterMovementBlocked(false);
}

void UPRGA_PlayerDown::StartDownTimer()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float ClampedDownTimeout = FMath::Max(DownTimeout, 0.0f);
	StartDownTimerInfo(ClampedDownTimeout);

	World->GetTimerManager().SetTimer(
		DownTimerHandle,
		this,
		&UPRGA_PlayerDown::HandleDownTimeout,
		ClampedDownTimeout,
		false);
}

void UPRGA_PlayerDown::StopDownTimer()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(DownTimerHandle);
	}
}

void UPRGA_PlayerDown::StartDownTimerInfo(float DurationSeconds)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	if (!IsValid(PlayerState))
	{
		return;
	}

	const float ClampedDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	PlayerState->StartDownTimerInfo(
		ClampedDurationSeconds,
		ResolveServerWorldTimeSeconds() + ClampedDurationSeconds);
}

void UPRGA_PlayerDown::ClearDownTimerInfo() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	if (IsValid(PlayerState))
	{
		PlayerState->ClearDownTimerInfo();
	}
}

float UPRGA_PlayerDown::ResolveServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UPRGA_PlayerDown::HandleDownTimeout()
{
	SendDownDeathConfirmedEvent();
}

void UPRGA_PlayerDown::SendDownDeathConfirmedEvent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	if (IsValid(PlayerState))
	{
		PlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_PlayerDeathConfirmed);
		
		UWorld* World = GetWorld();
		APRPlayGameMode* PlayGameMode = IsValid(World) ? World->GetAuthGameMode<APRPlayGameMode>() : nullptr;
		if (IsValid(PlayGameMode))
		{
			PlayGameMode->NotifyPlayerSurvivalStateChanged(PlayerState);
		}
	}
}

void UPRGA_PlayerDown::SetDownEnterMovementBlocked(bool bBlocked)
{
	if (bDownEnterMovementBlockAdded == bBlocked)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (bBlocked)
	{
		ASC->AddLooseGameplayTag(PRGameplayTags::State_Block_Move);
		ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
		bDownEnterMovementBlockAdded = true;
		return;
	}

	ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
	bDownEnterMovementBlockAdded = false;
}

void UPRGA_PlayerDown::FinalizeDownDeath()
{
	if (bDownDeathFinalized)
	{
		return;
	}

	bDownDeathFinalized = true;
	ApplyDownDeathStateTags();

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
			CharacterMovement->DisableMovement();
		}
	}

	NotifyDeathToGameMode();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGA_PlayerDown::ApplyDownDeathStateTags()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->AddLooseGameplayTag(PRGameplayTags::State_Dead);
	ASC->AddLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->AddLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Dead);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
}

void UPRGA_PlayerDown::NotifyDownToGameMode() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRPlayGameMode* PlayGameMode = IsValid(World) ? World->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(PlayGameMode))
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	PlayGameMode->NotifyPlayerSurvivalStateChanged(PlayerState);
}

void UPRGA_PlayerDown::NotifyDeathToGameMode() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRPlayGameMode* PlayGameMode = IsValid(World) ? World->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(PlayGameMode))
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	PlayGameMode->NotifyPlayerSurvivalStateChanged(PlayerState);
}

void UPRGA_PlayerDown::PlayDownEnterMontage()
{
	if (!IsValid(DownEnterMontage))
	{
		return;
	}

	SetDownEnterMovementBlocked(true);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DownEnterMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));
	if (!IsValid(ActiveMontageTask))
	{
		SetDownEnterMovementBlocked(false);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

bool UPRGA_PlayerDown::CanPlayDownAudio() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	return IsValid(PlayerCharacter) && PlayerCharacter->IsLocallyControlled();
}

void UPRGA_PlayerDown::PlayDownAudio()
{
	if (!CanPlayDownAudio())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float ClampedVolumeMultiplier = FMath::Max(DownAudioVolumeMultiplier, 0.0f);
	const float ClampedFadeInDuration = FMath::Max(DownAudioFadeInDuration, 0.0f);

	if (IsValid(DownEnterBGM))
	{
		DownEnterBGMAudioComponent = UGameplayStatics::SpawnSound2D(
			World,
			DownEnterBGM,
			ClampedVolumeMultiplier,
			1.0f,
			0.0f,
			nullptr,
			false,
			true);

		if (IsValid(DownEnterBGMAudioComponent))
		{
			DownEnterBGMAudioComponent->FadeIn(ClampedFadeInDuration, ClampedVolumeMultiplier, 0.0f);
		}
	}

	if (DownTimeout <= UrgentHeartbeatRemainingTime)
	{
		StartUrgentHeartbeatLoop();
		return;
	}

	if (IsValid(DownHeartbeatLoop))
	{
		DownHeartbeatAudioComponent = UGameplayStatics::SpawnSound2D(
			World,
			DownHeartbeatLoop,
			ClampedVolumeMultiplier,
			1.0f,
			0.0f,
			nullptr,
			false,
			false);

		if (IsValid(DownHeartbeatAudioComponent))
		{
			DownHeartbeatAudioComponent->FadeIn(ClampedFadeInDuration, ClampedVolumeMultiplier, 0.0f);
		}
	}

	const float UrgentHeartbeatDelay = FMath::Max(DownTimeout - UrgentHeartbeatRemainingTime, 0.0f);
	World->GetTimerManager().SetTimer(
		UrgentHeartbeatTimerHandle,
		this,
		&UPRGA_PlayerDown::StartUrgentHeartbeatLoop,
		UrgentHeartbeatDelay,
		false);
}

void UPRGA_PlayerDown::StartUrgentHeartbeatLoop()
{
	if (!CanPlayDownAudio())
	{
		return;
	}

	FadeOutAndClearAudioComponent(DownHeartbeatAudioComponent);

	if (IsValid(DownUrgentHeartbeatAudioComponent) || !IsValid(DownUrgentHeartbeatLoop))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float ClampedVolumeMultiplier = FMath::Max(DownAudioVolumeMultiplier, 0.0f);
	DownUrgentHeartbeatAudioComponent = UGameplayStatics::SpawnSound2D(
		World,
		DownUrgentHeartbeatLoop,
		ClampedVolumeMultiplier,
		1.0f,
		0.0f,
		nullptr,
		false,
		false);

	if (IsValid(DownUrgentHeartbeatAudioComponent))
	{
		DownUrgentHeartbeatAudioComponent->FadeIn(FMath::Max(DownAudioFadeInDuration, 0.0f), ClampedVolumeMultiplier, 0.0f);
	}
}

void UPRGA_PlayerDown::StopDownAudio()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(UrgentHeartbeatTimerHandle);
	}

	FadeOutAndClearAudioComponent(DownEnterBGMAudioComponent);
	FadeOutAndClearAudioComponent(DownHeartbeatAudioComponent);
	FadeOutAndClearAudioComponent(DownUrgentHeartbeatAudioComponent);
}

void UPRGA_PlayerDown::FadeOutAndClearAudioComponent(TObjectPtr<UAudioComponent>& AudioComponent)
{
	if (!IsValid(AudioComponent))
	{
		AudioComponent = nullptr;
		return;
	}

	const float ClampedFadeOutDuration = FMath::Max(DownAudioFadeOutDuration, 0.0f);
	UWorld* World = GetWorld();
	if (!IsValid(World) || ClampedFadeOutDuration <= UE_SMALL_NUMBER)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		AudioComponent = nullptr;
		return;
	}

	UAudioComponent* FadingAudioComponent = AudioComponent;
	FadingAudioComponent->FadeOut(ClampedFadeOutDuration, 0.0f);
	AudioComponent = nullptr;

	FTimerHandle DestroyTimerHandle;
	TWeakObjectPtr<UAudioComponent> WeakAudioComponent(FadingAudioComponent);
	World->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		FTimerDelegate::CreateLambda([WeakAudioComponent]()
		{
			if (UAudioComponent* AudioComponentToDestroy = WeakAudioComponent.Get())
			{
				AudioComponentToDestroy->Stop();
				AudioComponentToDestroy->DestroyComponent();
			}
		}),
		ClampedFadeOutDuration,
		false);
}
