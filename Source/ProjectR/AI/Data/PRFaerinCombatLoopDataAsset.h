// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 전투 Loop 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinCombatLoopDataAsset.generated.h"

class UPRAbilitySystemComponent;
class UPRPatternDataAsset;

UENUM(BlueprintType)
enum class EPRFaerinTeleportWrapperPolicy : uint8
{
	None				UMETA(DisplayName = "None"),
	TeleportOutOnly		UMETA(DisplayName = "Teleport Out Only"),
	TeleportOutAndIn	UMETA(DisplayName = "Teleport Out And In"),
	TeleportOutVFXOnly	UMETA(DisplayName = "Teleport Out VFX Only")
};

UENUM(BlueprintType)
enum class EPRFaerinTeleportVFXConvergePolicy : uint8
{
	TargetExact			UMETA(DisplayName = "Target Exact"),
	TargetNearby		UMETA(DisplayName = "Target Nearby")
};

UENUM(BlueprintType)
enum class EPRFaerinApproachPolicy : uint8
{
	PhaseDefault		UMETA(DisplayName = "Phase Default"),
	None				UMETA(DisplayName = "None"),
	KeepCurrentRange	UMETA(DisplayName = "Keep Current Range"),
	SprintToMeleeRange	UMETA(DisplayName = "Sprint To Melee Range"),
	ShiftClose			UMETA(DisplayName = "Shift Close"),
	NearTeleportToMeleeRange	UMETA(DisplayName = "Near Teleport To Melee Range"),
	SprintOrNearTeleport	UMETA(DisplayName = "Sprint Or Near Teleport")
};

