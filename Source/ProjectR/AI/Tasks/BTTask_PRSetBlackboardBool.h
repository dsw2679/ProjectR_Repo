// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Set 블랙보드 Bool 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRSetBlackboardBool.generated.h"

// Blackboard Bool 키를 명시한 값으로 기록하는 공용 Task다.
UCLASS()
class PROJECTR_API UBTTask_PRSetBlackboardBool : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 에디터에서 선택한 Blackboard Bool 키와 기록 값을 초기화한다.
	UBTTask_PRSetBlackboardBool();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 값을 기록할 Blackboard Bool 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName BlackboardKey = NAME_None;

	// Blackboard 키에 기록할 Bool 값이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	bool bValue = true;

	// 키가 없을 때 실패로 처리할지 결정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	bool bFailIfKeyMissing = true;
};
