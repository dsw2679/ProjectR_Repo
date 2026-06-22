// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Env Query Context Current 타겟 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PREnvQueryContext_CurrentTarget.generated.h"

struct FEnvQueryContextData;
struct FEnvQueryInstance;

// Soldier_Armored EQS媛 ?꾩옱 ?寃잛쓣 湲곗??쇰줈 ?꾩튂瑜?怨좊Ⅴ??Context?? 
UCLASS()
class PROJECTR_API UPREnvQueryContext_CurrentTarget : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	/*~ UEnvQueryContext Interface ~*/
	// Querier媛 蹂닿퀬 ?덈뒗 current_target Actor瑜?EQS Context濡??쒓났?쒕떎.
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

protected:
	// 현재 타겟 Actor를 읽을 Blackboard 키 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Blackboard")
	FName TargetActorBlackboardKey = TEXT("current_target");
};
