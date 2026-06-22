// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (무기별 줌(Zoom) 기능 및 조준용 카메라/애님레이어 구현)
// Author: 배유찬 (조준 시 HUD 크로스헤어 및 사격 프리뷰 연동 구현)
#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_PlayerAim.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"

UPRGA_PlayerAim::UPRGA_PlayerAim()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Aim);
	SetAssetTags(DefaultAbilityTags);
	
	ActivationPolicy = EPRAbilityActivationPolicy::WhileInputHeld;

	InputTag = PRGameplayTags::Input_Ability_Aim;
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Aiming);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UPRGA_PlayerAim::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	if (APRPlayerCharacter* AvatarCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UPRWeaponManagerComponent* WeaponManager = AvatarCharacter->GetWeaponManager())
		{
			return WeaponManager->GetCurrentWeaponSlot() != EPRWeaponSlotType::None;
		}
	}
	
	return false;
}

void UPRGA_PlayerAim::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = nullptr;
	if (APRPlayerCharacter* AvatarCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		WeaponManager = AvatarCharacter->GetWeaponManager();
		if (IsValid(WeaponManager))
		{
			if (WeaponManager->GetArmedState() != EPRArmedState::Armed)
			{
				WeaponManager->SetWeaponArmedState(EPRArmedState::Armed);
			}
		}
	}

	if (IsLocallyControlled())
	{
		ApplyAimCameraMode();

		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = AimFOV;
				}
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
			{
				FPRChangeCrosshairEventPayload CrosshairPayload;
				CrosshairPayload.Config = IsValid(WeaponManager) ? WeaponManager->GetActiveCrosshairConfig() : nullptr;
				EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_ChangeCrosshair, CrosshairPayload);
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_Aim_Start);
			}
		}
	}
}

void UPRGA_PlayerAim::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_PlayerAim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled())
	{
		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
			{
				if (SpringArm->GetCameraMode() == EPRCameraMode::Aim)
				{
					SpringArm->SetCameraMode(EPRCameraMode::Default);
				}
			}

			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = 0.0f;
				}
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
			{
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_Aim_End);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_PlayerAim::ApplyAimCameraMode()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
	{
		SpringArm->SetCameraModeSettings(
			EPRCameraMode::Aim,
			FPRSpringArmCameraModeSettings(AimTargetArmLength, AimTargetOffset, AimSocketOffset));
		SpringArm->SetCameraMode(EPRCameraMode::Aim);
	}
}
