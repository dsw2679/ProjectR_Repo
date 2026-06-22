// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 앉기(Crouch) 어빌리티 구현)
#include "ProjectR/AbilitySystem/Abilities/Player/PRPlayerCrouchAbility.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Player/PRCameraModifier.h"

UPRPlayerCrouchAbility::UPRPlayerCrouchAbility()
{
	// 태그 설정
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Crouch);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Crouching);
	InputTag = PRGameplayTags::Input_Ability_Crouch;
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);

	// 네트워크 설정
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPRPlayerCrouchAbility::ActivateAbility(const FGameplayAbilitySpecHandle SpecHandle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(SpecHandle, ActorInfo, ActivationInfo, TriggerEventData);
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character) && Character->CanCrouch())
	{
		Character->Crouch();
	}
}

void UPRPlayerCrouchAbility::EndAbility(const FGameplayAbilitySpecHandle SpecHandle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		Character->UnCrouch();
	}
	Super::EndAbility(SpecHandle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRPlayerCrouchAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	EndAbility(Handle,ActorInfo,ActivationInfo,true,true);
}

