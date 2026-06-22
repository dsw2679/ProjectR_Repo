// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Env Query Context Current 타겟 구현)
#include "PREnvQueryContext_CurrentTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

namespace
{
	AAIController* ResolveAIControllerFromQueryOwner(UObject* QueryOwner)
	{
		AActor* OwnerActor = Cast<AActor>(QueryOwner);
		if (AAIController* AIController = Cast<AAIController>(OwnerActor))
		{
			return AIController;
		}

		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		if (IsValid(OwnerPawn))
		{
			return Cast<AAIController>(OwnerPawn->GetController());
		}

		return nullptr;
	}
}

void UPREnvQueryContext_CurrentTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	AAIController* AIController = ResolveAIControllerFromQueryOwner(QueryOwner);
	if (!IsValid(AIController))
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorBlackboardKey));
	if (!IsValid(CurrentTarget))
	{
		return;
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, CurrentTarget);
}