UENUM(BlueprintType)
enum class EPRFaerinPostPatternPolicy : uint8
{
	PhaseDefault		UMETA(DisplayName = "Phase Default"),
	ForceStrafe			UMETA(DisplayName = "Force Strafe"),
	ForceImmediateNext	UMETA(DisplayName = "Force Immediate Next")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPeriodicSidePatternConfig
{
	GENERATED_BODY()

	// 해당 주기 보조 패턴을 현재 PhaseConfig에서 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern")
	bool bEnabled = false;

	// 주기적으로 실행할 보조 패턴 AbilityTag다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag AbilityTag;

	// 해당 페이즈 진입 후 첫 실행까지 기다릴 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern", meta = (ClampMin = "0.0"))
	float InitialDelaySeconds = 0.0f;

	// 한 번 시도한 뒤 다음 시도까지 기다릴 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern", meta = (ClampMin = "0.0"))
	float IntervalSeconds = 12.0f;

	// 실행 가능 시간이 되었을 때 실제로 Ability를 시도할 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ActivationChance = 1.0f;

	// 설계 검토용 메모다. 런타임에서는 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern")
	FString DesignNote;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPhaseLoopConfig
{
	GENERATED_BODY()

	// 이 설정이 적용되는 보스 페이즈다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRBossPhase Phase = EPRBossPhase::Phase1;

	// 현재 페이즈에서 공격 후 strafe가 지속되는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeDuration = 1.0f;

	// 현재 거리 대신 강제로 유지할 횡이동 반경이다. 0이면 현재 거리 기반으로 계산한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeRadiusOverride = 0.0f;

	// 한 번의 횡이동에서 타깃을 기준으로 회전할 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float StrafeArcAngleDegrees = 34.0f;

	// AI MoveTo 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeAcceptanceRadius = 80.0f;

	// 횡이동 목적지를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	FVector StrafeNavProjectExtent = FVector(220.0f, 220.0f, 360.0f);

	// 연속 횡이동 시 좌우 방향을 번갈아 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bAlternateStrafeDirection = true;

	// 횡이동 중 애니메이션/이동 표현에 적용할 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPREnemyMovePresentationConfig StrafePresentationConfig;

	// 스프린트 접근을 실제로 실행할 Gameplay Ability 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag ApproachAbilityTag;

	// 근거리 텔레포트 접근을 실제로 실행할 Gameplay Ability 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag NearTeleportAbilityTag;

	// sprint 접근으로 유지하려는 타깃과의 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachStopDistance = 500.0f;

	// 접근 이동을 유지할 최대 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachTimeoutSeconds = 0.75f;

	// AI MoveTo 접근 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachAcceptanceRadius = 120.0f;

	// 접근 목적지를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	FVector ApproachNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);

	// 접근 중 애니메이션/이동 표현에 적용할 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPREnemyMovePresentationConfig ApproachPresentationConfig;

	// Phase1/2에서 공격 실행 전 거리와 패턴 계열에 따라 접근 Ability를 먼저 실행할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach")
	bool bUsePrePatternApproachRoute = true;

	// 공격 전 접근 루트가 작동하기 시작하는 최소 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0"))
	float PrePatternApproachMinDistance = 50.0f;

	// 근거리 접근 분기 상한이다. 이 거리까지는 Spoke는 sprint, 원거리 계열은 EQS 텔레포트를 쓴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0"))
	float PrePatternApproachCloseMaxDistance = 500.0f;

	// 중거리 접근 분기 상한이다. 이 거리까지는 Spoke가 확률적으로 target 뒤 텔레포트를 섞는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0"))
	float PrePatternApproachMidMaxDistance = 1000.0f;

	// 중거리 Spoke 선택 시 target 뒤 텔레포트를 사용할 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MidRangeSpokeTargetBackTeleportChance = 0.8f;

	// target 뒤 텔레포트가 target의 forward 반대 방향으로 떨어질 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0"))
	float TargetBackTeleportDistance = 320.0f;

	// target 뒤 텔레포트 목적지를 NavMesh로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PrePatternApproach", meta = (ClampMin = "0.0"))
	FVector TargetBackTeleportNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);

	// 본 공격 전에 보조 포털 패턴을 확률적으로 끼워 넣을지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist")
	bool bUsePrePatternPortalAssist = false;

	// 보조 포털 패턴을 본 공격 앞에 붙일 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUsePrePatternPortalAssist"))
	float PrePatternPortalAssistChance = 0.5f;

	// 본 공격이 포털 계열일 때도 선행 포털 보조를 허용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (EditCondition = "bUsePrePatternPortalAssist"))
	bool bAllowPrePatternPortalAssistBeforePortalPatterns = true;

	// 보조 포털 후보로 사용할 PatternGroup이다. 비워 두면 Faerin Portal 그룹을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (Categories = "Pattern.Boss.Faerin", EditCondition = "bUsePrePatternPortalAssist"))
	FGameplayTag PrePatternPortalGroupTag;

	// 메인 공격 선택에서 제외할 PatternGroup 목록이다. 선행 보조 패턴 선택에는 영향을 주지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternSelection", meta = (Categories = "Pattern.Boss.Faerin"))
	FGameplayTagContainer MainPatternExcludedGroupTags;

	// 메인 공격 루프와 별개로 주기적으로 설치할 전장 압박용 보조 패턴들이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PeriodicSidePattern")
	TArray<FPRFaerinPeriodicSidePatternConfig> PeriodicSidePatterns;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPatternLoopMetadata
{
	GENERATED_BODY()

	// PatternData의 AbilityTag와 1:1로 대응되는 패턴 식별자다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag AbilityTag;

	// 현재 리팩터링 루프에서 선택 가능한 패턴인지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bEnabled = true;

	// 원작 재현용 텔레포트 전/후처리 정책이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinTeleportWrapperPolicy TeleportWrapperPolicy = EPRFaerinTeleportWrapperPolicy::None;

	// Phase3 이후 Teleport VFX가 어느 지점으로 집결할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	EPRFaerinTeleportVFXConvergePolicy TeleportVFXConvergePolicy = EPRFaerinTeleportVFXConvergePolicy::TargetExact;

	// TargetNearby 집결 시 타깃 주변에서 사용할 최소 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float TeleportVFXNearbyMinRadius = 180.0f;

	// TargetNearby 집결 시 타깃 주변에서 사용할 최대 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float TeleportVFXNearbyMaxRadius = 420.0f;

	// 계산된 집결 위치에 더할 월드 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	FVector TeleportVFXConvergeWorldOffset = FVector::ZeroVector;

	// 집결 완료 후 보스가 타겟을 바라보도록 회전할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	bool bTeleportVFXFaceTargetOnFinish = true;

	// 공격 종료 후 별도 접근이 꼭 필요한 패턴만 명시적으로 사용하는 접근 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinApproachPolicy ApproachPolicy = EPRFaerinApproachPolicy::PhaseDefault;

	// 공격 종료 후 루프 진행 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinPostPatternPolicy PostPatternPolicy = EPRFaerinPostPatternPolicy::PhaseDefault;

	// 설계 검토용 메모다. 런타임 판정에는 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FString DesignNote;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPatternPlan
{
	GENERATED_BODY()

	// 이번 루프에서 실행할 AbilityTag다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FGameplayTag AbilityTag;

	// PatternData에서 복사한 선택 규칙이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPRPatternRule PatternRule;

	// LoopData에서 복사한 실행 메타데이터다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPRFaerinPatternLoopMetadata LoopMetadata;

	// 최종 선택에 사용한 가중치다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float FinalSelectionWeight = 0.0f;
};

// Faerin 전용 전투 루프가 PatternData/AbilitySet을 원작형 순서로 엮는 데 사용하는 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinCombatLoopDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 페이즈별 루프 설정을 찾는다.
	const FPRFaerinPhaseLoopConfig* FindPhaseConfig(EPRBossPhase Phase) const;

	// AbilityTag에 대응되는 패턴 메타데이터를 찾는다.
	const FPRFaerinPatternLoopMetadata* FindPatternMetadata(const FGameplayTag& AbilityTag) const;

	// PatternData/AbilitySet 정합성을 점검하고 발견된 문제를 반환한다.
	bool ValidateLoopData(
		const UPRPatternDataAsset* PatternDataAsset,
		UPRAbilitySystemComponent* AbilitySystemComponent,
		TArray<FString>& OutErrors) const;

public:
	// 페이즈별 공격 후 루프 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TArray<FPRFaerinPhaseLoopConfig> PhaseConfigs;

	// PatternData의 Rule과 1:1 대응되는 패턴 실행 메타데이터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TArray<FPRFaerinPatternLoopMetadata> PatternMetadata;
};
