// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 반응 어빌리티 강제 실행 연동)
// Author: 손승우 (아머드 솔저/페어린 AI 어빌리티 실행 및 그로기/사망 캔슬 제어)
// Author: 이건주 (Penitent 특수 어빌리티 실행 연동 구현)
#include "BTTask_PRActivateEnemyAbility.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	bool HasActivateAbilityBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool IsDisabledPlayerTarget(const AActor* Target)
	{
		if (!IsValid(Target) || UPRCombatStatics::GetActorTeam(Target) != EPRTeam::Player)
		{
			return false;
		}

		const UAbilitySystemComponent* TargetAbilitySystem = UPRCombatStatics::FindAbilitySystemComponent(Target);
		return IsValid(TargetAbilitySystem)
			&& (TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Down)
				|| TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dead));
	}
}

UBTTask_PRActivateEnemyAbility::UBTTask_PRActivateEnemyAbility()
{
	NodeName = TEXT("Activate Enemy Ability");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_PRActivateEnemyAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ClearPostAbilityEndDelay();
	ClearAbilityEndDelegate();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	bAbortRequested = false;

	FGameplayTag ResolvedAbilityTag = AbilityTag;
	if (!ResolvedAbilityTag.IsValid() && AbilityTagBlackboardKey != NAME_None)
	{
		// 패턴 선택 Task는 Blackboard에 태그 이름만 기록하므로, 여기에서 실제 GameplayTag로 복원한다.
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			const FName AbilityTagName = BlackboardComponent->GetValueAsName(AbilityTagBlackboardKey);
			if (AbilityTagName != NAME_None)
			{
				ResolvedAbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
			}
		}
	}

	if (!ResolvedAbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	if (ShouldSkipAlertAbilityForSharedAlert(OwnerComp, ResolvedAbilityTag))
	{
		ApplySharedAlertSkippedState(OwnerComp);
		return EBTNodeResult::Succeeded;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (EnemyInterface == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UPRAbilitySystemComponent* ASC = EnemyInterface->GetEnemyAbilitySystemComponent();
	if (!IsValid(ASC) || !ControlledPawn->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	if (ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| ASC->HasMatchingGameplayTag(PRGameplayTags::State_Groggy))
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Verbose,
				TEXT("[Ability] BlockedByDisabledState Tag=%s Pawn=%s Dead=%s Groggy=%s"),
				*ResolvedAbilityTag.ToString(),
				*GetNameSafe(ControlledPawn),
				ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead) ? TEXT("true") : TEXT("false"),
				ASC->HasMatchingGameplayTag(PRGameplayTags::State_Groggy) ? TEXT("true") : TEXT("false"));
		}
		return EBTNodeResult::Failed;
	}

	if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
	{
		AActor* CurrentTarget = HasActivateAbilityBlackboardKey(BlackboardComponent, CurrentTargetKey)
			? Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey))
			: nullptr;
		if (IsDisabledPlayerTarget(CurrentTarget))
		{
			if (PREnemyAIDebug::IsPatternLogEnabled())
			{
				UE_LOG(
					LogPREnemyAI,
					Verbose,
					TEXT("[Ability] BlockedByDisabledTarget Tag=%s Pawn=%s Target=%s"),
					*ResolvedAbilityTag.ToString(),
					*GetNameSafe(ControlledPawn),
					*GetNameSafe(CurrentTarget));
			}
			return EBTNodeResult::Failed;
		}
	}

	// 몬스터 Ability 실행은 서버 ASC에서만 시도한다.
	if (!ASC->TryActivateAbilityOnServer(ResolvedAbilityTag, ActiveAbilityHandle))
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Warning,
				TEXT("[Ability] ActivateFailed Tag=%s Pawn=%s"),
				*ResolvedAbilityTag.ToString(),
				*GetNameSafe(ControlledPawn));
		}
		return EBTNodeResult::Failed;
	}

	ApplyTacticalModeOnAbilityActivated(OwnerComp);

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Ability] Activated Tag=%s Pawn=%s"),
			*ResolvedAbilityTag.ToString(),
			*GetNameSafe(ControlledPawn));
	}

	if (bWriteActivatedAbilityTagToBlackboard)
	{
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			if (HasActivateAbilityBlackboardKey(BlackboardComponent, ActivatedAbilityTagWriteKey))
			{
				// 마지막 실행 원거리 Ability 기록
				BlackboardComponent->SetValueAsName(ActivatedAbilityTagWriteKey, ResolvedAbilityTag.GetTagName());
			}
		}
	}

	if (bResetAttackPressureOnAbilityActivated)
	{
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			if (HasActivateAbilityBlackboardKey(BlackboardComponent, AttackPressureKey))
			{
				const float PreviousPressure = BlackboardComponent->GetValueAsFloat(AttackPressureKey);
				BlackboardComponent->SetValueAsFloat(AttackPressureKey, 0.0f);
				if (PREnemyAIDebug::IsAttackPressureLogEnabled())
				{
					UE_LOG(
						LogPREnemyAI,
						Verbose,
						TEXT("[AttackPressure] Reset Reason=AbilityActivated Prev=%.2f Pawn=%s"),
						PreviousPressure,
						*GetNameSafe(ControlledPawn));
				}
			}
		}
	}

	if (!bWaitUntilAbilityEnds)
	{
		ApplyPostAbilityCombatStateUpdates(OwnerComp);
		return EBTNodeResult::Succeeded;
	}

	BindAbilityEndDelegate(OwnerComp, ASC);

	// 즉시 종료되는 Ability는 델리게이트를 기다리지 않고 현재 Spec 상태로 바로 완료 처리한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec != nullptr && ActiveSpec->IsActive())
	{
		return EBTNodeResult::InProgress;
	}

	ClearAbilityEndDelegate();
	ApplyPostAbilityCombatStateUpdates(OwnerComp);
	if (StartPostAbilityEndDelay(OwnerComp, EBTNodeResult::Succeeded))
	{
		return EBTNodeResult::InProgress;
	}
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_PRActivateEnemyAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (bWaitingPostAbilityEndDelay)
	{
		ClearPostAbilityEndDelay();
		ClearAbilityEndDelegate();
		return EBTNodeResult::Aborted;
	}

	if (bDelayAbortUntilAbilityEnds && IsObservedAbilityActive())
	{
		bAbortRequested = true;
		return EBTNodeResult::InProgress;
	}

	ClearAbilityEndDelegate();
	return EBTNodeResult::Aborted;
}

