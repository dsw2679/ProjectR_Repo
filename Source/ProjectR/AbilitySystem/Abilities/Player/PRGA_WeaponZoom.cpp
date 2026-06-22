// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 Weapon Zoom 어빌리티 구현)
#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_WeaponZoom.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"

UPRGA_WeaponZoom::UPRGA_WeaponZoom()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Zoom);
	SetAssetTags(DefaultAbilityTags);

	InputTag = PRGameplayTags::Input_Ability_Zoom;
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;

	ActivationRequiredTags.AddTag(PRGameplayTags::State_Aiming);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_WeaponZooming);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PlayerHitReactLocked);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UPRGA_WeaponZoom::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CameraModeAfterZoom = EPRCameraMode::Default;

	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		CachedASC = ASC;
		GameplayTagUpdatedHandle = ASC->OnGameplayTagUpdated.AddUObject(this, &ThisClass::HandleGameplayTagUpdated);
	}

	ApplyLocalZoom();
}

void UPRGA_WeaponZoom::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndZoomAbility(false, EPRCameraMode::Aim);
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_WeaponZoom::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CleanupZoom();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UPRGA_WeaponZoom::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  bool bReplicateEndAbility,
                                  bool bWasCancelled)
{
	CleanupZoom();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_WeaponZoom::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	if (!IsActive())
	{
		return;
	}

	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Aiming) && !bTagExists)
	{
		EndZoomAbility(true, EPRCameraMode::Default);
		return;
	}

	const bool bBlockedStateAdded =
		ChangedTag.MatchesTagExact(PRGameplayTags::State_PlayerInputLocked)
		|| ChangedTag.MatchesTagExact(PRGameplayTags::State_PlayerHitReactLocked)
		|| ChangedTag.MatchesTagExact(PRGameplayTags::State_Dodging)
		|| ChangedTag.MatchesTagExact(PRGameplayTags::State_Reloading)
		|| ChangedTag.MatchesTagExact(PRGameplayTags::State_Down)
		|| ChangedTag.MatchesTagExact(PRGameplayTags::State_Dead);

	if (bBlockedStateAdded && bTagExists)
	{
		EndZoomAbility(true, EPRCameraMode::Default);
	}
}

void UPRGA_WeaponZoom::ApplyLocalZoom()
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
		PreviousCameraMode = SpringArm->GetCameraMode();
		PreviousCameraModeSettings = SpringArm->GetCameraModeSettings();
		bHasPreviousCameraModeSettings = true;

		SpringArm->SetCameraModeSettings(
			EPRCameraMode::WeaponZoom,
			FPRSpringArmCameraModeSettings(ZoomTargetArmLength, ZoomTargetOffset, ZoomSocketOffset));
		SpringArm->SetCameraMode(EPRCameraMode::WeaponZoom);
	}

	if (USkeletalMeshComponent* MeshComponent = Character->GetMesh())
	{
		bSavedOwnerNoSee = MeshComponent->bOwnerNoSee;
		bHasSavedOwnerNoSee = true;
		MeshComponent->SetOwnerNoSee(true);
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (IsValid(PlayerController->PlayerCameraManager))
		{
			ActiveCameraModifier = Cast<UPRCameraModifier>(
				PlayerController->PlayerCameraManager->AddNewCameraModifier(UPRCameraModifier::StaticClass()));

			if (IsValid(ActiveCameraModifier))
			{
				ActiveCameraModifier->SetActionCameraSettings(ZoomFOV, ZoomModifierLocationOffset);
			}
		}

		if (APRPlayerController* PRPlayerController = Cast<APRPlayerController>(PlayerController))
		{
			if (UPRUIControllerComponent* UIController = PRPlayerController->GetUIController())
			{
				UIController->ShowWeaponScope();
			}
		}
	}

	bZoomApplied = true;
}

void UPRGA_WeaponZoom::CleanupZoom()
{
	if (GameplayTagUpdatedHandle.IsValid())
	{
		if (UPRAbilitySystemComponent* ASC = CachedASC.Get())
		{
			ASC->OnGameplayTagUpdated.Remove(GameplayTagUpdatedHandle);
		}

		GameplayTagUpdatedHandle.Reset();
		CachedASC.Reset();
	}

	if (bZoomApplied && IsLocallyControlled())
	{
		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (APRPlayerController* PlayerController = Cast<APRPlayerController>(Character->GetController()))
			{
				if (UPRUIControllerComponent* UIController = PlayerController->GetUIController())
				{
					UIController->HideWeaponScope();
				}
			}

			if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
			{
				if (CameraModeAfterZoom == EPRCameraMode::Aim
					&& PreviousCameraMode == EPRCameraMode::Aim
					&& bHasPreviousCameraModeSettings)
				{
					SpringArm->SetCameraModeSettings(EPRCameraMode::Aim, PreviousCameraModeSettings);
					SpringArm->SetCameraMode(EPRCameraMode::Aim);
				}
				else
				{
					SpringArm->SetCameraMode(EPRCameraMode::Default);
				}
			}

			if (bHasSavedOwnerNoSee)
			{
				if (USkeletalMeshComponent* MeshComponent = Character->GetMesh())
				{
					MeshComponent->SetOwnerNoSee(bSavedOwnerNoSee);
				}
			}
		}

		if (IsValid(ActiveCameraModifier))
		{
			ActiveCameraModifier->DisableModifier(false);
		}
	}

	ActiveCameraModifier = nullptr;
	CameraModeAfterZoom = EPRCameraMode::Default;
	PreviousCameraMode = EPRCameraMode::Default;
	PreviousCameraModeSettings = FPRSpringArmCameraModeSettings();
	bHasSavedOwnerNoSee = false;
	bSavedOwnerNoSee = false;
	bHasPreviousCameraModeSettings = false;
	bZoomApplied = false;
}

void UPRGA_WeaponZoom::EndZoomAbility(bool bWasCancelled, EPRCameraMode InCameraModeAfterZoom)
{
	if (!IsActive())
	{
		return;
	}

	CameraModeAfterZoom = InCameraModeAfterZoom;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
