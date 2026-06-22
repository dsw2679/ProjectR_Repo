// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy 전투 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyCombatDataAsset.generated.h"

class UEnvQuery;

UENUM(BlueprintType)
enum class EPREnemyQueryCandidateSelectionMode : uint8
{
	BestScore					UMETA(DisplayName = "BestScore"),
	RandomTopCandidates			UMETA(DisplayName = "RandomTopCandidates"),
	WeightedRandomTopCandidates	UMETA(DisplayName = "WeightedRandomTopCandidates")
};

// EQS 실행 시 Named Float Param으로 주입할 값
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyEQSFloatParam
{
	GENERATED_BODY()

	// EQS 쿼리에서 참조할 Named Param 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	FName ParamName = NAME_None;

	// ParamName에 대응하는 실수 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	float Value = 0.0f;
};

// 전투 이동 중 회전 정책과 애님 표현 문맥 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyMovePresentationConfig
{
	GENERATED_BODY()

	// 타겟 Focus 유지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bMaintainTargetFocus = true;

	// 전투 locomotion BlendSpace 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatMovePose = true;

	// 전투 AimOffset 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatAimOffset = true;

	// Combat_Strafe 상태 진입 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatStrafeState = false;

	// Controller 기반 회전 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	bool bUseControllerDesiredRotation = true;

	// 이동 방향 기반 회전 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	bool bOrientRotationToMovement = false;

	// 이동 속도 override 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeedOverride = 0.0f;

	// Yaw 회전 속도 override 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float RotationYawRate = 0.0f;
};

// tactical_mode 진입 시 기본 전투 표현 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyTacticalModePresentationRule
{
	GENERATED_BODY()

	// 적용 대상 tactical_mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Presentation")
	EPRTacticalMode TacticalMode = EPRTacticalMode::Idle;

	// 적용할 전투 이동 표현 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Presentation")
	FPREnemyMovePresentationConfig PresentationConfig;
};

// EQS 기반 전투 이동 쿼리 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyMoveQueryConfig
{
	GENERATED_BODY()

	// 이동 목표 선택 EQS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

	// EQS 실행 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> QueryRunMode = EEnvQueryRunMode::SingleResult;

	// 후보 선택 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS에 넘길 Named Float Param 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 상위 후보 최대 선택 개수
	// 0 이하면 개수 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS", meta = (ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 비율
	// 0.9면 최고 점수의 90% 이상 후보만 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;

	// 쿼리 성공 시 적용할 전투 이동 표현 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Presentation")
	FPREnemyMovePresentationConfig PresentationConfig;
};

// tactical_mode별 attack_pressure 증가 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttackPressureGainRule
{
	GENERATED_BODY()

	// 규칙 적용 tactical_mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	EPRTacticalMode TacticalMode = EPRTacticalMode::Idle;

	// 초당 증가량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	float GainPerSecond = 0.0f;
};

// attack_pressure 누적 및 초기화 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttackPressureConfig
{
	GENERATED_BODY()

	// pressure 최대값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure", meta = (ClampMin = "0.0"))
	float MaxPressure = 10.0f;

	// 탐지 직후 시작 pressure 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure", meta = (ClampMin = "0.0"))
	float InitialAttackPressureOnAlert = 0.0f;

	// tactical_mode별 pressure 증가 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	TArray<FPREnemyAttackPressureGainRule> GainRules;

	// 타겟 상실 시 pressure 초기화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	bool bResetWhenNoTarget = true;

	// Return 진입 시 pressure 초기화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	bool bResetWhenReturning = true;

	// Dead / Groggy 상태 시 pressure 초기화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	bool bResetWhenDisabled = true;

	// LOS 미충족 시 pressure 누적 차단 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	bool bRequireLOSForGain = true;

	// 현재 tactical_mode에 맞는 pressure 증가량 반환
	float GetGainPerSecond(EPRTacticalMode TacticalMode) const
	{
		for (const FPREnemyAttackPressureGainRule& GainRule : GainRules)
		{
			if (GainRule.TacticalMode == TacticalMode)
			{
				return GainRule.GainPerSecond;
			}
		}

		return 0.0f;
	}
};

// 적/보스가 공통으로 사용하는 이동 표현 데이터 자산
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRCombatMoveDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// tactical_mode별 기본 전투 표현 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Presentation")
	TArray<FPREnemyTacticalModePresentationRule> TacticalModePresentationRules;

	// tactical_mode에 해당하는 전투 표현 규칙 반환
	const FPREnemyMovePresentationConfig* FindTacticalModePresentationConfig(EPRTacticalMode TacticalMode) const
	{
		for (const FPREnemyTacticalModePresentationRule& PresentationRule : TacticalModePresentationRules)
		{
			if (PresentationRule.TacticalMode == TacticalMode)
			{
				return &PresentationRule.PresentationConfig;
			}
		}

		return nullptr;
	}
};

// 일반 적이 공용으로 사용하는 전투 데이터 자산
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPREnemyCombatDataAsset : public UPRCombatMoveDataAsset
{
	GENERATED_BODY()

public:
	// 플레이어 주변 공격 각도 확보용 Strafe 쿼리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig CombatStrafeConfig;

	// 근접 공격 범위 진입용 쿼리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig ApproachMeleeRangeConfig;

	// 원거리 접근용 FastApproach 쿼리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig FastApproachConfig;

	// attack_pressure 누적 및 리셋 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Pressure")
	FPREnemyAttackPressureConfig AttackPressureConfig;

	// 타겟 후보 유지와 선택 정책
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Targeting")
	FPREnemyTargetingConfig TargetingConfig;
};