void UBTTask_PRActivateEnemyAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (bWaitingPostAbilityEndDelay)
	{
		return;
	}

	if (!IsValid(ActiveAbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		FinishObservedAbilityWait(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 델리게이트가 정상 호출되지 않는 예외 상황을 대비한 안전망이다.
	const FGameplayAbilitySpec* ActiveSpec = ActiveAbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec == nullptr || !ActiveSpec->IsActive())
	{
		FinishObservedAbilityWait(OwnerComp, EBTNodeResult::Succeeded);
	}
}

bool UBTTask_PRActivateEnemyAbility::IsObservedAbilityActive() const
{
	if (!IsValid(ActiveAbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpec* ActiveSpec = ActiveAbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	return ActiveSpec != nullptr && ActiveSpec->IsActive();
}

bool UBTTask_PRActivateEnemyAbility::ShouldSkipAlertAbilityForSharedAlert(
	UBehaviorTreeComponent& OwnerComp,
	const FGameplayTag& ResolvedAbilityTag) const
{
	if (!ResolvedAbilityTag.MatchesTagExact(PRGameplayTags::Ability_Enemy_Alert))
	{
		return false;
	}

	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!HasActivateAbilityBlackboardKey(BlackboardComponent, NearbyAIAlertedKey))
	{
		return false;
	}

	return BlackboardComponent->GetValueAsBool(NearbyAIAlertedKey);
}

void UBTTask_PRActivateEnemyAbility::ApplySharedAlertSkippedState(UBehaviorTreeComponent& OwnerComp)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* FocusTarget = nullptr;
	if (HasActivateAbilityBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		FocusTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	const EPRTacticalMode NextTacticalMode = bSetTacticalModeAfterAbilityEnds
		? TacticalModeAfterAbilityEnds
		: EPRTacticalMode::FastApproach;

	if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
	{
		EnemyAIController->ApplyTacticalModeState(NextTacticalMode, FocusTarget);
		return;
	}

	if (HasActivateAbilityBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NextTacticalMode));
	}
}

void UBTTask_PRActivateEnemyAbility::FinishObservedAbilityWait(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult)
{
	ClearAbilityEndDelegate();
	ApplyPostAbilityCombatStateUpdates(OwnerComp);

	if (bAbortRequested)
	{
		bAbortRequested = false;
		FinishLatentAbort(OwnerComp);
		return;
	}

	if (TaskResult == EBTNodeResult::Succeeded && StartPostAbilityEndDelay(OwnerComp, TaskResult))
	{
		return;
	}

	FinishLatentTask(OwnerComp, TaskResult);
}

