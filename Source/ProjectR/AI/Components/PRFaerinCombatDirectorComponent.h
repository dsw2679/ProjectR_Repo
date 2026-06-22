// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 전투 Director 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "ProjectR/AI/Data/PRFaerinCombatLoopDataAsset.h"
#include "PRFaerinCombatDirectorComponent.generated.h"

class AAIController;
class APREnemyBaseCharacter;
class UBehaviorTreeComponent;
class UPRAbilitySystemComponent;
class UPRFaerinTeleportVFXComponent;
class UPRPatternDataAsset;
struct FAbilityEndedData;
struct FPREnemyPatternQueryRuntime;

UENUM(BlueprintType)
enum class EPRFaerinCombatLoopState : uint8
{
	Idle				UMETA(DisplayName = "Idle"),
	SelectingPattern	UMETA(DisplayName = "Selecting Pattern"),
	PreparingTeleportVFX UMETA(DisplayName = "Preparing Teleport VFX"),
	WaitingPatternEnd	UMETA(DisplayName = "Waiting Pattern End"),
	PostPatternStrafe	UMETA(DisplayName = "Post Pattern Strafe"),
	ApproachSprint		UMETA(DisplayName = "Approach Sprint"),
	ApproachNearTeleport UMETA(DisplayName = "Approach Near Teleport")
};

