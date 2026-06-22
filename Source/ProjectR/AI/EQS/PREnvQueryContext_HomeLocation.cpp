// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Env Query Context Home Location 구현)
#include "PREnvQueryContext_HomeLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	AAIController* ResolveHomeLocationAIControllerFromQueryOwner(UObject* QueryOwner)
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

	bool ResolveHomeLocationFromEnemyInterface(const AActor* OwnerActor, FVector& OutHomeLocation)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(OwnerActor);
		if (EnemyInterface == nullptr)
		{
			return false;
		}

		OutHomeLocation = EnemyInterface->GetHomeLocation();
		return true;
	}
}

void UPREnvQueryContext_HomeLocation::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	AAIController* AIController = ResolveHomeLocationAIControllerFromQueryOwner(QueryOwner);

	if (IsValid(AIController))
	{
		UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent();
		if (IsValid(BlackboardComponent)
			&& HomeLocationBlackboardKey != NAME_None
			&& BlackboardComponent->GetKeyID(HomeLocationBlackboardKey) != FBlackboard::InvalidKey
			&& BlackboardComponent->IsVectorValueSet(HomeLocationBlackboardKey))
		{
			UEnvQueryItemType_Point::SetContextHelper(
				ContextData,
				BlackboardComponent->GetValueAsVector(HomeLocationBlackboardKey));
			return;
		}

		FVector HomeLocation = FVector::ZeroVector;
		if (ResolveHomeLocationFromEnemyInterface(AIController->GetPawn(), HomeLocation))
		{
			UEnvQueryItemType_Point::SetContextHelper(ContextData, HomeLocation);
			return;
		}
	}

	FVector HomeLocation = FVector::ZeroVector;
	if (ResolveHomeLocationFromEnemyInterface(Cast<AActor>(QueryOwner), HomeLocation))
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, HomeLocation);
	}
}
