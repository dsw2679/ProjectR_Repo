// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Env Query Context Home Location 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PREnvQueryContext_HomeLocation.generated.h"

struct FEnvQueryContextData;
struct FEnvQueryInstance;

// 비전투 순찰 EQS에서 스폰 기준 위치를 Center Context로 제공한다.
UCLASS()
class PROJECTR_API UPREnvQueryContext_HomeLocation : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	/*~ UEnvQueryContext Interface ~*/
	// Querier의 Blackboard home_location 값을 EQS Point Context로 제공한다.
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

protected:
	// 스폰 기준 위치를 읽을 Blackboard 키 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Blackboard")
	FName HomeLocationBlackboardKey = TEXT("home_location");
};
