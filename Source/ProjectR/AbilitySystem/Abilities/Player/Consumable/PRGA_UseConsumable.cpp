// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (인벤토리 아이템 소모 및 장비 연동 구현)
// Author: 이건주 (소모품 사용 애니메이션 중 무기 비활성화 및 행동 차단 구현)
#include "PRGA_UseConsumable.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_UseConsumable::UPRGA_UseConsumable()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_UseConsumable);
	
	// 소비템 사용중 태그 부여
	ActivationOwnedTags.AddTag(PRGameplayTags::State_UsingConsumable);
	
	// 아래의 행동 차단
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_SwapWeapon);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);
	
	SetAssetTags(DefaultAbilityTags);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

bool UPRGA_UseConsumable::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetSourceObject(Handle, ActorInfo));
	return IsValid(UseMontage) && IsValid(SourceConsumableItem) && SourceConsumableItem->HasAnyStack();
}

void UPRGA_UseConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bCommitted = false;
	ActiveConsumableItem = ResolveConsumableItemFromEventData(TriggerEventData);
	if (!IsValid(ActiveConsumableItem) || !ActiveConsumableItem->HasAnyStack() || !IsValid(UseMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		UseMontage,
		FMath::Max(UseMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();

	ActiveCommitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRGameplayTags::Event_Player_ConsumableCommit,
		nullptr,
		/*OnlyTriggerOnce=*/true,
		/*OnlyMatchExact=*/true);

	if (IsValid(ActiveCommitEventTask))
	{
		ActiveCommitEventTask->EventReceived.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableCommitEvent);
		ActiveCommitEventTask->ReadyForActivation();
	}
}

void UPRGA_UseConsumable::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility,
                                      bool bWasCancelled)
{
	ActiveMontageTask = nullptr;
	ActiveCommitEventTask = nullptr;
	ActiveConsumableItem = nullptr;
	bCommitted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_UseConsumable::SetConsumableItem(UPRItemInstance_Consumable* InConsumableItem)
{
	ActiveConsumableItem = InConsumableItem;
}

void UPRGA_UseConsumable::OnConsumableCommitEvent(FGameplayEventData EventData)
{
	if (bCommitted)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(ActiveConsumableItem) || !ActiveConsumableItem->HasAnyStack())
	{
		K2_CancelAbility();
		return;
	}

	if (!ApplyConsumableEffect())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableAbility][Server] 효과 적용 실패. Ability = %s | Item = %s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveConsumableItem));
		K2_CancelAbility();
		return;
	}

	if (!ConsumeActiveItem())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableAbility][Server] 스택 소모 실패. Ability = %s | Item = %s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveConsumableItem));
		K2_CancelAbility();
		return;
	}

	bCommitted = true;
}

void UPRGA_UseConsumable::OnConsumableMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UPRGA_UseConsumable::OnConsumableMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
}

bool UPRGA_UseConsumable::ApplyConsumableEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !UseEffect.IsValid())
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ActiveConsumableItem);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UseEffect.EffectClass, UseEffect.Level, Context);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	SpecHandle.Data->DynamicGrantedTags.AppendTags(UseEffect.DynamicTags);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	return true;
}

bool UPRGA_UseConsumable::ConsumeActiveItem()
{
	if (IsValid(ActiveConsumableItem))
	{
		return ActiveConsumableItem->RemoveStack(1);
	}
	return false;
}

UPRItemInstance_Consumable* UPRGA_UseConsumable::ResolveConsumableItemFromEventData(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData == nullptr)
	{
		if (UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetCurrentSourceObject()))
		{
			return SourceConsumableItem;
		}

		return ActiveConsumableItem;
	}

	if (UPRItemInstance_Consumable* ConsumableItem = Cast<UPRItemInstance_Consumable>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get())))
	{
		return ConsumableItem;
	}

	if (UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetCurrentSourceObject()))
	{
		return SourceConsumableItem;
	}

	return ActiveConsumableItem;
}
