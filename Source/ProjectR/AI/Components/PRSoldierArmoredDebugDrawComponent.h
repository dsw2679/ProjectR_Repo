// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Soldier Armored Debug Draw 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRSoldierArmoredDebugDrawComponent.generated.h"

class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPRSoldierArmoredCombatDataAsset;
struct FPRPatternRule;

// Soldier_Armored 전용 PIE 디버그 범위를 그리는 컴포넌트다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRSoldierArmoredDebugDrawComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Tick 기반 디버그 드로우를 위한 기본값을 설정한다.
	UPRSoldierArmoredDebugDrawComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 현재 PIE 환경에서 디버그 범위를 그릴 수 있는지 확인한다.
	bool ShouldDrawDebugRanges() const;

	// 전투 데이터와 패턴 데이터를 기준으로 공격 범위를 그린다.
	void DrawCombatRanges() const;

	// 감지 데이터 에셋을 기준으로 시야/청각 범위를 그린다.
	void DrawPerceptionRanges() const;

	// 현재 ThreatComponent가 선택한 타겟을 반환한다.
	AActor* GetCurrentTarget() const;

	// Owner 또는 Override에서 패턴 데이터를 가져온다.
	const UPRPatternDataAsset* GetPatternDataAsset() const;

	// Owner 또는 Override에서 감지 데이터를 가져온다.
	const UPRPerceptionConfig* GetPerceptionConfig() const;

	// SprintHammer 규칙을 찾아서 반환한다.
	const FPRPatternRule* FindSprintHammerRule(const UPRPatternDataAsset* PatternDataAsset) const;

	// Owner와 현재 타겟 사이 거리를 계산한다.
	float GetDistanceToTarget(const AActor* CurrentTarget) const;

	// 현재 타겟이 시야 부채꼴 안에 들어오는지 확인한다.
	bool IsTargetInsideSightCone(const AActor* CurrentTarget, float Radius, float PeripheralVisionAngleDegrees) const;

	// 수평 원형 범위를 그린다.
	void DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const;

	// 시야 부채꼴을 그린다.
	void DrawVisionArc(const FVector& Center, const FVector& ForwardDirection, float Radius, float PeripheralVisionAngleDegrees, const FColor& Color) const;

	// 범위 라벨을 그린다.
	void DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color) const;

	// 최대 거리 조건 범위를 그린다.
	void DrawMaxRange(const FString& Label, const FVector& Center, float Radius, float DistanceToTarget) const;

protected:
	// 전투 범위 디버그 사용 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bEnableRangeDebug = true;

	// 전투 범위 디버그에 콘솔 변수 사용이 필요한지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bRequireConsoleVariable = true;

	// 감지 범위 디버그 사용 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bEnablePerceptionDebug = true;

	// 감지 범위 디버그에 콘솔 변수 사용이 필요한지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bRequirePerceptionConsoleVariable = true;

	// 서버 권한 인스턴스에서만 그릴지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawOnlyAuthority = true;

	// 해머 콤보와 스프린트 히트 범위를 읽을 전투 데이터다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRSoldierArmoredCombatDataAsset> CombatDataAsset;

	// 패턴 데이터를 직접 지정할 때 사용한다. 비워 두면 Owner의 PatternDataAsset을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRPatternDataAsset> PatternDataAssetOverride;

	// 감지 데이터를 직접 지정할 때 사용한다. 비워 두면 Owner의 PerceptionConfig를 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRPerceptionConfig> PerceptionConfigOverride;

	// 원과 부채꼴을 바닥에서 띄워서 그릴 높이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawHeightOffset = 8.0f;

	// 원과 부채꼴 분할 개수다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "12"))
	int32 CircleSegments = 96;

	// 선 두께다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float LineThickness = 2.0f;

	// Tick마다 다시 그릴 때 선을 유지하는 시간이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawDuration = 0.0f;

	// 일반 조건 만족 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor ActiveColor = FColor::Green;

	// 일반 조건 미충족 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor InactiveColor = FColor(160, 160, 160);

	// Sprint 구간 조건 만족 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor SprintActiveColor = FColor::Cyan;

	// 시야 조건 만족 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor PerceptionActiveColor = FColor::Yellow;

	// 시야 유지 반경 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor LoseSightColor = FColor(255, 180, 0);

	// 청각 범위 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor HearingColor = FColor(255, 105, 180);

	// 라벨 표시 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawLabels = true;
};
