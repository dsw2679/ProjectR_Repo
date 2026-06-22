// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 해제 시 블랙보드 정보 초기화 구현)
// Author: 손승우 (해머 전투 콤보 초기화용 블랙보드 갱신 구현)
#include "BTTask_PRClearBlackboardValue.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

namespace
{
	bool HasClearBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

UBTTask_PRClearBlackboardValue::UBTTask_PRClearBlackboardValue()
{
	NodeName = TEXT("Clear Blackboard Value");
}

EBTNodeResult::Type UBTTask_PRClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || BlackboardKey == NAME_None)
	{
		return EBTNodeResult::Failed;
	}

	if (!HasClearBlackboardKey(BlackboardComponent, BlackboardKey))
	{
		return bFailIfKeyMissing ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	BlackboardComponent->ClearValue(BlackboardKey);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRClearBlackboardValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nClear Key: %s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.ToString());
}
