// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (사망 시 아이템/재화 드롭 시퀀스 연동 구현)
// Author: 배유찬 (보스 사망 시 게이트/장벽 비활성화 제어 구현)
// Author: 손승우 (몬스터 사망 디졸브 연출 및 사망 판정 공용화 구현)
#include "PRGameplayAbility_EnemyDeath.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/World/Drop/PRItemDropManagerSubsystem.h"

namespace
{
	// 사망 이벤트의 Instigator가 컨트롤러, 폰, 플레이어 스테이트, 투사체처럼 여러 형태로 올 수 있어 컨트롤러까지 역추적한다.
	AController* ResolveDropControllerFromActor(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return nullptr;
		}

		if (AController* Controller = Cast<AController>(Actor))
		{
			return Controller;
		}

		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			return PlayerState->GetPlayerController();
		}

		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetController();
		}

		return ResolveDropControllerFromActor(Actor->GetOwner());
	}

	// 데미지 GE Context는 공격 경로에 따라 Instigator, OriginalInstigator, EffectCauser 중 다른 곳에 플레이어 출처가 들어갈 수 있다.
	AController* ResolveDropControllerFromEffectContext(const FGameplayEffectContextHandle& ContextHandle)
	{
		if (AController* Controller = ResolveDropControllerFromActor(ContextHandle.GetInstigator()))
		{
			return Controller;
		}

		if (AController* Controller = ResolveDropControllerFromActor(ContextHandle.GetOriginalInstigator()))
		{
			return Controller;
		}

		if (AController* Controller = ResolveDropControllerFromActor(ContextHandle.GetEffectCauser()))
		{
			return Controller;
		}

		const UAbilitySystemComponent* InstigatorASC = ContextHandle.GetInstigatorAbilitySystemComponent();
		if (IsValid(InstigatorASC))
		{
			if (AController* Controller = ResolveDropControllerFromActor(InstigatorASC->GetOwnerActor()))
			{
				return Controller;
			}

			return ResolveDropControllerFromActor(InstigatorASC->GetAvatarActor());
		}

		return nullptr;
	}
}

UPRGameplayAbility_EnemyDeath::UPRGameplayAbility_EnemyDeath()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(TriggerData);

	// 사망 Ability 활성화 시 진행 중인 적/보스 패턴 Ability를 모두 중단하고 새 패턴 진입을 막는다.
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dead);
}

void UPRGameplayAbility_EnemyDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bDeathFinished = false;

	RequestMonsterDrop(TriggerEventData);

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (bDisableMovementOnDeath)
		{
			// 사망 이후 AI, Root Motion, 관성 이동이 다시 캐릭터를 움직이지 못하게 잠근다.
			Character->GetCharacterMovement()->DisableMovement();
		}
		else
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}

		if (UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent())
		{
			// 사망 연출 중 길막이나 추가 피격 판정이 남지 않도록 Pawn 충돌을 끈다.
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (IsValid(DeathMontage))
	{
		// 사망 몽타주는 다른 공격/그로기 몽타주와 동일하게 GAS 몽타주 복제 경로만 사용한다.
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DeathMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	if (bUseDissolveOnDeath)
	{
		RequestDeathDissolveVisual();
		ScheduleDeathDestroy(CalculateDeathDestroyDelay());
	}
	else
	{
		ScheduleDeathDestroy(DeathDestroyDelay);
	}
}

void UPRGameplayAbility_EnemyDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDestroyTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted()
{
	if (!bUseDissolveOnDeath && bEndAbilityWhenMontageEnds)
	{
		FinishDeath();
	}
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted()
{
	if (!bUseDissolveOnDeath && bEndAbilityWhenMontageEnds)
	{
		FinishDeath();
	}
}

void UPRGameplayAbility_EnemyDeath::DestroyDeathAvatar()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FinishDeath();
	AvatarActor->Destroy();
}

void UPRGameplayAbility_EnemyDeath::ScheduleDeathDestroy(float Delay)
{
	if (!bDestroyActorOnDeath)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDestroyTimerHandle,
			this,
			&UPRGameplayAbility_EnemyDeath::DestroyDeathAvatar,
			FMath::Max(Delay, 0.0f),
			false);
	}
	else
	{
		DestroyDeathAvatar();
	}
}

