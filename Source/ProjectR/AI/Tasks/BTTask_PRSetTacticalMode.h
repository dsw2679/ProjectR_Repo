// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 조준 대응용 우회 전술 전환 연동)
// Author: 손승우 (EQS 기반 적 전술 이동 상태 전환 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRSetTacticalMode.generated.h"

// BT 브랜치 진입/종료 시 tactical_mode를 명시적으로 갱신하는 Task다.
// 필요하면 공용 전투 이동 표현 설정도 함께 적용한다.
UCLASS()
class PROJECTR_API UBTTask_PRSetTacticalMode : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRSetTacticalMode();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// tactical_mode를 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 공용 전투 이동 표현 적용 시 참조할 현재 타겟 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 이 Task가 기록할 새 전술 상태다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	EPRTacticalMode NewTacticalMode = EPRTacticalMode::Idle;
};
