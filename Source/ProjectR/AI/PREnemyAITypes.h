// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 연동용 AI 타입 정의)
// Author: 손승우 (일반/보스 AI 순찰 및 전투 상태 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PREnemyAITypes.generated.h"

class AActor;

// Blackboard에 저장되는 몬스터의 큰 전술 상태다.
// 세부 패턴 선택은 BT/Ability가 담당하고, 이 값은 AI가 현재 무엇을 하려는지 공유하는 기준으로 쓴다.
UENUM(BlueprintType)
enum class EPRTacticalMode : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Alert		UMETA(DisplayName = "Alert"),
	FastApproach	UMETA(DisplayName = "FastApproach"),
	Attack		UMETA(DisplayName = "Attack"),
	Strafe		UMETA(DisplayName = "Strafe"),
	Return		UMETA(DisplayName = "Return"),
	Patrol		UMETA(DisplayName = "Patrol")
};

// 패턴의 전투 계열이다.
// BT 브랜치가 "근접", "돌진"처럼 큰 흐름을 먼저 나누고 DataAsset 규칙으로 세부 패턴을 고르게 한다.
UENUM(BlueprintType)
enum class EPRPatternCategory : uint8
{
	Any			UMETA(DisplayName = "Any"),
	Melee		UMETA(DisplayName = "Melee"),
	Sprint		UMETA(DisplayName = "Sprint"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Movement	UMETA(DisplayName = "Movement"),
	Special		UMETA(DisplayName = "Special")
};

// 패턴 컨텍스트 비교 시 어떤 조건을 무시할지 정의한다.
UENUM(BlueprintType)
enum class EPRPatternContextMatchMode : uint8
{
	FullMatch	UMETA(DisplayName = "FullMatch"),
	IgnoreRange	UMETA(DisplayName = "IgnoreRange")
};

// Perception이 타겟을 잃었을 때 Threat/Blackboard를 어디까지 정리할지 정한다.
// 적 종류마다 수색 집착도가 다를 수 있으므로 AIController 설정값으로 사용한다.
UENUM(BlueprintType)
enum class EPRTargetLostPolicy : uint8
{
	KeepCurrentTarget	UMETA(DisplayName = "Keep Current Target"),
	ClearCurrentTarget	UMETA(DisplayName = "Clear Current Target"),
	RemoveThreatEntry	UMETA(DisplayName = "Remove Threat Entry")
};

// 보스 공통 페이즈 값이다.
// 현재 Faerin은 이 공용 페이즈를 자신의 패턴 단계에 매핑한다.
UENUM(BlueprintType)
enum class EPRBossPhase : uint8
{
	Phase1	UMETA(DisplayName = "Phase 1"),
	Phase2	UMETA(DisplayName = "Phase 2"),
	Phase3	UMETA(DisplayName = "Phase 3"),
	Phase4	UMETA(DisplayName = "Phase 4")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRThreatEntry
{
	GENERATED_BODY()

	// 위협을 누적할 대상 액터다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	TObjectPtr<AActor> Target = nullptr;

	// 대상에게 누적된 위협 수치다. 가장 높은 대상이 현재 타겟 후보가 된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float ThreatValue = 0.0f;

	// 마지막으로 위협이 갱신된 시간이다. 오래된 항목을 잊어버릴 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastUpdatedTime = 0.0f;
};

// 몬스터가 전투 대상으로 유지할 수 있는 플레이어 후보 정보다.
// Perception, 피해 이벤트, 거리 기반 유지 조건, 점수 기반 타겟 선택을 한 항목에서 관리한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyTargetCandidate
{
	GENERATED_BODY()

	// 후보 플레이어 액터다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	TObjectPtr<AActor> Target = nullptr;

	// 현재 Perception이 감지 중인지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bCurrentlyPerceived = false;

	// 현재 후보와 직접 시야선이 이어져 있는지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bHasLOS = false;

	// 마지막으로 확인한 후보 위치다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	FVector LastKnownLocation = FVector::ZeroVector;

	// 마지막으로 감지된 월드 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastSensedTime = 0.0f;

	// 마지막으로 피해나 위협 이벤트가 들어온 월드 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastThreatTime = 0.0f;

	// 모든 후보에게 기본으로 주는 선택 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float BaseScore = 1.0f;

	// 최근 점수 창 안에서 피해량으로 누적한 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DamageScore = 0.0f;

	// 재평가 시 거리로 계산되는 임시 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DistanceScore = 0.0f;

	// 현재 타겟을 너무 자주 바꾸지 않기 위한 유지 보정 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float StickinessScore = 0.0f;

	// 마지막으로 후보 상태가 갱신된 월드 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastUpdatedTime = 0.0f;

	// 현재 점수 창에서 누적한 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DamageInCurrentWindow = 0.0f;

	// 최종 선택 계산에 사용한 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float FinalSelectionScore = 0.0f;

	// 기존 최고 Threat 선택 로직을 유지하기 위한 1차 호환 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float ThreatValue = 0.0f;
};

// 몬스터 개체별 타겟 후보 유지와 선택 정책이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyTargetingConfig
{
	GENERATED_BODY()

	// 피해 기반 점수를 모았다가 초기화하는 시간 창이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.1"))
	float ScoreWindowDuration = 10.0f;

	// 모든 공격 후보에게 기본으로 주는 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float BaseCandidateScore = 1.0f;

	// 피해량을 선택 점수로 바꿀 때 곱하는 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float DamageScoreScale = 0.05f;

	// 피해 기반 점수의 최대값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MaxDamageScore = 5.0f;

	// 가까운 후보에게 줄 수 있는 최대 거리 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float DistanceScoreMax = 3.0f;

	// 거리 점수가 0에 가까워지는 기준 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "1.0"))
	float DistanceScoreMaxRange = 1200.0f;

	// LOS가 있는 후보에게 주는 보정 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float LOSScore = 0.5f;

	// 현재 타겟을 유지하기 위해 더하는 보정 점수다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float CurrentTargetStickinessScore = 1.0f;

	// 새 타겟이 현재 타겟보다 이 비율 이상 높을 때 전환을 허용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "1.0"))
	float SwitchScoreRatio = 1.3f;

	// 타겟이 너무 자주 바뀌지 않도록 전환 후 잠그는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float SwitchCooldown = 3.0f;

	// LOS 상실 후에도 공격 후보를 유지할 수 있는 전투 유지 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float EngagementRetainRadius = 2500.0f;

	// 갱신되지 않은 후보를 잊어버리는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float CandidateForgetTime = 10.0f;

	// 한 번 후보에 들어온 플레이어를 시야/인지 손실만으로 제거하지 않을지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bNeverForgetPlayerCandidates = false;

	// 페이즈 전환 중에는 현재 타겟과 후보 목록을 유지할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bKeepCurrentTargetDuringPhaseTransition = false;

	// current target이 비었을 때 남아 있는 후보에서 즉시 복구할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bRestoreCurrentTargetFromCandidates = false;

	// 플레이어 후보에는 전투 유지 반경 제거 규칙을 적용하지 않을지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bIgnoreEngagementRetainRadiusForPlayerCandidates = false;
};

// 공격 하나가 시작된 뒤 타겟이 중간에 바뀌지 않도록 잠그는 런타임 상태다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttackCommitState
{
	GENERATED_BODY()

	// 현재 실행 중인 공격이 참조하는 고정 타겟이다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	TObjectPtr<AActor> ActiveAttackTarget = nullptr;

	// 공격 시작 시점에 고정한 타겟 위치 또는 MotionWarp 기준점이다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	FVector ActiveAttackTargetLocation = FVector::ZeroVector;

	// 공격 하나가 커밋되어 타겟 교체를 막아야 하는지 여부다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bIsAttackCommitted = false;

	// 같은 타겟 고정 구간이 중첩될 때 가장 바깥 구간이 끝날 때까지 고정을 유지하기 위한 깊이다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	int32 AttackCommitDepth = 0;

	// 공격 중 타겟 전환이 보류된 후보다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	TObjectPtr<AActor> PendingTargetCandidate = nullptr;

	// 보류 후보의 마지막 계산 점수다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI")
	float PendingTargetScore = 0.0f;
};

// 패턴 선택 시점에 필요한 상황값만 모아둔 구조체다.
// BTTask가 Blackboard와 Pawn 상태를 읽어 채운 뒤 FPRPatternRule과 비교한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternContext
{
	GENERATED_BODY()

	// 현재 타겟까지의 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DistanceToTarget = 0.0f;

	// 타겟을 시야선으로 확인할 수 있는지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bHasLOS = false;

	// 현재 전술 상태다. 지금은 거리/LOS 중심이지만 이후 패턴 조건 확장 지점이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	EPRTacticalMode TacticalMode = EPRTacticalMode::Idle;

	// 돌진 계열 패턴이 실제로 지나갈 수 있는 직선 경로를 확보했는지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bChargePathClear = false;

	// 현재 패턴 선택 주체가 보스인지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bIsBoss = false;

	// 보스 패턴 선택 시 비교할 현재 보스 페이즈다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	EPRBossPhase BossPhase = EPRBossPhase::Phase1;

	// 현재 누적된 공격 압박도다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float CurrentAttackPressure = 0.0f;
};

// 하나의 몬스터 패턴 후보를 정의한다.
// 조건에 맞는 후보들만 모은 뒤 SelectionWeight 비율로 최종 Ability를 고른다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternRule
{
	GENERATED_BODY()

	// 실행할 Ability를 찾는 GameplayTag다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	FGameplayTag AbilityTag;

	// BT에서 큰 공격 흐름을 나눌 때 사용하는 패턴 계열이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	EPRPatternCategory PatternCategory = EPRPatternCategory::Any;

	// 원작 패턴군 또는 보스별 세부 패턴 그룹을 구분하는 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	FGameplayTag PatternGroupTag;

	// 이 패턴이 유효한 최소 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	// 이 패턴이 유효한 최대 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MaxRange = 3000.0f;

	// true면 타겟이 시야선 안에 있을 때만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRequiresLOS = true;

	// true면 Blackboard의 charge_path_clear가 켜져 있을 때만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRequiresChargePathClear = false;

	// true면 AllowedTacticalModes에 들어 있는 전술 상태에서만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRestrictTacticalModes = false;

	// 이 패턴을 허용할 전술 상태 목록이다. 비워두면 bRestrictTacticalModes가 false일 때만 의미가 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (EditCondition = "bRestrictTacticalModes"))
	TArray<EPRTacticalMode> AllowedTacticalModes;

	// true면 AllowedBossPhases에 들어 있는 보스 페이즈에서만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRestrictBossPhases = false;

	// 이 패턴을 허용할 보스 페이즈 목록이다. 일반 몬스터에는 적용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (EditCondition = "bRestrictBossPhases"))
	TArray<EPRBossPhase> AllowedBossPhases;

	// 조건을 통과한 후보끼리의 선택 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	// 이 패턴을 선택하기 위해 필요한 최소 공격 압박도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float RequiredAttackPressure = 0.0f;

	// 데이터 에셋에서 잘못된 패턴이 들어왔는지 빠르게 걸러낸다.
	bool IsValid() const
	{
		return AbilityTag.IsValid()
			&& MinRange <= MaxRange
			&& SelectionWeight > 0.0f;
	}

	// 현재 상황값이 이 패턴의 거리/시야 조건을 만족하는지 확인한다.
	bool MatchesContext(
		const FPRPatternContext& Context,
		const EPRPatternContextMatchMode MatchMode = EPRPatternContextMatchMode::FullMatch) const
	{
		if (!IsValid())
		{
			return false;
		}

		if (MatchMode != EPRPatternContextMatchMode::IgnoreRange)
		{
			const bool bInRange = Context.DistanceToTarget >= MinRange
				&& Context.DistanceToTarget <= MaxRange;
			if (!bInRange)
			{
				return false;
			}
		}

		if (bRequiresLOS && !Context.bHasLOS)
		{
			return false;
		}

		if (bRequiresChargePathClear && !Context.bChargePathClear)
		{
			return false;
		}

		if (bRestrictTacticalModes && !AllowedTacticalModes.Contains(Context.TacticalMode))
		{
			return false;
		}

		if (bRestrictBossPhases && (!Context.bIsBoss || !AllowedBossPhases.Contains(Context.BossPhase)))
		{
			return false;
		}

		if (Context.CurrentAttackPressure < RequiredAttackPressure)
		{
			return false;
		}

		return true;
	}
};
