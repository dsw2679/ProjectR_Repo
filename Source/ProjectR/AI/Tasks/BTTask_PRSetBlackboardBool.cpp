// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Set 블랙보드 Bool 비헤이비어 트리 태스크 구현)
#include "BTTask_PRSetBlackboardBool.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

namespace
{
	bool HasSetBlackboardBoolKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

UBTTask_PRSetBlackboardBool::UBTTask_PRSetBlackboardBool()
{
	NodeName = TEXT("Set Blackboard Bool");
}

EBTNodeResult::Type UBTTask_PRSetBlackboardBool::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || BlackboardKey == NAME_None)
	{
		return EBTNodeResult::Failed;
	}

	if (!HasSetBlackboardBoolKey(BlackboardComponent, BlackboardKey))
	{
		return bFailIfKeyMissing ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	BlackboardComponent->SetValueAsBool(BlackboardKey, bValue);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSetBlackboardBool::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nSet Key: %s\nValue: %s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.ToString(),
		bValue ? TEXT("true") : TEXT("false"));
}
