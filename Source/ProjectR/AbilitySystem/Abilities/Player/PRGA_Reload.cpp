// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (피격 상태에 따른 재장전 캔슬 연동)
// Author: 배유찬 (재장전 시퀀스 및 좌수 IK 보정 구현)
// Author: 이건주 (재장전 시 무기 자원 충전 연동 구현)
#include "PRGA_Reload.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

UPRGA_Reload::UPRGA_Reload()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Reload);
	SetAssetTags(DefaultAbilityTags);
	InputTag = PRGameplayTags::Input_Ability_Reload;
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_Reload;
	AbilityTriggers.Add(TriggerData);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UPRGA_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayTagContainer* SourceTags,
                                       const FGameplayTagContainer* TargetTags,
                                       FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager(ActorInfo))
	{
		if (WeaponManager->GetArmedState() == EPRArmedState::Unarmed)
		{
			return false;
		}
	}

	EPRAmmoType AmmoType;
	if (!TryGetActiveAmmoType(AmmoType))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UPRAttributeSet_Weapon* WeaponSet = IsValid(ASC) ? ASC->GetSet<UPRAttributeSet_Weapon>() : nullptr;
	if (!IsValid(WeaponSet))
	{
		return false;
	}

	// 탄창이 이미 가득 찼거나 예비탄이 없으면 재장전 불가
	const float Magazine = WeaponSet->GetMagazineAmmoByType(AmmoType);
	const float MaxMagazine = WeaponSet->GetMaxMagazineAmmoByType(AmmoType);
	const float Reserve = WeaponSet->GetReserveAmmoByType(AmmoType);
	if (Magazine >= MaxMagazine - KINDA_SMALL_NUMBER)
	{
		return false;
	}
	if (Reserve <= 0.f)
	{
		return false;
	}

	return true;
}

void UPRGA_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UPRWeaponDataAsset* WeaponData = GetActiveWeaponData();
	if (!IsValid(WeaponData) || !IsValid(WeaponData->ReloadMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 재장전 몽타주 재생. ASC가 클라/서버 양측에서 재생을 처리한다
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		WeaponData->ReloadMontage,
		FMath::Max(WeaponData->ReloadMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_Reload::OnReloadMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_Reload::OnReloadMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_Reload::OnReloadMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_Reload::OnReloadMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();

	// 몽타주 노티파이가 SendGameplayEventToActor로 발행하는 자원 이동 트리거 대기
	ActiveCommitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRGameplayTags::Event_Player_ReloadCommit,
		nullptr,
		/*OnlyTriggerOnce=*/true,
		/*OnlyMatchExact=*/true);

	if (IsValid(ActiveCommitEventTask))
	{
		ActiveCommitEventTask->EventReceived.AddDynamic(this, &UPRGA_Reload::OnReloadCommitEvent);
		ActiveCommitEventTask->ReadyForActivation();
	}
}

void UPRGA_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle,
                              const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo,
                              bool bReplicateEndAbility,
                              bool bWasCancelled)
{
	if (bWasCancelled)
	{
		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager(ActorInfo))
		{
			WeaponManager->RequestWeaponAnimation(EPRWeaponAnimationState::Idle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Reload::OnReloadCommitEvent(FGameplayEventData EventData)
{
	// 자원 이동은 서버 권위에서만 수행. 클라이언트 예측 측 이벤트는 무시
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
	{
		ExecuteReload();
	}
}

void UPRGA_Reload::OnReloadMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UPRGA_Reload::OnReloadMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
}

void UPRGA_Reload::ExecuteReload()
{
	EPRAmmoType AmmoType;
	if (!TryGetActiveAmmoType(AmmoType))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> ReloadGE = (AmmoType == EPRAmmoType::Primary)
		? ReloadAmmoGE_Primary
		: ReloadAmmoGE_Secondary;
	if (!IsValid(ReloadGE))
	{
		return;
	}

	// 이동량은 Reload GE의 실행 계산이 슬롯 Magazine·MaxMagazine·Reserve를 캡처해 산출한다
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ReloadGE, GetAbilityLevel(), Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

UPRWeaponDataAsset* UPRGA_Reload::GetActiveWeaponData() const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return nullptr;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return nullptr;
	}

	return WeaponManager->GetWeaponDataBySlotType(WeaponManager->GetCurrentWeaponSlot());
}

bool UPRGA_Reload::TryGetActiveAmmoType(EPRAmmoType& OutAmmoType) const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return false;
	}

	const EPRWeaponSlotType ActiveSlot = WeaponManager->GetCurrentWeaponSlot();
	if (ActiveSlot == EPRWeaponSlotType::None)
	{
		return false;
	}

	OutAmmoType = (ActiveSlot == EPRWeaponSlotType::Primary) ? EPRAmmoType::Primary : EPRAmmoType::Secondary;
	return true;
}
