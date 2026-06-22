// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 사망 어빌리티 구현)
#include "PRGA_PlayerDeath.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_PlayerDeath::UPRGA_PlayerDeath()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Death);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData DeathTriggerData;
	DeathTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DeathTriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(DeathTriggerData);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	AbilitiesToCancelOnMontage.AddTag(PRGameplayTags::Ability_Player_Down);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_GetUp);
	
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);


	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Block_Move);
	
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UPRGA_PlayerDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EnterDeath();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGA_PlayerDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_PlayerDeath::HandleDeathMontageCompleted()
{
}

void UPRGA_PlayerDeath::HandleDeathMontageInterrupted()
{
}

void UPRGA_PlayerDeath::EnterDeath()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
		}
	}

	NotifyDeathToGameMode();
	
	UAnimMontage* MontageToPlay = nullptr;
	
	if (auto ASC =  GetAbilitySystemComponentFromActorInfo())
	{
		const bool bIsDown = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down); 
		MontageToPlay = bIsDown ? DownToDeathMontage : DeathMontage;
		ASC->CancelAbilities(&AbilitiesToCancelOnMontage);
	}
	
	if (!IsValid(MontageToPlay))
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		NAME_None,
		true);

	MontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UPRGA_PlayerDeath::NotifyDeathToGameMode() const
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
