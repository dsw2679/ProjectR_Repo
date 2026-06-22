// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 연동 어그로 갱신 구현)
// Author: 손승우 (적 AI의 타겟 가시성, 사거리 및 블랙보드 전투 정보 갱신 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PRUpdateEnemyCombatBlackboard.generated.h"

// 전투 중 자주 필요한 Blackboard 값을 한 곳에서 갱신하는 서비스다.
// BT 조건 노드와 패턴 선택 Task가 같은 거리/타겟/돌진 가능 여부를 보도록 만든다.
UCLASS()
class PROJECTR_API UBTService_PRUpdateEnemyCombatBlackboard : public UBTService
{
	GENERATED_BODY()

public:
	// 서비스 기본 주기와 노드 이름 설정
	UBTService_PRUpdateEnemyCombatBlackboard();

	// 전투 대상, 거리, 돌진 경로 정보를 Blackboard에 갱신
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// BT 에디터에 표시할 서비스 설명 반환
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 AI Pawn 자신을 기록하는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelfActorKey = TEXT("self_actor");

	// 아래 키 이름들은 적 Blackboard 에셋의 키와 맞아야 한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasValidTargetKey = TEXT("has_valid_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName DistanceToTargetKey = TEXT("distance_to_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 돌진 경로가 막혔는지 확인할 Trace 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	TEnumAsByte<ECollisionChannel> ChargeTraceChannel = ECC_Visibility;

	// 현재 타겟이 실제 시야선 안에 있는지 확인할 Trace 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AI")
	TEnumAsByte<ECollisionChannel> LOSCheckTraceChannel = ECC_Visibility;
};
