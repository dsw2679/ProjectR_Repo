// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Debug Draw 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinDebugDrawComponent.generated.h"

class UBlackboardComponent;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
struct FPRPatternRule;

// Faerin 전용 PIE 디버그 범위를 그리는 컴포넌트다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinDebugDrawComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Tick 기반 디버그 표시를 위한 기본값을 설정한다.
	UPRFaerinDebugDrawComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 현재 PIE 환경에서 디버그 범위를 그릴 수 있는지 확인한다.
	bool ShouldDrawDebugRanges() const;

	// PatternDataAsset의 Faerin 패턴별 선택 범위를 그린다.
	void DrawPatternRanges() const;

	// 감지 데이터 에셋을 기준으로 시야/청각 범위를 그린다.
	void DrawPerceptionRanges() const;

	// 현재 타겟과 Blackboard 상태를 모아 패턴 룰 비교용 문맥을 만든다.
	bool BuildPatternContext(FPRPatternContext& OutContext, AActor*& OutCurrentTarget) const;

	// 현재 ThreatComponent가 선택한 타겟을 반환한다.
	AActor* GetCurrentTarget() const;

	// Owner 또는 Override에서 패턴 데이터를 가져온다.
	const UPRPatternDataAsset* GetPatternDataAsset() const;

	// Owner 또는 Override에서 감지 데이터를 가져온다.
	const UPRPerceptionConfig* GetPerceptionConfig() const;

	// Owner의 AIController에서 Blackboard를 가져온다.
	const UBlackboardComponent* GetBlackboardComponent() const;

	// Blackboard에 키가 실제로 준비되어 있는지 반환한다.
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const;

	// 현재 선택된 AbilityTag 이름을 Blackboard에서 가져온다.
	FName GetSelectedAbilityTagName(const UBlackboardComponent* BlackboardComponent) const;

	// Owner가 가진 현재 보스 페이즈를 가져온다.
	EPRBossPhase GetCurrentBossPhase() const;

	// 룰이 현재 보스 페이즈에서 허용되는지 확인한다.
	bool IsRuleAllowedInCurrentPhase(const FPRPatternRule& PatternRule, EPRBossPhase CurrentPhase) const;

	// 패턴 룰의 표시 색을 결정한다.
	FColor ResolvePatternRuleColor(
		const FPRPatternRule& PatternRule,
		const FPRPatternContext& PatternContext,
		FName SelectedAbilityTagName) const;

	// 화면에 표시할 짧은 패턴 라벨을 만든다.
	FString MakePatternRuleLabel(const FPRPatternRule& PatternRule, const FPRPatternContext& PatternContext) const;

	// Owner와 타겟 사이의 현재 거리를 계산한다.
	float GetDistanceToTarget(const AActor* CurrentTarget) const;

	// 현재 타겟이 시야 부채꼴 안에 들어오는지 확인한다.
	bool IsTargetInsideSightCone(const AActor* CurrentTarget, float Radius, float PeripheralVisionAngleDegrees) const;

	// 수평 원형 범위를 그린다.
	void DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const;

	// 시야 부채꼴을 그린다.
	void DrawVisionArc(const FVector& Center, const FVector& ForwardDirection, float Radius, float PeripheralVisionAngleDegrees, const FColor& Color) const;

	// 범위 라벨을 겹침이 덜한 각도로 그린다.
	void DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color, int32 LabelIndex) const;

	// 현재 패턴 선택 문맥을 Owner 위에 표시한다.
	void DrawContextLabel(const FPRPatternContext& PatternContext, FName SelectedAbilityTagName) const;

protected:
	// 패턴 범위 디버그 사용 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bEnablePatternRangeDebug = true;

	// 패턴 범위 디버그에 콘솔 변수 사용이 필요한지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bRequirePatternConsoleVariable = true;

	// 감지 범위 디버그 사용 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bEnablePerceptionDebug = true;

	// 감지 범위 디버그에 콘솔 변수 사용이 필요한지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bRequirePerceptionConsoleVariable = true;

	// 서버 권한 인스턴스에서만 그릴지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawOnlyAuthority = true;

	// 패턴 데이터를 직접 지정할 때 사용한다. 비워 두면 Owner의 PatternDataAsset을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRPatternDataAsset> PatternDataAssetOverride;

	// 감지 데이터를 직접 지정할 때 사용한다. 비워 두면 Owner의 PerceptionConfig를 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRPerceptionConfig> PerceptionConfigOverride;

	// 원과 부채꼴을 바닥에서 띄워 그릴 높이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawHeightOffset = 8.0f;

	// 원과 부채꼴 분할 개수다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "12"))
	int32 CircleSegments = 96;

	// 선 두께다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float LineThickness = 2.0f;

	// Tick마다 다시 그릴 때 유지할 시간이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawDuration = 0.0f;

	// 패턴 라벨 표시 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawLabels = true;

	// 현재 선택 문맥 라벨 표시 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawContextLabel = true;

	// 타겟 방향 보조선을 표시할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawTargetLine = true;

	// 패턴 라벨 사이의 기본 각도 간격이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "1.0"))
	float LabelAngleStepDegrees = 27.0f;

	// 모든 조건을 만족하는 패턴 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor ActiveColor = FColor::Green;

	// 현재 Blackboard에 선택된 패턴 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor SelectedColor = FColor::Cyan;

	// 거리 조건만 맞지 않는 패턴 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor RangeMismatchColor = FColor(160, 160, 160);

	// 페이즈 조건이 맞지 않는 패턴 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor PhaseBlockedColor = FColor(180, 90, 255);

	// LOS/전술/돌진 경로 등 기타 조건이 맞지 않는 패턴 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor BlockedColor = FColor(255, 120, 80);

	// 시야 조건 만족 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor PerceptionActiveColor = FColor::Yellow;

	// 시야 유지 반경 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor LoseSightColor = FColor(255, 180, 0);

	// 청각 범위 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor HearingColor = FColor(255, 105, 180);

	// 타겟 보조선 색상이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor TargetLineColor = FColor::White;

	// 현재 타겟 Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// LOS Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// 돌진 경로 Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	// 전술 상태 Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 공격 압박 Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 선택된 AbilityTag Blackboard 키 이름이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelectedAbilityTagKey = TEXT("selected_ability_tag");
};
