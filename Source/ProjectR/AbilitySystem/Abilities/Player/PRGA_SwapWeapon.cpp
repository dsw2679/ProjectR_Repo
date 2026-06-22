// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Swap Weapon 어빌리티 구현)
#include "PRGA_SwapWeapon.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

UPRGA_SwapWeapon::UPRGA_SwapWeapon()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_SwapWeapon);
	SetAssetTags(DefaultAbilityTags);
	InputTag = PRGameplayTags::Input_Ability_SwapWeapon;

	ActivationOwnedTags.AddTag(PRGameplayTags::State_SwappingWeapon);

	// 발사·재장전·사격 모드 전환 중 슬롯 교체로 인한 상태 꼬임 방지
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_SwappingWeapon);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);

	// SetCurrentWeaponSlot이 내부에서 클라→서버 RPC를 처리하므로 어빌리티는 서버에서만 실행
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_SwapWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = ActorInfo != nullptr
		? Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	if (!IsValid(PlayerCharacter))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	// 현재 활성 슬롯의 반대 슬롯에 무기가 있어야 교체 가능
	const EPRWeaponSlotType CurrentSlot = WeaponManager->GetCurrentWeaponSlot();
	const EPRWeaponSlotType TargetSlot = ResolveTargetSlot(CurrentSlot);

	if (TargetSlot == EPRWeaponSlotType::None
		|| !IsValid(WeaponManager->GetWeaponInstanceBySlotType(TargetSlot))
		|| !IsValid(SelectDrawMontage(TargetSlot)))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	return true;
}

void UPRGA_SwapWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EPRWeaponSlotType CurrentSlot = WeaponManager->GetCurrentWeaponSlot();
	const EPRWeaponSlotType TargetSlot = ResolveTargetSlot(CurrentSlot);
	UAnimMontage* DrawMontage = SelectDrawMontage(TargetSlot);
	if (TargetSlot == EPRWeaponSlotType::None
		|| !IsValid(WeaponManager->GetWeaponInstanceBySlotType(TargetSlot))
		|| !IsValid(DrawMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedTargetSlot = TargetSlot;
	WeaponManager->SetCurrentWeaponSlot(TargetSlot);

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DrawMontage,
		FMath::Max(DrawMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_SwapWeapon::OnDrawMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_SwapWeapon::OnDrawMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_SwapWeapon::OnDrawMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_SwapWeapon::OnDrawMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRGA_SwapWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ActiveMontageTask = nullptr;
	CachedTargetSlot = EPRWeaponSlotType::None;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_SwapWeapon::OnDrawMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGA_SwapWeapon::OnDrawMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

UAnimMontage* UPRGA_SwapWeapon::SelectDrawMontage(EPRWeaponSlotType TargetSlot) const
{
	if (TargetSlot == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponDrawMontage;
	}

	if (TargetSlot == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponDrawMontage;
	}

	return nullptr;
}

EPRWeaponSlotType UPRGA_SwapWeapon::ResolveTargetSlot(EPRWeaponSlotType CurrentSlot) const
{
	if (CurrentSlot == EPRWeaponSlotType::Primary)
	{
		return EPRWeaponSlotType::Secondary;
	}

	return EPRWeaponSlotType::Primary;
}
