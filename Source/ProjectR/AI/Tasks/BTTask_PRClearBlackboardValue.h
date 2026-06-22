// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 해제 시 블랙보드 정보 초기화 구현)
// Author: 손승우 (해머 전투 콤보 초기화용 블랙보드 갱신 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRClearBlackboardValue.generated.h"

// Blackboard 키 값을 비우는 범용 Task
UCLASS()
class PROJECTR_API UBTTask_PRClearBlackboardValue : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 에디터에서 선택한 Blackboard 키를 ClearValue로 초기화
	UBTTask_PRClearBlackboardValue();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// ClearValue 대상 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName BlackboardKey = NAME_None;

	// 키 누락 시 실패 처리 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	bool bFailIfKeyMissing = true;
};
