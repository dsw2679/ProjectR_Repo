// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 돌진 어빌리티 구현)
#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Sprint.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

UPRGA_Sprint::UPRGA_Sprint()
{
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Sprint);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Sprinting);
	InputTag = PRGameplayTags::Input_Ability_Sprint;

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Aiming);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Crouching);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetCharacterMovement()))
	{
		// OptionalRelevantTags는 실패 사유 기록용 출력 포인터라 전달된 경우에만 태그를 추가한다.
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	if (PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		// 이동 입력이 없는 상태에서는 질주를 시작하지 않고 실패 사유만 기록한다.
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	if (!HasEnoughStaminaToSprint(ActorInfo))
	{
		// 스태미너가 질주 종료 임계값 이하라면 비용 부족으로 취급한다.
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cost);
		}
		return false;
	}

	return true;
}

void UPRGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartSprint();
}

void UPRGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnregisterStaminaChangeDelegate();
	StopMovementInputCheck();

	if (bSprintStarted)
	{
		// 실제 질주가 시작된 경우에만 회복 딜레이를 부여한다.
		ApplyStaminaRegenDelay();
	}
	bSprintStarted = false;

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayerCharacter->SetSprintingFromAbility(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Sprint::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Sprint::StartSprint()
{
	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		// Avatar가 플레이어 캐릭터가 아니면 질주 상태를 반영할 대상이 없으므로 Ability를 취소한다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayerCharacter->SetSprintingFromAbility(true);
	bSprintStarted = true;
	RegisterStaminaChangeDelegate();
	StartMovementInputCheck();

	if (!HasEnoughStaminaToSprint(CurrentActorInfo))
	{
		// 시작 직후 스태미너가 이미 고갈된 경우 토글 질주가 남지 않도록 즉시 종료한다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UPRGA_Sprint::ApplyStaminaRegenDelay()
{
	if (!IsValid(StaminaRegenDelayEffectClass))
	{
		// BP에서 회복 딜레이 GE가 할당되지 않은 경우에는 딜레이 적용 없이 종료한다.
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(StaminaRegenDelayEffectClass);
	if (SpecHandle.IsValid())
	{
		// 유효한 Spec이 만들어진 경우에만 Owner ASC에 회복 딜레이 GE를 적용한다.
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}

bool UPRGA_Sprint::HasEnoughStaminaToSprint(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo == nullptr || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		// ActorInfo는 구조체 포인터이고 ASC는 약참조라 각각 null과 핸들 유효성을 확인한다.
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		// ASC UObject가 이미 파괴되었거나 유효하지 않으면 Attribute를 읽을 수 없다.
		return false;
	}

	const float CurrentStamina = ASC->GetNumericAttribute(UPRAttributeSet_Player::GetStaminaAttribute());
	return CurrentStamina > SprintStaminaEndThreshold;
}

void UPRGA_Sprint::RegisterStaminaChangeDelegate()
{
	if (StaminaChangedDelegateHandle.IsValid())
	{
		// 이미 콜백이 등록되어 있으면 중복 등록으로 EndAbility가 여러 번 호출될 수 있다.
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		// ASC가 없으면 스태미너 변경 이벤트를 구독할 수 없다.
		return;
	}

	StaminaChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Player::GetStaminaAttribute()).AddUObject(this, &UPRGA_Sprint::HandleStaminaChanged);
}

void UPRGA_Sprint::UnregisterStaminaChangeDelegate()
{
	if (!StaminaChangedDelegateHandle.IsValid())
	{
		// 등록된 콜백이 없으면 해제할 대상도 없다.
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		// Ability 종료 후 늦게 도착한 Attribute 변경 콜백이 접근하지 않도록 바인딩을 해제한다.
		ASC->GetGameplayAttributeValueChangeDelegate(
			UPRAttributeSet_Player::GetStaminaAttribute()).Remove(StaminaChangedDelegateHandle);
	}

	StaminaChangedDelegateHandle.Reset();
}

void UPRGA_Sprint::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!IsActive() || !bSprintStarted)
	{
		// Ability가 이미 끝났거나 질주가 실제 시작되지 않은 상태의 콜백은 무시한다.
		return;
	}

	if (ChangeData.NewValue <= SprintStaminaEndThreshold)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

/*~ 이동 입력 감시 ~*/

void UPRGA_Sprint::StartMovementInputCheck()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(MovementInputCheckTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		MovementInputCheckTimerHandle,
		this,
		&UPRGA_Sprint::HandleMovementInputCheck,
		MovementInputCheckInterval,
		true);
}

void UPRGA_Sprint::StopMovementInputCheck()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(MovementInputCheckTimerHandle);
	}
	MovementInputCheckTimerHandle.Invalidate();
}

void UPRGA_Sprint::HandleMovementInputCheck()
{
	if (!IsActive() || !bSprintStarted)
	{
		StopMovementInputCheck();
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetCharacterMovement()))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		// 토글 질주 중 방향 입력이 사라지면 Sprint 상태와 Ability를 함께 종료한다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
