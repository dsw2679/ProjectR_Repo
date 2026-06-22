// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 및 전투 상태 기반 위협도 보정 연동)
// Author: 손승우 (AI 어그로 관리 및 위협도 기반 타겟 선정 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "TimerManager.h"
#include "PREnemyThreatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRTargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);

// 몬스터의 타겟 후보와 현재 공격 대상을 서버에서 관리하는 컴포넌트다.
// 1차 리팩터링에서는 기존 최고 Threat 선택 동작을 유지하되, Targeting 확장을 위한 후보/공격 커밋 상태를 함께 보관한다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyThreatComponent();

	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// 공격 커밋 중이면 고정된 공격 대상을, 아니면 현재 타겟을 반환한다.
	AActor* GetAttackTarget() const;

	// 현재 공격 커밋에 고정된 타겟을 반환한다.
	AActor* GetActiveAttackTarget() const { return AttackCommitState.ActiveAttackTarget; }

	// 현재 공격 커밋에 고정된 타겟 위치를 반환한다.
	const FVector& GetActiveAttackTargetLocation() const { return AttackCommitState.ActiveAttackTargetLocation; }

	// 공격 커밋으로 타겟 전환을 막고 있는지 확인한다.
	bool IsAttackCommitted() const { return AttackCommitState.bIsAttackCommitted; }

	// 현재 시점에 current_target을 즉시 교체할 수 있는지 확인한다.
	bool CanSwitchCurrentTarget() const;

	// 타겟 후보 유지와 선택 정책을 런타임 설정으로 적용한다.
	void SetTargetingConfig(const FPREnemyTargetingConfig& InTargetingConfig);

	// 현재 적용 중인 타겟 후보 유지와 선택 정책을 반환한다.
	const FPREnemyTargetingConfig& GetTargetingConfig() const { return TargetingConfig; }

	// 현재 후보 목록을 반환한다.
	const TArray<FPREnemyTargetCandidate>& GetTargetCandidates() const { return TargetCandidates; }

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddThreat(AActor* Target, float Amount);

	// 피해를 준 플레이어를 공격 후보에 등록하고 현재 공격 대상 전환을 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddDamageThreat(AActor* Target, float DamageAmount);

	// 지정한 후보를 현재 공격 대상으로 강제 선택한다. 공격 커밋 중이면 보류 후보로 저장된다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ForceCurrentTarget(AActor* NewTarget);

	// Perception이 플레이어를 감지했을 때 공격 후보 상태를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void UpdatePerceivedTarget(AActor* Target, FVector SensedLocation, bool bHasLOS);

	// Perception이 플레이어를 놓쳤을 때 후보 유지 상태를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void MarkTargetPerceptionLost(AActor* LostTarget, FVector LastKnownLocation);

	// 후보 정보를 외부에서 읽을 수 있게 복사해 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	bool GetTargetCandidate(AActor* Target, FPREnemyTargetCandidate& OutCandidate) const;

	// 후보 유지 반경과 만료 시간을 기준으로 더 이상 유효하지 않은 후보를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void RefreshTargetCandidates();

	// 현재 공격 대상을 이번 공격의 고정 대상으로 잠근다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void BeginAttackCommit(AActor* InTarget, FVector InTargetLocation);

	// 공격 잠금을 해제하고 보류된 타겟 전환을 재평가한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void EndAttackCommit();

	// 사망/그로기처럼 공격이 강제로 끊길 때 잠금 상태를 즉시 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ForceClearAttackCommit();

	// 현재 타겟이 죽거나 유효하지 않게 되었을 때 다음 후보로 교체한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void InvalidateCurrentTarget();

	// 현재 타겟만 비워 수색/복귀 BT가 동작하게 한다. Threat 기록은 유지한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ReleaseCurrentTargetForSearch(AActor* LostTarget);

	// Perception에서 타겟을 잃었다는 신호가 왔을 때 위협 목록을 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void OnTargetLost(AActor* LostTarget);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI")
	FPRTargetChangedSignature OnTargetChanged;

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ 05.28 김동석 PRCombatEngageMentSubystem 을 통한 플레이어 전투상태 알림 ~*/
	// 전투 교전 후보 보고 타이머 시작
	void StartCombatEngagementReportTimer();

	// 전투 교전 후보 보고 타이머 정리
	void StopCombatEngagementReportTimer();

	// 유지 중인 전투 후보를 집계 서브시스템에 보고
	void ReportCombatEngagedCandidates();

	// 전투 교전 후보로 보고 가능한 대상 여부 확인
	bool ShouldReportCombatEngagedCandidate(const FPREnemyTargetCandidate& Candidate) const;

	bool IsValidThreatTarget(const AActor* Target) const;
	// 후보 풀에 유지할 수 있는 '살아있는 플레이어 객체'인지만 검사한다. (다운/사망 등 일시 상태는 제외하지 않는다)
	bool IsTrackablePlayerCandidate(const AActor* Target) const;
	// 진단용: 현재 타겟이 무효 처리되어 비워질 때, IsValidThreatTarget의 어떤 조건이 false였는지 로그로 남긴다.
	void LogThreatTargetInvalidReason(const AActor* Target, const TCHAR* Context) const;
	void ReevaluateTarget(bool bResetScoreWindow = true);
	void SetCurrentTarget(AActor* NewTarget, float CandidateScore = 0.0f);
	void CleanupInvalidEntries();
	// 10초 점수 창이 만료되었는지 확인하고, 필요할 때 피해 누적 점수를 초기화한다.
	bool ResetScoreWindowIfNeeded(float CurrentTime, bool bClearDamageScores = true);
	// 현재 점수 창에 누적된 피해 점수를 다음 창을 위해 초기화한다.
	void ClearScoreWindowDamage();
	// 후보의 피해/거리/LOS/현재 타겟 유지 점수를 다시 계산한다.
	float UpdateCandidateSelectionScore(FPREnemyTargetCandidate& Candidate) const;
	// 최종 선택 점수를 가중치로 사용해 다음 공격 대상을 확률 선택한다.
	bool SelectWeightedTarget(AActor*& OutTarget, float& OutScore) const;
	FPREnemyTargetCandidate* FindTargetCandidate(AActor* Target);
	const FPREnemyTargetCandidate* FindTargetCandidate(AActor* Target) const;
	FPREnemyTargetCandidate& FindOrAddTargetCandidate(AActor* Target, float CurrentTime);
	bool ShouldRemoveTargetCandidate(const FPREnemyTargetCandidate& Candidate, float CurrentTime) const;
	bool ShouldRetainPlayerCandidate(const FPREnemyTargetCandidate& Candidate) const;
	bool ShouldRetainPlayerTargetOnExplicitLoss(AActor* Target) const;
	bool IsOwnerPhaseTransitioning() const;
	void QueuePendingTarget(AActor* NewTarget, float CandidateScore);
	void ClearPendingTarget();
	// 타겟 후보/현재 타겟/커밋 상태를 로그로 출력한다.
	void LogTargetDebugState(const TCHAR* Reason) const;
	// 타겟 후보/현재 타겟/커밋 상태를 월드에 그린다.
	void DrawTargetDebugState(const TCHAR* Reason) const;

protected:
	// 타겟 후보 유지와 선택 정책이다. 기본값은 안전한 fallback이며, 일반 몬스터는 CombatDataAsset 값으로 덮어쓴다.
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|AI")
	FPREnemyTargetingConfig TargetingConfig;

	// 현재 공격 후보 목록이다.
	UPROPERTY()
	TArray<FPREnemyTargetCandidate> TargetCandidates;

	// BT/Ability가 참조하는 현재 공격 대상이다.
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget = nullptr;

	// 공격 하나가 진행 중일 때 타겟 변경을 보류하기 위한 상태다.
	UPROPERTY()
	FPREnemyAttackCommitState AttackCommitState;

	// 05.28 김동석추가 
	// 전투 교전 후보 보고 주기
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.1"))
	float CombatEngagementReportInterval = 1.0f;

	FTimerHandle CombatEngagementReportTimerHandle;

	float LastSwitchTime = -1000.0f;

	float LastScoreWindowResetTime = -1000.0f;
};