void UPRGameplayAbility_EnemyDeath::RequestDeathDissolveVisual()
{
	APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	EnemyCharacter->RequestDeathDissolveVisual(
		IsValid(ActiveMontageTask) ? DeathMontage.Get() : nullptr,
		MontagePlayRate,
		DissolveTimingMode == EPRDeathDissolveTimingMode::AfterMontageEnd,
		DissolveTimingMode == EPRDeathDissolveTimingMode::AfterMontageEnd
			? DissolveDelayAfterMontage
			: DissolveDelayAfterMontageStart,
		DissolveDuration,
		DissolveStartValue,
		DissolveEndValue,
		DissolveScalarParameterName,
		DissolveNiagaraSystem,
		NiagaraDissolveParameterName,
		DissolveTexture,
		DissolveTextureUV,
		DissolveTickInterval);
}

void UPRGameplayAbility_EnemyDeath::RequestMonsterDrop(const FGameplayEventData* TriggerEventData)
{
	APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(EnemyCharacter) || !EnemyCharacter->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPRItemDropManagerSubsystem* DropManager = World->GetSubsystem<UPRItemDropManagerSubsystem>();
	if (!IsValid(DropManager))
	{
		return;
	}

	AController* KillerController = nullptr;
	if (TriggerEventData != nullptr)
	{
		// GameplayEvent의 직접 Instigator를 먼저 사용하고, 비어 있으면 GE Context 안의 공격 출처를 순차적으로 확인한다.
		AActor* InstigatorActor = const_cast<AActor*>(TriggerEventData->Instigator.Get());
		KillerController = ResolveDropControllerFromActor(InstigatorActor);
		if (!IsValid(KillerController))
		{
			KillerController = ResolveDropControllerFromEffectContext(TriggerEventData->ContextHandle);
		}
	}

	if (!IsValid(KillerController))
	{
		// 일부 공격 경로에서 이벤트 Context가 비어 있을 수 있으므로, 마지막으로 적이 기억하는 현재 타겟을 개인 보상 대상으로 사용한다.
		if (UPREnemyThreatComponent* ThreatComponent = EnemyCharacter->GetEnemyThreatComponent())
		{
			KillerController = ResolveDropControllerFromActor(ThreatComponent->GetCurrentTarget());
		}
	}

	FPRMonsterDeathDropRequest DropRequest;
	DropRequest.MonsterId = EnemyCharacter->GetMonsterId();
	DropRequest.DeadMonster = EnemyCharacter;
	DropRequest.KillerController = KillerController;
	DropRequest.DropLocation = EnemyCharacter->GetActorLocation();

	DropManager->HandleMonsterDied(DropRequest);
}

float UPRGameplayAbility_EnemyDeath::CalculateDeathDestroyDelay() const
{
	if (!bUseDissolveOnDeath)
	{
		return DeathDestroyDelay;
	}

	const bool bDelayAfterMontageEnd = DissolveTimingMode == EPRDeathDissolveTimingMode::AfterMontageEnd;
	const float MontageDelay = bDelayAfterMontageEnd && IsValid(DeathMontage) && IsValid(ActiveMontageTask)
		? (DeathMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)) + 0.1f
		: 0.0f;
	const float StartDelay = bDelayAfterMontageEnd
		? FMath::Max(DissolveDelayAfterMontage, 0.0f)
		: FMath::Max(DissolveDelayAfterMontageStart, 0.0f);
	const float VisualDuration = FMath::Max(DissolveDuration, 0.0f);

	return MontageDelay + StartDelay + VisualDuration;
}

void UPRGameplayAbility_EnemyDeath::FinishDeath()
{
	if (bDeathFinished)
	{
		return;
	}

	bDeathFinished = true;
}