UENUM(BlueprintType)
enum class EPRFaerinNearTeleportPlacementMode : uint8
{
	SelfEQS			UMETA(DisplayName = "Self EQS"),
	TargetBack		UMETA(DisplayName = "Target Back")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRFaerinLoopStepFinishedSignature, bool, bSucceeded);

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinApproachSprintRequest
{
	GENERATED_BODY()

	// 접근 요청이 유효한지 나타낸다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bIsValid = false;

	// 스프린트 접근의 기준 타깃이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<AActor> TargetActor;

	// 타깃과 이 거리 이하로 좁혀지면 End 섹션으로 전환한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float StopDistance = 0.0f;

	// 접근이 너무 오래 지속되지 않게 막는 최대 시간이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float TimeoutSeconds = 0.0f;

	// MoveTo와 거리 종료 판정에 사용하는 허용 반경이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float AcceptanceRadius = 0.0f;

	// 접근 목적지를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FVector NavProjectExtent = FVector::ZeroVector;

	// 접근 중 애니메이션 이동 표현에 적용할 설정이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPREnemyMovePresentationConfig PresentationConfig;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinNearTeleportRequest
{
	GENERATED_BODY()

	// 접근 요청이 유효한지 나타낸다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bIsValid = false;

	// 근거리 텔레포트 접근의 기준 타깃이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<AActor> TargetActor;

	// 텔레포트 목적지를 계산하는 방식이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinNearTeleportPlacementMode PlacementMode = EPRFaerinNearTeleportPlacementMode::SelfEQS;

	// TargetBack 방식에서 target의 뒤쪽으로 물러나는 거리다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float TargetBackDistance = 320.0f;

	// TargetBack 목적지를 NavMesh로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FVector TargetBackNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);
};

enum class EPRFaerinObservedAbilityRole : uint8
{
	None,
	Pattern,
	PrePatternPortal,
	PrePatternApproach,
	Approach
};

enum class EPRFaerinPrePatternApproachStartResult : uint8
{
	NotNeeded,
	Started,
	Failed
};

// Faerin 전용 공격 루프를 PatternData, LoopData, AbilitySet 기반으로 실행하는 컴포넌트다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinCombatDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Faerin 루프 실행을 위한 기본값을 설정한다.
	UPRFaerinCombatDirectorComponent();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 현재 Blackboard 문맥 기준으로 다음 Faerin 공격 루프 한 단계를 실행한다.
	bool RunNextLoopStep(UBehaviorTreeComponent& OwnerComp);

	// 진행 중인 루프 단계를 중단하고 내부 상태를 정리한다.
	void CancelCurrentLoopStep();

	// 현재 루프 단계를 새로 시작할 수 있는지 확인한다.
	bool CanRunLoopStep() const;

	// 현재 루프 실행 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinCombatLoopState GetLoopState() const { return LoopState; }

	// 현재 페이즈 설정을 기준으로 공격 후 횡이동 시간을 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	float CalculateStrafeDuration() const;

	// 현재 스프린트 접근 GA가 사용할 요청 데이터를 복사한다.
	bool GetActiveApproachSprintRequest(FPRFaerinApproachSprintRequest& OutRequest) const;

	// 현재 근거리 텔레포트 접근 GA가 사용할 요청 데이터를 복사한다.
	bool GetActiveNearTeleportRequest(FPRFaerinNearTeleportRequest& OutRequest) const;

protected:
	// Owner와 데이터 참조를 다시 확인하고 캐시한다.
	bool InitializeDirector();

	// 현재 BT 문맥에서 선택 가능한 패턴 후보를 고른다.
	bool SelectPatternPlan(UBehaviorTreeComponent& OwnerComp, FPRFaerinPatternPlan& OutPlan);

	// 공통 ThreatComponent가 고른 공격 대상을 Blackboard와 Director 캐시에 동기화한다.
	bool SyncActiveTargetFromThreat(UBehaviorTreeComponent& OwnerComp);

	// 선택된 패턴과 접근/본공격/후속 공격이 같은 대상을 보도록 Threat 타겟을 고정한다.
	bool BeginLoopTargetCommit();

	// 현재 루프에서 잡은 타겟 고정을 해제한다.
	void EndLoopTargetCommit();

	// 거리 때문에 패턴 선택이 실패했을 때 sprint 접근으로 유효 사거리까지 진입한다.
	bool StartOutOfRangeApproach(UBehaviorTreeComponent& OwnerComp);

	// 사거리 밖 접근의 기준이 될 패턴 후보를 고른다.
	bool SelectOutOfRangeApproachPlan(
		UBehaviorTreeComponent& OwnerComp,
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		FPRFaerinPatternPlan& OutPlan);

	// 현재 ASC 상태에서 지정 AbilityTag를 가진 패턴을 활성화할 수 있는지 확인한다.
	bool CanActivatePatternAbilityTag(const FGameplayTag& AbilityTag) const;

	// 현재 PhaseConfig 기준으로 메인 패턴 선택에서 제외해야 하는 Rule인지 확인한다.
	bool ShouldExcludePatternRuleFromMainSelection(
		const FPRPatternRule& PatternRule,
		const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 선택된 패턴 계획을 Teleport VFX 래퍼 또는 Ability 실행으로 시작한다.
	bool StartPatternPlan(
		const FPRFaerinPatternPlan& PatternPlan,
		EPRFaerinObservedAbilityRole ObservedRole = EPRFaerinObservedAbilityRole::Pattern);

	// Phase1/2 공격 전 접근 루트가 필요하면 접근 Ability를 먼저 실행한다.
	EPRFaerinPrePatternApproachStartResult TryStartPrePatternApproach(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		const FPRFaerinPatternPlan& PatternPlan);

	// Phase1/2 공격 전 접근 루트와 배치 방식을 결정한다.
	bool ResolvePrePatternApproachRoute(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		const FPRFaerinPatternPlan& PatternPlan,
		bool& bOutUseNearTeleport,
		EPRFaerinNearTeleportPlacementMode& OutTeleportPlacementMode) const;

	// 공격 전 접근 루트 대상 phase인지 확인한다.
	bool IsPrePatternApproachPhase(EPRBossPhase Phase) const;

	// 선택된 패턴이 Spoke 계열인지 확인한다.
	bool IsSpokePatternPlan(const FPRFaerinPatternPlan& PatternPlan) const;

	// 선택된 패턴이 원거리 선행 텔레포트 계열인지 확인한다.
	bool IsRangedPreApproachPatternPlan(const FPRFaerinPatternPlan& PatternPlan) const;

	// Phase3 이후 본체 공격 전에 Teleport VFX 래퍼를 거쳐야 하는지 확인한다.
	bool ShouldRunTeleportVFXWrapper(const FPRFaerinPatternPlan& PatternPlan) const;

	// Teleport VFX 래퍼를 시작하고 완료 후 원래 패턴 GA를 실행하도록 대기한다.
	bool StartTeleportVFXWrapper(
		const FPRFaerinPatternPlan& PatternPlan,
		EPRFaerinObservedAbilityRole ObservedRole);

	// Teleport VFX 래퍼 완료 콜백을 처리한다.
	void HandleTeleportVFXFinished(bool bSucceeded);

	// Teleport VFX 래퍼 완료 델리게이트를 해제한다.
	void ClearTeleportVFXFinishedDelegate();

	// Owner가 보유한 Teleport VFX 컴포넌트를 반환한다.
	UPRFaerinTeleportVFXComponent* GetTeleportVFXComponent() const;

	// 선택된 패턴 Ability를 서버 ASC에서 실행한다.
	bool ActivatePatternAbility(
		const FPRFaerinPatternPlan& PatternPlan,
		EPRFaerinObservedAbilityRole ObservedRole = EPRFaerinObservedAbilityRole::Pattern);

	// 본 공격 앞에 붙일 보조 포털 패턴을 선택한다.
	bool TrySelectPrePatternPortalAssistPlan(
		UBehaviorTreeComponent& OwnerComp,
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		const FPRFaerinPatternPlan& MainPatternPlan,
		FPRFaerinPatternPlan& OutAssistPlan);

	// 보조 포털 후보로 사용할 PatternGroup을 결정한다.
	FGameplayTag ResolvePrePatternPortalGroupTag(const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 선택된 패턴이 포털 계열인지 확인한다.
	bool IsPortalPatternPlan(const FPRFaerinPatternPlan& PatternPlan) const;

	// Phase3/4 진입 직후 첫 본공격 전에는 보조 포털을 억제해야 하는지 확인한다.
	bool ShouldSuppressPortalAssistForCurrentPhase() const;

	// 첫 본공격 억제 규칙을 적용할 페이즈인지 확인한다.
	bool IsFirstMainPatternPortalSuppressionPhase(EPRBossPhase Phase) const;

	// 페이즈가 바뀌었으면 주기 보조 패턴 스케줄을 초기화한다.
	void RefreshPeriodicSidePatternPhase(EPRBossPhase CurrentPhase);

	// 현재 페이즈에서 실행 가능한 주기 보조 패턴을 메인 공격과 별개로 시도한다.
	void TryActivatePeriodicSidePatterns(const FPRFaerinPhaseLoopConfig& PhaseConfig);

	// 단일 주기 보조 패턴이 실행 가능하면 Ability를 활성화한다.
	bool TryActivatePeriodicSidePattern(const FPRFaerinPeriodicSidePatternConfig& SidePatternConfig);

	// Ability 종료 델리게이트를 연결한다.
	void BindAbilityEndDelegate();

	// Ability 종료 델리게이트를 해제한다.
	void ClearAbilityEndDelegate();

	// 관찰 중인 Ability가 아직 활성 상태인지 확인한다.
	bool IsObservedAbilityActive() const;

	// 관찰 중인 Ability 종료 콜백을 처리한다.
	void HandleObservedAbilityEnded(const FAbilityEndedData& EndedData);

	// 보조 포털이 끝난 뒤 저장해 둔 본 공격을 이어서 실행한다.
	void HandlePrePatternPortalAssistFinished(bool bSucceeded);

	// 공격 전 접근이 끝난 뒤 보관해 둔 본 공격을 이어서 실행한다.
	void HandlePrePatternApproachFinished(bool bSucceeded);

	// 스프린트 접근 종료 후 본 공격을 시작해도 되는 거리까지 들어왔는지 확인한다.
	bool CanStartMainPatternAfterPrePatternApproach() const;

	// Ability 종료 이후 후속 루프 동작을 시작한다.
	void HandlePatternExecutionFinished(bool bSucceeded);

	// ShiftPlayerClose 이후 즉시 Spoke 후속 공격을 실행한다.
	bool TryStartShiftFollowUpSpoke();

	// ShiftPlayerClose 강제 연계 태그를 붙인 상태에서 후속 패턴을 시작한다.
	bool StartForcedFollowUpPatternPlan(const FPRFaerinPatternPlan& PatternPlan);

	// Phase1/2에서 공격 후 strafe만 수행하고 다음 루프에서 패턴을 다시 고르는 흐름인지 확인한다.
	bool ShouldUsePhase12PostPatternStrafeLoop(const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 지정 AbilityTag의 패턴 계획을 강제 후속 실행용으로 만든다.
	bool BuildForcedPatternPlanByTag(
		const FGameplayTag& AbilityTag,
		FPRFaerinPatternPlan& OutPlan,
		bool bIgnoreCooldown = false) const;

	// 공격 후 횡이동이 필요한지 판단한다.
	bool ShouldRunPostPatternStrafe(const FPRFaerinPatternPlan& PatternPlan) const;

	// 현재 거리가 방금 실행한 패턴의 유효 사거리 밖이라 즉시 접근해야 하는지 판단한다.
	bool ShouldRunUrgentPatternRangeApproach(const FPRFaerinPatternPlan& PatternPlan, const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 원작형 공격 후 횡이동을 시작한다.
	bool StartPostPatternStrafe(const FPRFaerinPhaseLoopConfig& PhaseConfig);

	// 횡이동 목적지를 계산한다.
	bool ResolvePostPatternStrafeDestination(const FPRFaerinPhaseLoopConfig& PhaseConfig, FVector& OutDestination) const;

	// 횡이동 타이머가 끝났을 때 호출된다.
	void HandlePostPatternStrafeElapsed();

	// 횡이동 이후 다음 공격을 준비하는 접근이 필요한지 판단한다.
	bool ShouldRunPostStrafeApproach(const FPRFaerinPatternPlan& PatternPlan, const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 횡이동 이후 sprint 접근을 시작한다.
	bool StartPostStrafeApproach(const FPRFaerinPhaseLoopConfig& PhaseConfig);

	// 접근 Ability를 지정 방식으로 실행한다.
	bool StartApproachAbility(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		bool bUseNearTeleport,
		EPRFaerinNearTeleportPlacementMode TeleportPlacementMode,
		EPRFaerinObservedAbilityRole InObservedAbilityRole);

	// 스프린트 접근 GA가 사용할 실행 요청을 만든다.
	bool BuildApproachSprintRequest(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		EPRFaerinObservedAbilityRole InObservedAbilityRole,
		FPRFaerinApproachSprintRequest& OutRequest) const;

	// 현재 접근 역할과 선택 패턴에 맞는 스프린트 종료 거리를 계산한다.
	float ResolveApproachSprintStopDistance(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		EPRFaerinObservedAbilityRole InObservedAbilityRole) const;

	// 근거리 텔레포트 접근 GA가 사용할 실행 요청을 만든다.
	bool BuildNearTeleportRequest(
		const FPRFaerinPhaseLoopConfig& PhaseConfig,
		EPRFaerinNearTeleportPlacementMode PlacementMode,
		FPRFaerinNearTeleportRequest& OutRequest) const;

	// 최종 접근 정책에서 근거리 텔레포트를 사용할지 결정한다.
	bool ShouldUseNearTeleportApproach(EPRFaerinApproachPolicy ApproachPolicy, const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// NearTeleport GA 기본값에서 SprintOrNearTeleport 선택 확률을 읽는다.
	float ResolveNearTeleportSelectionChance(const FGameplayTag& AbilityTag) const;

	// 패턴 메타데이터와 페이즈 설정을 합쳐 최종 접근 정책을 결정한다.
	EPRFaerinApproachPolicy ResolveApproachPolicy(const FPRFaerinPatternPlan& PatternPlan, const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 직전 접근 이후 너무 짧은 시간 안에 다시 접근하려는지 확인한다.
	bool IsApproachSprintRepeatBlocked() const;

	// 현재 접근 요청 캐시를 초기화한다.
	void ClearActiveApproachSprintRequest();

	// 현재 근거리 텔레포트 접근 요청 캐시를 초기화한다.
	void ClearActiveNearTeleportRequest();

	// 전체 루프 단계 타임아웃이 발생했을 때 호출된다.
	void HandleLoopStepFailSafeElapsed();

	// 루프 단계를 완료하고 BT에 결과를 알린다.
	void FinishLoopStep(bool bSucceeded);

	// 다음 틱에 루프 완료를 예약한다.
	void QueueFinishLoopStep(bool bSucceeded);

	// 검증 문제를 로그로 출력한다.
	void LogValidationErrors(const TArray<FString>& Errors) const;

	// Owner의 현재 보스 페이즈를 반환한다.
	EPRBossPhase ResolveCurrentPhase() const;

	// 패턴 선택 결과를 디버그 로그로 남긴다.
	void LogSelectedPattern(const FPRFaerinPatternPlan& PatternPlan, const FPREnemyPatternQueryRuntime& PatternRuntime) const;

private:
	// 현재 Owner의 AIController를 반환한다.
	AAIController* GetOwnerAIController() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss|Faerin")
	FPRFaerinLoopStepFinishedSignature OnFaerinLoopStepFinished;

protected:
	// Faerin 원작형 루프 설정 데이터다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<UPRFaerinCombatLoopDataAsset> LoopDataAsset;

	// BeginPlay 시 PatternData/LoopData 정합성을 1차 점검할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bValidateOnBeginPlay = true;

	// 한 루프 단계가 멈췄을 때 강제로 실패 처리할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float LoopStepFailSafeSeconds = 12.0f;

	// 플레이어가 계속 멀어질 때 접근 Ability가 연속 반복되는 것을 막는 최소 간격이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachSprintRepeatBlockSeconds = 1.25f;

	// 패턴 선택 시 사용할 카테고리 필터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRPatternCategory PatternCategoryFilter = EPRPatternCategory::Any;

	// 패턴 선택 시 사용할 그룹 필터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Pattern.Boss.Faerin"))
	FGameplayTag PatternGroupFilter;

	// Blackboard 현재 타깃 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// Blackboard LOS 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// Blackboard 돌진 경로 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	// Blackboard 전술 상태 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// Blackboard 공격 압박 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

private:
	UPROPERTY(Transient)
	TObjectPtr<APREnemyBaseCharacter> OwnerEnemy;

	UPROPERTY(Transient)
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPRPatternDataAsset> PatternDataAsset;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTarget;

	FPRFaerinPatternPlan ActivePatternPlan;
	FPRFaerinPatternPlan PendingMainPatternPlan;
	FPRFaerinApproachSprintRequest ActiveApproachSprintRequest;
	FPRFaerinNearTeleportRequest ActiveNearTeleportRequest;
	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
	FDelegateHandle TeleportVFXFinishedDelegateHandle;
	FTimerHandle LoopStepFailSafeTimerHandle;
	FTimerHandle PostPatternStrafeTimerHandle;
	EPRFaerinCombatLoopState LoopState = EPRFaerinCombatLoopState::Idle;
	EPRFaerinObservedAbilityRole ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;
	EPRFaerinObservedAbilityRole PendingTeleportVFXAbilityRole = EPRFaerinObservedAbilityRole::None;
	int32 NextStrafeDirectionSign = 1;
	float LastApproachEndTime = -TNumericLimits<float>::Max();
	EPRBossPhase PeriodicSidePatternPhase = EPRBossPhase::Phase1;
	TMap<FGameplayTag, float> PeriodicSidePatternNextAllowedTimes;
	TSet<uint8> RuntimeValidatedPhaseValues;
	bool bHasLoggedInitializationError = false;
	bool bHasPendingMainPatternPlan = false;
	bool bHasPeriodicSidePatternPhase = false;
	bool bHasStartedMainPatternInCurrentPhase = false;
	bool bHasLoopTargetCommit = false;
};
