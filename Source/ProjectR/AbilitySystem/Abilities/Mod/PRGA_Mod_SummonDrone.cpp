// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Mod Summon Drone 구현)
#include "PRGA_Mod_SummonDrone.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/Drone/PRSupportDroneActor.h"
#include "ProjectR/Drone/PRSupportDroneDataAsset.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_Mod_SummonDrone::UPRGA_Mod_SummonDrone()
{
	PlayerAbilityType = EPRPlayerAbilityType::Mod;
	InputTag = PRGameplayTags::Input_Ability_Mod;
}

bool UPRGA_Mod_SummonDrone::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!IsValid(DroneClass))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	return true;
}

/*~ UPRGA_Mod_HasDuration Interface ~*/

void UPRGA_Mod_SummonDrone::OnDurationStarted_Implementation()
{
	Super::OnDurationStarted_Implementation();

	// 지속시간 비용 감시
	ActiveDurationCostHandle = GetLastAppliedModCostHandle();
	BindDurationCostRemovalEvent();

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!IsValid(DroneClass) || !IsValid(SpawnSupportDrone(GetCurrentActorInfo())))
	{
		CleanupDroneRuntime();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

/*~ UPRGA_Mod Interface ~*/

void UPRGA_Mod_SummonDrone::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 활성 드론 정리
	DestroyActiveDrone();
	CleanupDroneRuntime();
	Super::CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
}

/*~ 소환 ~*/

APRSupportDroneActor* UPRGA_Mod_SummonDrone::SpawnSupportDrone(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ActorInfo == nullptr || !IsValid(DroneClass))
	{
		return nullptr;
	}

	APawn* PlayerPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerPawn))
	{
		return nullptr;
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	if (bReplaceExistingDrone)
	{
		DestroyActiveDrone();
	}
	else if (IsValid(ActiveDrone))
	{
		return ActiveDrone;
	}

	const FVector SpawnLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorRotation().RotateVector(SpawnOffset);
	const FRotator SpawnRotation = PlayerPawn->GetActorRotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerPawn;
	SpawnParameters.Instigator = PlayerPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APRSupportDroneActor* SpawnedDrone = World->SpawnActor<APRSupportDroneActor>(
		DroneClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
	if (!IsValid(SpawnedDrone))
	{
		return nullptr;
	}

	SpawnedDrone->InitializeDrone(PlayerPawn, DroneData);
	// DataAsset 수명 타이머 제거
	SpawnedDrone->SetLifeSpan(0.0f);
	SpawnedDrone->StartDissolve(SpawnDissolveDuration, EPRDroneDissolveMode::Appear);

	ActiveDrone = SpawnedDrone;
	return SpawnedDrone;
}

void UPRGA_Mod_SummonDrone::DestroyActiveDrone()
{
	if (!IsValid(ActiveDrone))
	{
		ActiveDrone = nullptr;
		return;
	}

	// 퇴장 Dissolve 요청
	ActiveDrone->StartDissolve(DestroyDissolveDuration, EPRDroneDissolveMode::Disappear);
	ActiveDrone = nullptr;
}

void UPRGA_Mod_SummonDrone::CleanupDroneRuntime()
{
	UnbindDurationCostRemovalEvent();
	ActiveDurationCostHandle.Invalidate();
	ActiveDrone = nullptr;
}

/*~ 이벤트 바인딩 ~*/

void UPRGA_Mod_SummonDrone::BindDurationCostRemovalEvent()
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

void UPRGA_Mod_SummonDrone::UnbindDurationCostRemovalEvent()
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

/*~ 이벤트 처리 ~*/

void UPRGA_Mod_SummonDrone::HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	(void)RemovalInfo;

	// 비용 종료
	DurationCostRemovedDelegateHandle.Reset();
	ActiveDurationCostHandle.Invalidate();

	DestroyActiveDrone();
	CleanupDroneRuntime();
}
