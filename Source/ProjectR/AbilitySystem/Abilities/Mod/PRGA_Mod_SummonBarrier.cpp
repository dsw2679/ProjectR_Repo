// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Mod Summon Barrier 구현)
#include "PRGA_Mod_SummonBarrier.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/AbilitySystem/Data/PRBarrierAbilityDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRBarrierAnchorActor.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"

UPRGA_Mod_SummonBarrier::UPRGA_Mod_SummonBarrier()
{
	PlayerAbilityType = EPRPlayerAbilityType::Mod;
	InputTag = PRGameplayTags::Input_Ability_Mod;
}

/*~ UGameplayAbility Interface ~*/

bool UPRGA_Mod_SummonBarrier::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (HasLaunchActivationWindow(ActorInfo))
	{
		// 발사 비용 우회
		return UGameplayAbility::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UPRGA_Mod_SummonBarrier::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (HasLaunchActivationWindow(ActorInfo))
	{
		// 발사 추가 비용 없음
		return;
	}

	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Mod_SummonBarrier::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (HasLaunchActivationWindow(ActorInfo))
	{
		// 발사 경로
		UPRGA_Mod::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

		if (!HasAuthority(&ActivationInfo))
		{
			bLaunchRequested = true;
			UnbindDurationCostRemovalEvent();
			ActiveDurationCostHandle.Invalidate();
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		const bool bLaunched = LaunchActiveBarrier();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, !bLaunched);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

/*~ UPRGA_Mod_HasDuration Interface ~*/

void UPRGA_Mod_SummonBarrier::OnDurationStarted_Implementation()
{
	Super::OnDurationStarted_Implementation();

	ActiveDurationCostHandle = GetLastAppliedModCostHandle();
	BindDurationCostRemovalEvent();

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!IsValid(BarrierData) || !IsValid(SpawnBarrier(GetCurrentActorInfo())))
	{
		CleanupBarrierRuntime();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	BindSurvivalTagEvents();
}

/*~ UPRGA_Mod Interface ~*/

void UPRGA_Mod_SummonBarrier::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 활성 배리어 정리
	RequestActiveBarrierEnd();
	CleanupBarrierRuntime();
	Super::CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
}

/*~ 배리어 상태 ~*/

bool UPRGA_Mod_SummonBarrier::HasActiveBarrier() const
{
	return IsValid(ActiveBarrier) && !bLaunchRequested;
}

bool UPRGA_Mod_SummonBarrier::HasLaunchActivationWindow(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return !bLaunchRequested
		&& (HasActiveBarrier() || ActiveDurationCostHandle.IsValid() || HasReplicatedDurationCostState(ActorInfo));
}

bool UPRGA_Mod_SummonBarrier::HasReplicatedDurationCostState(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// 액터 정보 확인
	if (ActorInfo == nullptr)
	{
		return false;
	}

	// ASC 확인
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		return false;
	}

	// 무기 데이터 확인
	const UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData();
	if (!IsValid(WeaponData))
	{
		return false;
	}

	if (WeaponData->SlotType == EPRWeaponSlotType::Primary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	}

	if (WeaponData->SlotType == EPRWeaponSlotType::Secondary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	}

	return false;
}

APRGroundBoxProjectileBase* UPRGA_Mod_SummonBarrier::SpawnBarrier(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ActorInfo == nullptr
		|| !IsValid(BarrierData)
		|| !IsValid(BarrierData->BarrierActorClass)
		|| !IsValid(BarrierData->BarrierAnchorActorClass))
	{
		return nullptr;
	}

	APawn* PlayerPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerPawn) || !PlayerPawn->HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	RequestActiveBarrierEnd();
	bLaunchRequested = false;

	APRBarrierAnchorActor* SpawnedAnchor = SpawnBarrierAnchor(PlayerPawn);
	if (!IsValid(SpawnedAnchor))
	{
		return nullptr;
	}
	ActiveBarrierAnchor = SpawnedAnchor;

	USceneComponent* AttachComponent = SpawnedAnchor->GetBarrierAttachComponent();
	if (!IsValid(AttachComponent))
	{
		DestroyActiveBarrierAnchor();
		return nullptr;
	}

	const FName AttachSocketName = SpawnedAnchor->GetBarrierAttachSocketName();
	const FTransform SpawnTransform = AttachComponent->GetSocketTransform(AttachSocketName);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerPawn;
	SpawnParameters.Instigator = PlayerPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APRGroundBoxProjectileBase* SpawnedBarrier = World->SpawnActor<APRGroundBoxProjectileBase>(
		BarrierData->BarrierActorClass,
		SpawnTransform,
		SpawnParameters);
	if (!IsValid(SpawnedBarrier))
	{
		DestroyActiveBarrierAnchor();
		return nullptr;
	}
	FPRGroundBoxLaunchParams LaunchParams;
	LaunchParams.SourceActor = PlayerPawn;
	LaunchParams.DamageEffectSpec = MakeModEffectSpec(BarrierData->BarrierDamage, BarrierData->BarrierGroggyDamage);
	LaunchParams.OverrideMaxHealth = BarrierData->BarrierMaxHealth;
	LaunchParams.bUseGroundSnap = BarrierData->bUseGroundSnap;
	SpawnedBarrier->InitializeAttachedGroundBox(LaunchParams);
	SpawnedBarrier->AttachToComponent(
		AttachComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName);
	SpawnedBarrier->OnGroundBoxDestroyed.AddDynamic(this, &ThisClass::HandleBarrierDestroyed);

	ActiveBarrier = SpawnedBarrier;
	return SpawnedBarrier;
}

