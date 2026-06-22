// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Pick Reachable Location 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "BTTask_PRPickReachableLocation.generated.h"

class APawn;
class UBlackboardComponent;
class UEnvQuery;
class UNavigationQueryFilter;

// 순찰/수색처럼 위치만 필요한 EQS 선택 설정이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyLocationQueryConfig
{
	GENERATED_BODY()

	// 이동 목표 위치를 선택할 EQS 템플릿이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

	// EQS 실행 방식이다. 랜덤 후보 선택 모드에서는 내부적으로 AllMatching으로 보정된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> QueryRunMode = EEnvQueryRunMode::SingleResult;

	// EQS 결과 후보 중 최종 위치를 고르는 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS Named Float Param으로 주입할 값 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 상위 후보 최대 선택 개수다. 0 이하면 개수 제한을 두지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS", meta = (ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;
};

// Blackboard 기준 위치 주변에서 NavMesh 이동 가능 지점을 선택하는 공용 Task다.
UCLASS()
class PROJECTR_API UBTTask_PRPickReachableLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 기본 Blackboard 키와 탐색 설정을 초기화한다.
	UBTTask_PRPickReachableLocation();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 기준 위치 Blackboard 값을 읽는다.
	bool ResolveOriginLocation(const UBlackboardComponent* BlackboardComponent, FVector& OutOriginLocation) const;

	// EQS 기반 이동 후보 위치를 선택한다.
	bool PickEQSLocation(APawn* ControlledPawn, FVector& OutPickedLocation) const;

	// 기준 위치 주변에서 이동 가능한 NavMesh 지점을 선택한다.
	bool PickReachableLocation(const UObject* WorldContextObject, const FVector& OriginLocation, FVector& OutPickedLocation) const;

	// 선택된 이동 목표를 Blackboard에 기록한다.
	bool WriteGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const;

protected:
	// 순찰/수색 기준 위치를 읽을 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName OriginLocationKey = TEXT("home_location");

	// 선택된 이동 지점을 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// 설정되어 있으면 NavMesh 랜덤 선택보다 먼저 실행할 EQS 순찰 위치 선택 설정이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|EQS")
	FPREnemyLocationQueryConfig EQSQueryConfig;

	// EQS가 실패했을 때 기존 NavMesh 랜덤 위치 선택으로 보정할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|EQS")
	bool bFallbackToNavmeshRandom = true;

	// 기준 위치 주변에서 이동 지점을 고를 반경이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "0.0"))
	float PickRadius = 700.0f;

	// 기준 위치와 너무 가까운 후보를 거르기 위한 최소 거리다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "0.0"))
	float MinDistanceFromOrigin = 150.0f;

	// NavMesh 후보를 다시 뽑아볼 최대 횟수다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "1"))
	int32 MaxPickAttempts = 8;

	// NavMesh 후보를 찾지 못했을 때 기준 위치를 목표로 기록할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation")
	bool bFallbackToOrigin = true;

	// 이동 가능 지점 검색에 사용할 Navigation Query Filter다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation")
	TSubclassOf<UNavigationQueryFilter> FilterClass;
};
