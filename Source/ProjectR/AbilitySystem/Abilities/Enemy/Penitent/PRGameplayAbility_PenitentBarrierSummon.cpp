// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Barrier Summon 공격 패턴 어빌리티 구현)
#include "PRGameplayAbility_PenitentBarrierSummon.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/Data/PRBarrierAbilityDataAsset.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRBarrierAnchorActor.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"

UPRGameplayAbility_PenitentBarrierSummon::UPRGameplayAbility_PenitentBarrierSummon()
{
	FGameplayTagContainer BarrierSummonTags;
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_BarrierSummon);
	SetAssetTags(BarrierSummonTags);
}

bool UPRGameplayAbility_PenitentBarrierSummon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	if (IsValid(PenitentCharacter)
		&& PenitentCharacter->HasActiveBarrier()
		&& AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		return true;
	}

	return IsValid(BarrierData)
		&& IsValid(BarrierData->BarrierActorClass)
		&& IsValid(BarrierData->BarrierAnchorActorClass)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

void UPRGameplayAbility_PenitentBarrierSummon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (PenitentCharacter->HasActiveBarrier()
		&& AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		FGameplayAbilitySpecHandle LaunchAbilityHandle;
		UPRAbilitySystemComponent* PenitentAbilitySystemComponent = Cast<UPRAbilitySystemComponent>(AbilitySystemComponent);
		const bool bLaunchActivated = IsValid(PenitentAbilitySystemComponent)
			&& PenitentAbilitySystemComponent->TryActivateAbilityOnServer(
				PRGameplayTags::Ability_Enemy_Penitent_BarrierLaunch,
				LaunchAbilityHandle);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, !bLaunchActivated);
		return;
	}

	if (PenitentCharacter->HasActiveBarrier()
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(BarrierSummonMontage))
	{
		// 몽타주 누락으로 소환 중단
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | BarrierSummonMontage가 유효하지 않습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	constexpr bool bOnlyTriggerOnce = true;
	constexpr bool bOnlyMatchExact = true;
	ActiveBarrierEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_PenitentBarrierSummon,
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
		BarrierSummonMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		MontageStartSection);
	if (!IsValid(ActiveMontageTask))
	{
		// 몽타주 태스크 생성 실패
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bBarrierActionExecuted = false;
	ActiveBarrierEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleBarrierSummonGameplayEvent);
	ActiveBarrierEventTask->ReadyForActivation();

	ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleBarrierSummonMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleBarrierSummonMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleBarrierSummonMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleBarrierSummonMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_PenitentBarrierSummon::EndAbility(const FGameplayAbilitySpecHandle Handle,
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

void UPRGameplayAbility_PenitentBarrierSummon::HandleBarrierSummonGameplayEvent(FGameplayEventData Payload)
{
	if (bBarrierActionExecuted || Payload.EventTag != PRCombatGameplayTags::Event_Ability_PenitentBarrierSummon)
	{
		return;
	}

	bBarrierActionExecuted = true;
	if (!ExecuteBarrierSummon())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UPRGameplayAbility_PenitentBarrierSummon::HandleBarrierSummonMontageCompleted()
{
	if (!bBarrierActionExecuted)
	{
		// 소환 노티파이 누락 감지
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | 배리어 소환 노티파이를 받지 못했습니다."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_PenitentBarrierSummon::HandleBarrierSummonMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UPRGameplayAbility_PenitentBarrierSummon::ExecuteBarrierSummon()
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent)
		|| !IsValid(World))
	{
		return false;
	}

	if (PenitentCharacter->HasActiveBarrier()
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		return false;
	}

	if (!IsValid(BarrierData)
		|| !IsValid(BarrierData->BarrierActorClass)
		|| !IsValid(BarrierData->BarrierAnchorActorClass))
	{
		// 배리어 데이터 누락
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | 배리어 데이터가 유효하지 않습니다."));
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PenitentCharacter;
	SpawnParameters.Instigator = PenitentCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APRBarrierAnchorActor* BarrierAnchor = World->SpawnActor<APRBarrierAnchorActor>(
		BarrierData->BarrierAnchorActorClass,
		PenitentCharacter->GetActorTransform(),
		SpawnParameters);
	if (!IsValid(BarrierAnchor))
	{
		// 배리어 앵커 스폰 실패
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | 배리어 앵커가 정상적으로 스폰되지 않았습니다."));
		return false;
	}

	BarrierAnchor->InitializeAnchor(PenitentCharacter, BarrierData);
	USceneComponent* BarrierAttachComponent = BarrierAnchor->GetBarrierAttachComponent();
	if (!IsValid(BarrierAttachComponent))
	{
		// 배리어 부착 기준 누락
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | 배리어 부착 컴포넌트가 유효하지 않습니다."));
		BarrierAnchor->Destroy();
		return false;
	}

	const FName AttachSocketName = BarrierAnchor->GetBarrierAttachSocketName();
	const FTransform SpawnTransform = BarrierAttachComponent->GetSocketTransform(AttachSocketName);
	APRGroundBoxProjectileBase* BarrierActor = World->SpawnActor<APRGroundBoxProjectileBase>(
		BarrierData->BarrierActorClass,
		SpawnTransform,
		SpawnParameters);

	if (!IsValid(BarrierActor))
	{
		// 배리어 액터 스폰 실패
		UE_LOG(LogTemp, Warning, TEXT("UPRGameplayAbility_PenitentBarrierSummon | 배리어 액터가 정상적으로 스폰되지 않았습니다."));
		BarrierAnchor->Destroy();
		return false;
	}

	// 노티파이 프레임 기준 배리어 부착 상태 확정
	FPRGroundBoxLaunchParams LaunchParams;
	LaunchParams.SourceActor = PenitentCharacter;
	LaunchParams.OverrideMaxHealth = BarrierData->BarrierMaxHealth;
	LaunchParams.bUseGroundSnap = BarrierData->bUseGroundSnap;
	BarrierActor->InitializeAttachedGroundBox(LaunchParams);
	BarrierActor->AttachToComponent(
		BarrierAttachComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName);
	PenitentCharacter->SetSpawnedBarrierActor(BarrierActor, BarrierAnchor);
	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	return true;
}
