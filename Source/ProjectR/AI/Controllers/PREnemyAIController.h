// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 기반 탐지 보정 연동)
// Author: 손승우 (일반 몬스터 및 보스 AI 인지 제어 및 비헤이비어 트리 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UBlackboardComponent;
class UBehaviorTree;
class UPRPerceptionConfig;
class UPRCombatMoveDataAsset;
class UPREnemyThreatComponent;
struct FPREnemyMovePresentationConfig;

// 공용 적 AIController
// Perception, Threat, Blackboard, 전투 표현 문맥 적용 담당
UCLASS()
class PROJECTR_API APREnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APREnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// tactical_mode 갱신 및 전투 표현 규칙 적용
	void ApplyTacticalModeState(
		EPRTacticalMode NewMode,
		AActor* FocusTarget = nullptr,
		const FPREnemyMovePresentationConfig* OverridePresentationConfig = nullptr);

	// 전투 이동 표현 문맥 적용
	void ApplyCombatMovePresentationContext(AActor* FocusTarget, const FPREnemyMovePresentationConfig& PresentationConfig);

	// 전투 이동 표현 문맥 해제
	void ClearCombatMovePresentationContext(bool bClearGameplayFocus);

protected:
	/*~ AIPerception Interface ~*/

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/*~ Threat Interface ~*/

	UFUNCTION()
	void HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget);

	// Perception 설정 적용
	void ApplyPerceptionConfig(const UPRPerceptionConfig* Config);

	// 현재 Blackboard tactical_mode 반환
	EPRTacticalMode GetBlackboardTacticalMode() const;

	// last_known_target_location 유효 여부 반환
	bool HasLastKnownTargetLocation() const;

	// Blackboard tactical_mode 값 기록
	void SetBlackboardTacticalMode(EPRTacticalMode NewMode);

	// 현재 소유 Pawn 기준 전투 데이터 자산 반환
	const UPRCombatMoveDataAsset* GetCurrentCombatDataAsset() const;

	// tactical_mode 규칙 또는 override 기반 전투 표현 적용 또는 해제
	void ApplyPresentationForTacticalMode(
		EPRTacticalMode NewMode,
		AActor* FocusTarget,
		const FPREnemyMovePresentationConfig* OverridePresentationConfig = nullptr);

	// 탐지 직후 시작 attack_pressure 적용
	void ApplyInitialAttackPressureOnAlert();

	// 최초 Alert를 주변 일반 몬스터에게 전파한다.
	void PropagateAlertToNearbyEnemies(AActor* AlertTarget);

	// 주변 일반 몬스터의 Alert 전파를 받아 같은 공격 대상을 설정한다.
	bool ReceiveSharedAlert(APREnemyAIController* AlertSourceController, AActor* AlertTarget);

	// 주변 Alert를 받을 수 있는 상태인지 확인한다.
	bool CanReceiveSharedAlert(AActor* AlertTarget) const;

	// 주변 Alert 관련 Blackboard 값을 기록하거나 초기화한다.
	void WriteNearbyAlertBlackboardState(bool bNearbyAlerted, AActor* AlertSourceActor);

	// BehaviorTree에서 Blackboard를 준비하고 캐시
	void CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

	// Threat 바인딩 해제
	void ClearThreatBinding();

protected:
	// AI Perception 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> CachedBlackboardComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPRPerceptionConfig> CachedPerceptionConfig;

	UPROPERTY(Transient)
	TObjectPtr<UPREnemyThreatComponent> CachedThreatComponent;

	// 현재 타겟 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 타겟 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	// 마지막 목격 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName LastKnownTargetLocationKey = TEXT("last_known_target_location");

	// 복귀 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HomeLocationKey = TEXT("home_location");

	// LOS Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// tactical_mode Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// attack_pressure Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 주변 AI Alert 전파를 받아 Alert된 상태인지 기록하는 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName NearbyAIAlertedKey = TEXT("nearby_ai_alerted");

	// 이 AI를 Alert시킨 주변 몬스터를 기록하는 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName NearbyAIAlertSourceKey = TEXT("nearby_ai_alert_source");

	// 타겟 상실 시 처리 정책
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Perception")
	EPRTargetLostPolicy TargetLostPolicy = EPRTargetLostPolicy::ClearCurrentTarget;

	// 다음 타겟 해제 시 Investigate로 이어가기 위한 Alert 유지 플래그
	UPROPERTY(Transient)
	bool bPreserveAlertOnNextTargetClear = false;

	// 주변 Alert 수신으로 발생한 타겟 변경을 다시 전파하지 않기 위한 플래그
	UPROPERTY(Transient)
	bool bSuppressAlertPropagationForSharedAlert = false;

	// 주변 Alert 수신으로 발생한 타겟 변경인지 구분하기 위한 플래그
	UPROPERTY(Transient)
	bool bHandlingSharedAlertTargetChange = false;
};
