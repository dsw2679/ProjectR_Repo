// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Update Penitent 몬스터 전투 블랙보드 갱신용 비헤이비어 트리 서비스 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PRUpdatePenitentCombatBlackboard.generated.h"

// Penitent 전용 전투 Blackboard 보조값 갱신 서비스
UCLASS()
class PROJECTR_API UBTService_PRUpdatePenitentCombatBlackboard : public UBTService
{
	GENERATED_BODY()

public:
	// 서비스 주기와 이름 초기화
	UBTService_PRUpdatePenitentCombatBlackboard();

	/*~ UBTService Interface ~*/
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/*~ UBTNode Interface ~*/
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 타겟 거리 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName DistanceToTargetKey = TEXT("distance_to_target");

	// 거리 구간 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName DistanceBandKey = TEXT("distance_band");

	// 교전 거리 조절 방향 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CombatSpacingDirectionKey = TEXT("combat_spacing_direction");

	// 소환 배리어 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SpawnedBarrierActorKey = TEXT("spawned_barrier_actor");

	// 배리어 보유 여부 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasActiveBarrierKey = TEXT("has_active_barrier");

	// 회피 가능 여부 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CanDodgeKey = TEXT("can_dodge");
};
