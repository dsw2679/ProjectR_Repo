// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 반응 어빌리티 강제 실행 연동)
// Author: 손승우 (아머드 솔저/페어린 AI 어빌리티 실행 및 그로기/사망 캔슬 제어)
// Author: 이건주 (Penitent 특수 어빌리티 실행 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRActivateEnemyAbility.generated.h"

class UBehaviorTreeComponent;
class UPRAbilitySystemComponent;
struct FAbilityEndedData;

// BT에서 선택된 Gameplay Ability를 서버 ASC에 실행 요청하는 Task다.
// 고정 AbilityTag를 쓰거나 Blackboard의 selected_ability_tag 값을 읽어 실행한다.
UCLASS()
class PROJECTR_API UBTTask_PRActivateEnemyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRActivateEnemyAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 고정 Ability를 실행하고 싶을 때 지정한다. 비어 있으면 Blackboard 값을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (Categories = "Ability.Enemy,Ability.Boss"))
	FGameplayTag AbilityTag;

	// SelectEnemyPattern Task가 기록한 AbilityTag 이름을 읽는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName AbilityTagBlackboardKey = TEXT("selected_ability_tag");

	// 전술 상태를 저장하는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 전투 표현 적용 시 참조할 현재 타겟 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 주변 Alert 전파로 깨어난 몬스터가 공용 Alert Ability를 건너뛰기 위해 확인하는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName NearbyAIAlertedKey = TEXT("nearby_ai_alerted");

	// true면 실제 Ability 활성화 성공 시점에 전술 모드를 공격 상태로 전환한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bSetTacticalModeOnAbilityActivated = true;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bSetTacticalModeOnAbilityActivated"))
	EPRTacticalMode TacticalModeOnAbilityActivated = EPRTacticalMode::Attack;

	// true면 Ability가 끝날 때까지 BT 실행을 InProgress로 유지한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWaitUntilAbilityEnds = true;

	// Abort 요청이 와도 Ability 종료까지 BT 전환 지연
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bWaitUntilAbilityEnds"))
	bool bDelayAbortUntilAbilityEnds = true;

	// true면 Ability가 끝난 뒤 전술 상태를 지정한 값으로 되돌린다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bSetTacticalModeAfterAbilityEnds = false;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bSetTacticalModeAfterAbilityEnds"))
	EPRTacticalMode TacticalModeAfterAbilityEnds = EPRTacticalMode::FastApproach;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bResetAttackPressureOnAbilityActivated = true;

	// true면 활성화 성공한 AbilityTag를 Blackboard에 기록한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWriteActivatedAbilityTagToBlackboard = false;

	// 활성화 성공한 AbilityTag 이름을 저장할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bWriteActivatedAbilityTagToBlackboard"))
	FName ActivatedAbilityTagWriteKey = TEXT("last_used_ranged_ability");

	// Ability 종료 후 다음 패턴 재평가 전까지 유지할 회복 시간이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (ClampMin = "0.0", EditCondition = "bWaitUntilAbilityEnds"))
	float PostAbilityEndDelay = 0.0f;

private:
	bool ShouldSkipAlertAbilityForSharedAlert(UBehaviorTreeComponent& OwnerComp, const FGameplayTag& ResolvedAbilityTag) const;
	void ApplySharedAlertSkippedState(UBehaviorTreeComponent& OwnerComp);
	void ApplyTacticalModeOnAbilityActivated(UBehaviorTreeComponent& OwnerComp);
	void ApplyPostAbilityCombatStateUpdates(UBehaviorTreeComponent& OwnerComp);
	void BindAbilityEndDelegate(UBehaviorTreeComponent& OwnerComp, UPRAbilitySystemComponent* ASC);
	void ClearAbilityEndDelegate();
	void HandleObservedAbilityEnded(const FAbilityEndedData& EndedData);
	bool IsObservedAbilityActive() const;
	void FinishObservedAbilityWait(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult);
	bool StartPostAbilityEndDelay(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult);
	void ClearPostAbilityEndDelay();
	void HandlePostAbilityEndDelayElapsed();

	// 대기 중인 Ability가 끝났을 때 BT를 바로 깨우기 위해 캐시한다.
	UPROPERTY()
	TObjectPtr<UPRAbilitySystemComponent> ActiveAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> ActiveBehaviorTreeComponent;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
	FTimerHandle PostAbilityEndDelayTimerHandle;
	EBTNodeResult::Type PendingPostAbilityTaskResult = EBTNodeResult::Succeeded;
	bool bAbortRequested = false;
	bool bWaitingPostAbilityEndDelay = false;
};
