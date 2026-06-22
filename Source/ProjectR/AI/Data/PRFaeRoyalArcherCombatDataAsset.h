// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Fae 로열 아처 전투 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRFaeRoyalArcherCombatDataAsset.generated.h"

// Fae Royal Archer의 공중 교전 거리와 비행 이동 기준을 보관하는 데이터 자산이다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaeRoyalArcherCombatDataAsset : public UPREnemyCombatDataAsset
{
	GENERATED_BODY()

public:
	// 궁수 공중 전투 데이터 기본값을 초기화한다.
	UPRFaeRoyalArcherCombatDataAsset();

public:
	// 사격을 가장 선호하는 타겟 기준 수평 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Range", meta = (ClampMin = "0.0"))
	float PreferredAttackDistance = 1400.0f;

	// 이 거리보다 가까우면 사격보다 공중 회피를 우선한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Range", meta = (ClampMin = "0.0"))
	float MinAttackDistance = 500.0f;

	// 이 거리보다 멀면 사격보다 공중 위치 보정을 우선한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Range", meta = (ClampMin = "0.0"))
	float MaxAttackDistance = 1800.0f;

	// 전투 중 타겟 기준으로 유지할 선호 공중 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float PreferredCombatHoverHeight = 250.0f;

	// 비전투 또는 복귀 중 HomeLocation 기준으로 유지할 선호 공중 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float HomeHoverHeightOffset = 250.0f;

	// 타겟 또는 HomeLocation 기준 최소 상대 고도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float MinFlightHeightAboveReference = 120.0f;

	// 타겟 또는 HomeLocation 기준 최대 상대 고도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float MaxFlightHeightAboveReference = 900.0f;

	// 공중 이동 중 CharacterMovement Flying 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float AirMoveSpeed = 420.0f;

	// 공중 이동 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float AirMoveAcceptanceRadius = 120.0f;

	// 공중 이동이 이 시간 안에 끝나지 않으면 BT가 재판단하도록 실패 처리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Flight", meta = (ClampMin = "0.05"))
	float AirMoveTimeout = 3.0f;

	// 타겟을 잃은 뒤 마지막 목격 위치 주변을 수색할 공중 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Search", meta = (ClampMin = "0.0"))
	float AirSearchRadius = 650.0f;

	// 마지막 목격 위치 기준 수색 목표 고도 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Search", meta = (ClampMin = "0.0"))
	float AirSearchHoverHeight = 220.0f;

	// 비전투 Patrol 중 home_location 주변을 배회할 공중 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Patrol", meta = (ClampMin = "0.0"))
	float AirPatrolRadius = 450.0f;

	// 비전투 Patrol 중 home_location 기준 목표 고도 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Patrol", meta = (ClampMin = "0.0"))
	float AirPatrolHoverHeight = 180.0f;

	// Strafe 목표점을 현재 방향에서 회전시킬 기본 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Strafe", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AirStrafeArcDegrees = 35.0f;

	// 공중 전투 위치를 선택 EQS로 먼저 고를지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirEQS")
	bool bUseAirCombatPositionQuery = false;

	// 공중 전투 위치 후보 EQS다. 결과 XY를 사용하고 Z는 궁수 전투 고도로 보정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirEQS", meta = (EditCondition = "bUseAirCombatPositionQuery"))
	FPREnemyMoveQueryConfig AirCombatPositionQueryConfig;

	// 공중 전투 후보 생성 시 PreferredAttackDistance에 더할 거리 오프셋 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition")
	TArray<float> AirPositionDistanceOffsets;

	// 공중 전투 후보 생성 시 PreferredCombatHoverHeight에 더할 고도 오프셋 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition")
	TArray<float> AirPositionHeightOffsets;

	// 현재 위치와 너무 가까운 후보를 낮게 평가하기 위한 기준 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition", meta = (ClampMin = "0.0"))
	float AirPositionMinSeparationFromCurrent = 260.0f;

	// 선호 사거리와 가까운 후보에 부여할 점수 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition", meta = (ClampMin = "0.0"))
	float AirPositionDistanceScoreWeight = 1.0f;

	// 타겟 시야가 확보된 후보에 부여할 점수 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition", meta = (ClampMin = "0.0"))
	float AirPositionLOSScoreWeight = 2.5f;

	// 현재 위치에서 충분히 벗어나는 후보에 부여할 점수 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition", meta = (ClampMin = "0.0"))
	float AirPositionSeparationScoreWeight = 0.65f;

	// 선호 전투 고도에 가까운 후보에 부여할 점수 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|AirPosition", meta = (ClampMin = "0.0"))
	float AirPositionHeightScoreWeight = 0.35f;

	// 근접 회피 시 타겟 반대 방향으로 빠질 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Evade", meta = (ClampMin = "0.0"))
	float CloseEvadeBackDistance = 700.0f;

	// 근접 회피 시 좌우로 섞을 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Evade", meta = (ClampMin = "0.0"))
	float CloseEvadeSideDistance = 350.0f;

	// 근접 회피 시 위쪽으로 빠질 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Evade", meta = (ClampMin = "0.0"))
	float CloseEvadeUpDistance = 160.0f;

	// 공중 이동 경로 Sweep 검사에 사용할 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Trace", meta = (ClampMin = "0.0"))
	float AirMoveProbeRadius = 34.0f;

	// 공중 전투 이동 중 적용할 Focus, AimOffset, Strafe 표현 문맥이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|RoyalArcher|Presentation")
	FPREnemyMovePresentationConfig AirCombatPresentationConfig;
};
