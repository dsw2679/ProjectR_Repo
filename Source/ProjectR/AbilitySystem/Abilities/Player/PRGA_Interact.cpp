// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Interact 어빌리티 구현)
#include "PRGA_Interact.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "ProjectR/AbilitySystem/Tasks/PRAbilityTask_FaceTarget.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"

UPRGA_Interact::UPRGA_Interact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
	SetAssetTags(DefaultAbilityTags);
	
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Block_Interaction);
	
	InputTag = PRGameplayTags::Input_Ability_Interact;
}

void UPRGA_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                     const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}
	
	if (!ActorInfo->IsLocallyControlledPlayer())
	{
		// 서버 인스턴스의 유지형 상호작용 시작 대기
		if (UPRInteractorComponent* Interactor = GetInteractorComponent(ActorInfo->PlayerController.Get()))
		{
			BindToSustainedStart(Interactor);
			if (Interactor->IsSustained())
			{
				BindToSustained(Interactor);
			}
		}
		return;
	}
	// 로컬: 상호작용 시도
	if (UPRInteractorComponent* Interactor = GetInteractorComponent(ActorInfo->PlayerController.Get()))
	{
		if (!Interactor->HasFocus())
		{
			K2_EndAbility();
			return;
		}
		Interactor->InteractFocused(); // TODO: Focused 자동 Interaction 대신 NPC의 경우 선택지 표시
		
		if (Interactor->IsSustained())
		{
			BindToSustained(Interactor);
		}
		else if (Interactor->IsHolding())
		{
			BindToHoldFinished(Interactor);
			WaitHoldRelease(Interactor);
		}
		else
		{
			K2_EndAbility();
		}
	}
}

void UPRGA_Interact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UPRInteractorComponent* Interactor = GetInteractorComponent(ActorInfo->PlayerController.Get());
	if (IsValid(Interactor))
	{
		if (ActorInfo->IsLocallyControlledPlayer())
		{
			if (Interactor->IsHolding())
			{
				Interactor->OnInteractionReleased();
			}
			if (Interactor->IsSustained())
			{
				Interactor->RequestEndInteract(bWasCancelled);
			}
		}
		Interactor->OnInteractionStart.RemoveAll(this);
		Interactor->OnInteractionEnd.RemoveAll(this);
		Interactor->OnHoldEnd.RemoveAll(this);
	}

	StopFaceTargetTask();
	WaitHoldReleaseTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Interact::InputPressed(const FGameplayAbilitySpecHandle Handle,
   const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (CanActivateAbility(Handle,ActorInfo,nullptr,nullptr,nullptr))
	{
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
	}
}

UPRInteractorComponent* UPRGA_Interact::GetInteractorComponent(AController* InController)
{
	// 캐시된 상호작용 컴포넌트 우선 반환
	if (CachedInteractorComponent.IsValid())
	{
		return CachedInteractorComponent.Get();
	}

	if (!IsValid(InController))
	{
		return nullptr;
	}

	CachedInteractorComponent = InController->FindComponentByClass<UPRInteractorComponent>();
	return CachedInteractorComponent.Get();
}

void UPRGA_Interact::BindToSustained(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}

	StartFaceTargetTask(Interactor);
	Interactor->OnInteractionEnd.RemoveAll(this);
	Interactor->OnInteractionEnd.AddWeakLambda(this, [this]()
	{
		K2_EndAbility();
	});
}

// 서버 권위 유지형 상호작용 시작 대기 바인딩
void UPRGA_Interact::BindToSustainedStart(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}

	Interactor->OnInteractionStart.RemoveAll(this);
	Interactor->OnInteractionStart.AddWeakLambda(this, [this]()
	{
		if (CachedInteractorComponent.IsValid())
		{
			BindToSustained(CachedInteractorComponent.Get());
		}
	});
}

void UPRGA_Interact::BindToHoldFinished(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	Interactor->OnHoldEnd.RemoveAll(this);
	Interactor->OnHoldEnd.AddWeakLambda(this, [this]()
	{
		if (CachedInteractorComponent.IsValid())
		{
			if (CachedInteractorComponent->IsSustained())
			{
				BindToSustained(CachedInteractorComponent.Get());
			}
			else
			{
				K2_EndAbility();
			}
		}
	});
}

void UPRGA_Interact::WaitHoldRelease(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	WaitHoldReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	WaitHoldReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitHoldReleaseTask->ReadyForActivation();
}

// 유지형 상호작용 대상 방향 회전 및 접근 Task 시작
void UPRGA_Interact::StartFaceTargetTask(UPRInteractorComponent* Interactor)
{
	StopFaceTargetTask();

	if (!IsValid(Interactor))
	{
		return;
	}

	UPRInteractableComponent* ActiveInteractableComponent = Interactor->GetActiveInteractableComponent();
	if (!IsValid(ActiveInteractableComponent))
	{
		// 클라이언트 선반영 단계에서 활성 상호작용 정보 복제 전 사용할 포커스 대상
		ActiveInteractableComponent = Interactor->GetFocusedComponent();
	}

	AActor* TargetActor = IsValid(ActiveInteractableComponent) ? ActiveInteractableComponent->GetOwner() : nullptr;
	if (!IsValid(TargetActor))
	{
		return;
	}

	ActiveFaceTargetTask = UPRAbilityTask_FaceTarget::FaceTarget(this,
		TargetActor,
		FaceTargetBlendTime,
		FaceTargetTaskDuration,
		FaceTargetAcceptanceDistance,
		FaceTargetMovementScale,
		FaceTargetObstacleBackoffDistance,
		FaceTargetApproachFanAngle,
		FaceTargetTraceCount);
	if (IsValid(ActiveFaceTargetTask))
	{
		ActiveFaceTargetTask->ReadyForActivation();
	}
}

// 유지형 상호작용 대상 방향 회전 및 접근 Task 종료
void UPRGA_Interact::StopFaceTargetTask()
{
	if (IsValid(ActiveFaceTargetTask))
	{
		ActiveFaceTargetTask->EndTask();
	}

	ActiveFaceTargetTask = nullptr;
}

void UPRGA_Interact::OnInputReleased(float TimeHeld)
{
	K2_EndAbility();
}