bool UBTTask_PRActivateEnemyAbility::StartPostAbilityEndDelay(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult)
{
	if (PostAbilityEndDelay <= 0.0f)
	{
		return false;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	ActiveBehaviorTreeComponent = &OwnerComp;
	PendingPostAbilityTaskResult = TaskResult;
	bWaitingPostAbilityEndDelay = true;

	World->GetTimerManager().SetTimer(
		PostAbilityEndDelayTimerHandle,
		this,
		&UBTTask_PRActivateEnemyAbility::HandlePostAbilityEndDelayElapsed,
		PostAbilityEndDelay,
		false);

	return true;
}

void UBTTask_PRActivateEnemyAbility::ClearPostAbilityEndDelay()
{
	if (!bWaitingPostAbilityEndDelay)
	{
		return;
	}

	if (UBehaviorTreeComponent* OwnerComp = ActiveBehaviorTreeComponent.Get())
	{
		if (UWorld* World = OwnerComp->GetWorld())
		{
			World->GetTimerManager().ClearTimer(PostAbilityEndDelayTimerHandle);
		}
	}

	PostAbilityEndDelayTimerHandle.Invalidate();
	PendingPostAbilityTaskResult = EBTNodeResult::Succeeded;
	bWaitingPostAbilityEndDelay = false;
	ActiveBehaviorTreeComponent = nullptr;
}

void UBTTask_PRActivateEnemyAbility::HandlePostAbilityEndDelayElapsed()
{
	UBehaviorTreeComponent* OwnerComp = ActiveBehaviorTreeComponent.Get();
	const EBTNodeResult::Type TaskResult = PendingPostAbilityTaskResult;
	const bool bShouldFinishAbort = bAbortRequested;

	ClearPostAbilityEndDelay();
	bAbortRequested = false;

	if (!IsValid(OwnerComp))
	{
		return;
	}

	if (bShouldFinishAbort)
	{
		FinishLatentAbort(*OwnerComp);
		return;
	}

	FinishLatentTask(*OwnerComp, TaskResult);
}

void UBTTask_PRActivateEnemyAbility::ApplyTacticalModeOnAbilityActivated(UBehaviorTreeComponent& OwnerComp)
{
	if (!bSetTacticalModeOnAbilityActivated)
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || !HasActivateAbilityBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		return;
	}

	AActor* FocusTarget = nullptr;
	if (HasActivateAbilityBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		FocusTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
	{
		EnemyAIController->ApplyTacticalModeState(TacticalModeOnAbilityActivated, FocusTarget);
	}
	else
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnAbilityActivated));
	}
}

void UBTTask_PRActivateEnemyAbility::ApplyPostAbilityCombatStateUpdates(UBehaviorTreeComponent& OwnerComp)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	if (bSetTacticalModeAfterAbilityEnds && HasActivateAbilityBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		AActor* FocusTarget = nullptr;
		if (HasActivateAbilityBlackboardKey(BlackboardComponent, CurrentTargetKey))
		{
			FocusTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
		}

		if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
		{
			EnemyAIController->ApplyTacticalModeState(TacticalModeAfterAbilityEnds, FocusTarget);
		}
		else
		{
			BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeAfterAbilityEnds));
		}
	}
}

void UBTTask_PRActivateEnemyAbility::BindAbilityEndDelegate(UBehaviorTreeComponent& OwnerComp, UPRAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC))
	{
		return;
	}

	ActiveAbilitySystemComponent = ASC;
	ActiveBehaviorTreeComponent = &OwnerComp;

	UAbilitySystemComponent* BaseASC = ASC;
	AbilityEndedDelegateHandle = BaseASC->OnAbilityEnded.AddUObject(this, &UBTTask_PRActivateEnemyAbility::HandleObservedAbilityEnded);
}

void UBTTask_PRActivateEnemyAbility::ClearAbilityEndDelegate()
{
	if (AbilityEndedDelegateHandle.IsValid())
	{
		UAbilitySystemComponent* BaseASC = ActiveAbilitySystemComponent.Get();
		if (IsValid(BaseASC))
		{
			BaseASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		}

		AbilityEndedDelegateHandle = FDelegateHandle();
	}

	ActiveAbilitySystemComponent = nullptr;
	ActiveBehaviorTreeComponent = nullptr;
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
}

void UBTTask_PRActivateEnemyAbility::HandleObservedAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (!ActiveAbilityHandle.IsValid() || EndedData.AbilitySpecHandle != ActiveAbilityHandle)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = ActiveBehaviorTreeComponent.Get();
	if (!IsValid(OwnerComp))
	{
		ClearAbilityEndDelegate();
		return;
	}

	FinishObservedAbilityWait(*OwnerComp, EBTNodeResult::Succeeded);
}

FString UBTTask_PRActivateEnemyAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nAbilityTag: %s\nActivated Mode: %s\nWait: %s\nPost Mode: %s\nPost Delay: %.2f\nWrite Activated Tag: %s\nShared Alert Skip Key: %s"),
		*Super::GetStaticDescription(),
		AbilityTag.IsValid() ? *AbilityTag.ToString() : *FString::Printf(TEXT("BB:%s"), *AbilityTagBlackboardKey.ToString()),
		bSetTacticalModeOnAbilityActivated
			? *StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalModeOnAbilityActivated))
			: TEXT("None"),
		bWaitUntilAbilityEnds ? TEXT("true") : TEXT("false"),
		bSetTacticalModeAfterAbilityEnds
			? *StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalModeAfterAbilityEnds))
			: TEXT("None"),
		PostAbilityEndDelay,
		bWriteActivatedAbilityTagToBlackboard ? *ActivatedAbilityTagWriteKey.ToString() : TEXT("false"),
		*NearbyAIAlertedKey.ToString());
}