APRBarrierAnchorActor* UPRGA_Mod_SummonBarrier::SpawnBarrierAnchor(APawn* PlayerPawn)
{
	if (!IsValid(PlayerPawn) || !IsValid(BarrierData) || !IsValid(BarrierData->BarrierAnchorActorClass))
	{
		return nullptr;
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerPawn;
	SpawnParameters.Instigator = PlayerPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRBarrierAnchorActor* SpawnedAnchor = World->SpawnActor<APRBarrierAnchorActor>(
		BarrierData->BarrierAnchorActorClass,
		PlayerPawn->GetActorTransform(),
		SpawnParameters);
	if (!IsValid(SpawnedAnchor))
	{
		return nullptr;
	}

	// 소스 부착
	SpawnedAnchor->InitializeAnchor(PlayerPawn, BarrierData);
	return SpawnedAnchor;
}

bool UPRGA_Mod_SummonBarrier::LaunchActiveBarrier()
{
	if (!HasActiveBarrier())
	{
		return false;
	}

	APRGroundBoxProjectileBase* BarrierToLaunch = ActiveBarrier;
	const FVector LaunchDirection = BarrierToLaunch->GetActorForwardVector();
	if (!IsValid(BarrierData) || LaunchDirection.IsNearlyZero() || BarrierData->LaunchSpeed <= 0.0f)
	{
		return false;
	}

	bLaunchRequested = true;
	UnbindDurationCostRemovalEvent();
	UnbindSurvivalTagEvents();

	BarrierToLaunch->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleBarrierDestroyed);
	BarrierToLaunch->LaunchGroundBoxProjectile(LaunchDirection, BarrierData->LaunchSpeed);

	ActiveBarrier = nullptr;
	DestroyActiveBarrierAnchor();
	return true;
}

void UPRGA_Mod_SummonBarrier::RequestActiveBarrierEnd()
{
	if (IsValid(ActiveBarrier) && ActiveBarrier->HasAuthority())
	{
		// 활성 배리어 제거
		ActiveBarrier->RequestGroundBoxEnd(EPRProjectileDestroyReason::Manual);
	}

	ActiveBarrier = nullptr;
	DestroyActiveBarrierAnchor();
	bLaunchRequested = false;
}

void UPRGA_Mod_SummonBarrier::DestroyActiveBarrierAnchor()
{
	if (IsValid(ActiveBarrierAnchor) && ActiveBarrierAnchor->HasAuthority())
	{
		// 활성 앵커 제거
		ActiveBarrierAnchor->Destroy();
	}

	ActiveBarrierAnchor = nullptr;
}

void UPRGA_Mod_SummonBarrier::CleanupBarrierRuntime()
{
	UnbindDurationCostRemovalEvent();
	UnbindSurvivalTagEvents();

	if (IsValid(ActiveBarrier))
	{
		ActiveBarrier->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleBarrierDestroyed);
	}

	ActiveDurationCostHandle.Invalidate();
	ActiveBarrier = nullptr;
	DestroyActiveBarrierAnchor();
	bLaunchRequested = false;
}

/*~ 이벤트 바인딩 ~*/

void UPRGA_Mod_SummonBarrier::BindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || DurationCostRemovedDelegateHandle.IsValid())
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		// 비용 종료 감시
		DurationCostRemovedDelegateHandle = RemovedDelegate->AddUObject(this, &ThisClass::HandleDurationCostRemoved);
	}
}

void UPRGA_Mod_SummonBarrier::UnbindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || !DurationCostRemovedDelegateHandle.IsValid())
	{
		DurationCostRemovedDelegateHandle.Reset();
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		RemovedDelegate->Remove(DurationCostRemovedDelegateHandle);
	}

	DurationCostRemovedDelegateHandle.Reset();
}

void UPRGA_Mod_SummonBarrier::BindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (!DeadTagEventHandle.IsValid())
	{
		// 사망 감시
		DeadTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::HandleSurvivalTagChanged);
	}

	if (!DownTagEventHandle.IsValid())
	{
		// 다운 감시
		DownTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::HandleSurvivalTagChanged);
	}
}

void UPRGA_Mod_SummonBarrier::UnbindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		DeadTagEventHandle.Reset();
		DownTagEventHandle.Reset();
		return;
	}

	if (DeadTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DeadTagEventHandle, PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
		DeadTagEventHandle.Reset();
	}

	if (DownTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DownTagEventHandle, PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved);
		DownTagEventHandle.Reset();
	}
}

/*~ 이벤트 처리 ~*/

void UPRGA_Mod_SummonBarrier::HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	// 비용 종료
	DurationCostRemovedDelegateHandle.Reset();
	ActiveDurationCostHandle.Invalidate();

	// 클라이언트 예측 GE 교체
	if (!HasAuthority(&CurrentActivationInfo) && HasReplicatedDurationCostState(GetCurrentActorInfo()))
	{
		return;
	}

	if (!bLaunchRequested)
	{
		RequestActiveBarrierEnd();
	}

	CleanupBarrierRuntime();
}

void UPRGA_Mod_SummonBarrier::HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	if (Tag == PRGameplayTags::State_Dead || Tag == PRGameplayTags::State_Down)
	{
		RequestActiveBarrierEnd();
		CleanupBarrierRuntime();
	}
}

void UPRGA_Mod_SummonBarrier::HandleBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier)
{
	if (DestroyedBarrier != ActiveBarrier)
	{
		return;
	}

	// 배리어 참조 정리
	CleanupBarrierRuntime();
}
