// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Update 타겟 조준 위협도 갱신용 비헤이비어 트리 서비스 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PRUpdateTargetAimThreat.generated.h"

class UBlackboardComponent;

// 현재 공격 대상이 자신을 조준 중인지 Blackboard에 기록하는 공용 BT Service다.
UCLASS()
class PROJECTR_API UBTService_PRUpdateTargetAimThreat : public UBTService
{
	GENERATED_BODY()

public:
	// 조준 위협 판정 주기와 기본 Blackboard 키를 초기화한다.
	UBTService_PRUpdateTargetAimThreat();

	/*~ UBTService Interface ~*/
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/*~ UBTNode Interface ~*/
	virtual FString GetStaticDescription() const override;

protected:
	// Blackboard 또는 AIController에서 자기 Actor를 가져온다.
	AActor* ResolveSelfActor(const UBehaviorTreeComponent& OwnerComp, const UBlackboardComponent* BlackboardComponent) const;

	// 현재 타겟이 자기 Actor를 조준 중인지 판정한다.
	bool IsTargetAimingAtSelf(AActor* TargetActor, AActor* SelfActor) const;

	// 타겟이 조준 상태 태그를 가지고 있는지 확인한다.
	bool HasRequiredAimingState(AActor* TargetActor) const;

	// 타겟의 조준 방향이 자기 Actor를 향하는지 확인한다.
	bool PassesAimCone(AActor* TargetActor, AActor* SelfActor, FVector& OutTargetViewLocation, FVector& OutSelfAimLocation) const;

	// 타겟과 자기 Actor 사이의 시야가 막혀 있는지 확인한다.
	bool PassesVisibilityTrace(AActor* TargetActor, AActor* SelfActor, const FVector& TargetViewLocation, const FVector& SelfAimLocation) const;

protected:
	// 현재 공격 대상 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 자기 Actor Blackboard 키다. 키가 없으면 AIController의 Pawn을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelfActorKey = TEXT("self_actor");

	// 조준 위협 여부를 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TargetAimingAtSelfKey = TEXT("is_target_aiming_at_self");

	// 타겟이 State.Aiming 태그를 가지고 있을 때만 조준 위협으로 인정할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat")
	bool bRequireTargetAimingState = true;

	// 조준 방향과 자기 Actor 방향 사이의 허용 반각이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AimConeHalfAngleDegrees = 10.0f;

	// 조준 위협을 인정할 최대 거리다. 0 이하이면 거리 제한을 사용하지 않는다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat", meta = (ClampMin = "0.0"))
	float MaxAimThreatDistance = 2500.0f;

	// 자기 Actor 위치에서 조준 목표점으로 사용할 높이 보정값이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat")
	float SelfAimTargetHeightOffset = 60.0f;

	// 조준 방향이 맞더라도 중간 장애물이 있으면 위협에서 제외할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat")
	bool bRequireLineOfSight = true;

	// 시야 확인에 사용할 Trace 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|AimThreat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
