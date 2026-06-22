// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Run 페어린 보스 전투 Loop Step 비헤이비어 트리 태스크 구현)
#include "BTTask_PRRunFaerinCombatLoopStep.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"

UBTTask_PRRunFaerinCombatLoopStep::UBTTask_PRRunFaerinCombatLoopStep()
{
	NodeName = TEXT("Run Faerin Combat Loop Step");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_PRRunFaerinCombatLoopStep::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ClearActiveLoopBinding();

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(ControlledPawn))
	{
		return EBTNodeResult::Failed;
	}

	UPRFaerinCombatDirectorComponent* DirectorComponent =
		ControlledPawn->FindComponentByClass<UPRFaerinCombatDirectorComponent>();
	if (!IsValid(DirectorComponent))
	{
		return EBTNodeResult::Failed;
	}

	ActiveDirectorComponent = DirectorComponent;
	ActiveBehaviorTreeComponent = &OwnerComp;
	DirectorComponent->OnFaerinLoopStepFinished.AddDynamic(
		this,
		&UBTTask_PRRunFaerinCombatLoopStep::HandleLoopStepFinished);

	if (!DirectorComponent->RunNextLoopStep(OwnerComp))
	{
		ClearActiveLoopBinding();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_PRRunFaerinCombatLoopStep::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (IsValid(ActiveDirectorComponent))
	{
		ActiveDirectorComponent->CancelCurrentLoopStep();
	}

	ClearActiveLoopBinding();
	return EBTNodeResult::Aborted;
}

void UBTTask_PRRunFaerinCombatLoopStep::HandleLoopStepFinished(bool bSucceeded)
{
	UBehaviorTreeComponent* BehaviorTreeComponent = ActiveBehaviorTreeComponent.Get();
	const EBTNodeResult::Type TaskResult = bSucceeded
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;

	ClearActiveLoopBinding();

	if (IsValid(BehaviorTreeComponent))
	{
		FinishLatentTask(*BehaviorTreeComponent, TaskResult);
	}
}

void UBTTask_PRRunFaerinCombatLoopStep::ClearActiveLoopBinding()
{
	if (IsValid(ActiveDirectorComponent))
	{
		ActiveDirectorComponent->OnFaerinLoopStepFinished.RemoveDynamic(
			this,
			&UBTTask_PRRunFaerinCombatLoopStep::HandleLoopStepFinished);
	}

	ActiveDirectorComponent = nullptr;
	ActiveBehaviorTreeComponent = nullptr;
}

FString UBTTask_PRRunFaerinCombatLoopStep::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nFaerin CombatDirector의 한 루프 단계를 실행한다."),
		*Super::GetStaticDescription());
}
