// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Run 페어린 보스 전투 Loop Step 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRRunFaerinCombatLoopStep.generated.h"

class UBehaviorTreeComponent;
class UPRFaerinCombatDirectorComponent;

// Faerin CombatDirector에 원작형 공격 루프 한 단계를 요청하는 BT Task다.
UCLASS()
class PROJECTR_API UBTTask_PRRunFaerinCombatLoopStep : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRRunFaerinCombatLoopStep();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// CombatDirector가 루프 완료를 알렸을 때 BT Task를 종료한다.
	UFUNCTION()
	void HandleLoopStepFinished(bool bSucceeded);

	// 현재 바인딩된 Director와 BT 참조를 정리한다.
	void ClearActiveLoopBinding();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCombatDirectorComponent> ActiveDirectorComponent;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTreeComponent> ActiveBehaviorTreeComponent;
};
