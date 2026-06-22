// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Barrier Launch 공격 패턴 어빌리티 구현)
#include "PRGameplayAbility_PenitentBarrierLaunch.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "ProjectR/AbilitySystem/Data/PRBarrierAbilityDataAsset.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"

UPRGameplayAbility_PenitentBarrierLaunch::UPRGameplayAbility_PenitentBarrierLaunch()
{
	FGameplayTagContainer BarrierLaunchTags;
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_BarrierLaunch);
	SetAssetTags(BarrierLaunchTags);
}

bool UPRGameplayAbility_PenitentBarrierLaunch::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;
	const APRPenitentCharacter* PenitentCharacter = ActorInfo
		? Cast<APRPenitentCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;

	return IsValid(AbilitySystemComponent)
		&& IsValid(PenitentCharacter)
		&& IsValid(BarrierData)
		&& BarrierData->LaunchSpeed > 0.0f
		&& PenitentCharacter->HasActiveBarrier()
		&& AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

void UPRGameplayAbility_PenitentBarrierLaunch::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PenitentCharacter->HasActiveBarrier()
		|| !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(BarrierLaunchMontage))
	{
		// 몽타주 누락으로 발사 중단
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierLaunch | BarrierLaunchMontage가 유효하지 않습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	constexpr bool bOnlyTriggerOnce = true;
	constexpr bool bOnlyMatchExact = true;
	ActiveBarrierEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_PenitentBarrierLaunch,
		nullptr,
		bOnlyTriggerOnce,
		bOnlyMatchExact);
	if (!IsValid(ActiveBarrierEventTask))
	{
		// 노티파이 대기 태스크 생성 실패
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		BarrierLaunchMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		MontageStartSection);
	if (!IsValid(ActiveMontageTask))
	{
		// 몽타주 태스크 생성 실패
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bBarrierActionExecuted = false;
	ActiveBarrierEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleBarrierLaunchGameplayEvent);
	ActiveBarrierEventTask->ReadyForActivation();

	ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleBarrierLaunchMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleBarrierLaunchMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleBarrierLaunchMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleBarrierLaunchMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_PenitentBarrierLaunch::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsValid(ActiveBarrierEventTask))
	{
		ActiveBarrierEventTask->EndTask();
		ActiveBarrierEventTask = nullptr;
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	bBarrierActionExecuted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_PenitentBarrierLaunch::HandleBarrierLaunchGameplayEvent(FGameplayEventData Payload)
{
	if (bBarrierActionExecuted || Payload.EventTag != PRCombatGameplayTags::Event_Ability_PenitentBarrierLaunch)
	{
		return;
	}

	bBarrierActionExecuted = true;
	if (!ExecuteBarrierLaunch())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UPRGameplayAbility_PenitentBarrierLaunch::HandleBarrierLaunchMontageCompleted()
{
	if (!bBarrierActionExecuted)
	{
		// 발사 노티파이 누락 감지
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierLaunch | 배리어 발사 노티파이를 받지 못했습니다."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_PenitentBarrierLaunch::HandleBarrierLaunchMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UPRGameplayAbility_PenitentBarrierLaunch::ExecuteBarrierLaunch()
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PenitentCharacter) || !PenitentCharacter->HasAuthority())
	{
		return false;
	}

	APRGroundBoxProjectileBase* BarrierActor = PenitentCharacter->GetSpawnedBarrierActor();
	if (!IsValid(BarrierActor))
	{
		// 무효 배리어 상태 정리
		PenitentCharacter->CleanupSummonedBarrier(false);
		return false;
	}

	// 노티파이 프레임 기준 배리어 발사 상태 확정
	if (!IsValid(BarrierData) || BarrierData->LaunchSpeed <= 0.0f)
	{
		return false;
	}

	BarrierActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	BarrierActor->LaunchGroundBoxProjectile(PenitentCharacter->GetActorForwardVector(), BarrierData->LaunchSpeed);
	PenitentCharacter->CleanupSummonedBarrier(false);
	return true;
}
