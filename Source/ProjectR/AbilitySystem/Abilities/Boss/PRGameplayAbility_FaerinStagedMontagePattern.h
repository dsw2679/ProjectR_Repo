// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Staged Montage Pattern 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRGameplayAbility_FaerinStagedMontagePattern.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UEnvQuery;
class UPRFaerinCharacterEventRouterComponent;

// Faerin 스테이지형 패턴에서 보스 이동 목적지를 계산하는 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinStageDestinationMode : uint8
{
	None		UMETA(DisplayName = "None"),
	TargetOffset	UMETA(DisplayName = "Target Offset"),
	EnvQuery	UMETA(DisplayName = "EnvQuery")
};

// Faerin 스테이지 진입 시 보스 위치를 보정하기 위한 목적지 설정이다.
USTRUCT(BlueprintType)
struct FPRFaerinStagedMontageDestinationConfig
{
	GENERATED_BODY()

	// 목적지 계산 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination")
	EPRFaerinStageDestinationMode DestinationMode = EPRFaerinStageDestinationMode::None;

	// TargetOffset 방식 또는 EQS 실패 fallback에서 타겟 기준으로 적용할 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination")
	FVector TargetLocalOffset = FVector::ZeroVector;

	// 목적지 산출에 사용할 EQS이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery"))
	TObjectPtr<UEnvQuery> DestinationQuery;

	// EQS 실행 방식이다. 후보 랜덤 선택 모드에서는 내부에서 AllMatching으로 보정된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery"))
	TEnumAsByte<EEnvQueryRunMode::Type> QueryRunMode = EEnvQueryRunMode::SingleResult;

	// EQS 결과 후보 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery"))
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS Named Float Param 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery"))
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 상위 후보 최대 선택 개수다. 0 이하면 개수 제한을 두지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery", ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery", ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;

	// EQS 실패 시 TargetLocalOffset 방식으로 대체할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|EQS",
		meta = (EditCondition = "DestinationMode == EPRFaerinStageDestinationMode::EnvQuery"))
	bool bFallbackToTargetOffsetWhenQueryFails = true;

	// 목적지를 지면에 투영할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground")
	bool bProjectToGround = true;

	// 지면 투영 실패 시 스테이지를 실패 처리할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground",
		meta = (EditCondition = "bProjectToGround"))
	bool bRequireGroundProjection = false;

	// 지면 트레이스 시작 높이 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground",
		meta = (EditCondition = "bProjectToGround", ClampMin = "0.0"))
	float GroundTraceUpOffset = 500.0f;

	// 지면 트레이스 하향 길이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground",
		meta = (EditCondition = "bProjectToGround", ClampMin = "0.0"))
	float GroundTraceDownOffset = 1500.0f;

	// 지면 투영 결과에 추가로 더할 위치 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground",
		meta = (EditCondition = "bProjectToGround"))
	FVector GroundLocationOffset = FVector::ZeroVector;

	// 지면 투영에 사용할 트레이스 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Destination|Ground",
		meta = (EditCondition = "bProjectToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;
};

// Faerin 패턴을 구성하는 하나의 몽타주 스테이지다.
USTRUCT(BlueprintType)
struct FPRFaerinStagedMontageStage
{
	GENERATED_BODY()

	// 디버그와 BP 확장 이벤트에서 식별할 스테이지 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage")
	FName StageName = NAME_None;

	// 해당 스테이지에서 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Animation")
	TObjectPtr<UAnimMontage> Montage;

	// 몽타주 시작 섹션이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Animation")
	FName MontageStartSection = NAME_None;

	// 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// 원작 AnimSet의 StartTime처럼 몽타주를 중간 시간부터 시작할 때 사용하는 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Animation", meta = (ClampMin = "0.0"))
	float MontageStartTimeSeconds = 0.0f;

	// 스테이지 시작 전에 이동을 멈출지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Movement")
	bool bStopMovementBeforeStage = true;

	// 스테이지 시작 전에 현재 타겟을 바라보도록 회전할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Movement")
	bool bFaceTargetBeforeStage = true;

	// 스테이지 시작 전에 보스 위치를 목적지로 이동할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Movement")
	bool bApplyDestinationBeforeStage = false;

	// 보스 위치 이동 목적지 설정이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Movement",
		meta = (EditCondition = "bApplyDestinationBeforeStage"))
	FPRFaerinStagedMontageDestinationConfig DestinationConfig;

	// 원작 StateElapsedCondition처럼 몽타주 종료 전에 고정 시간으로 다음 stage에 진입할지 여부이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Timing")
	bool bAdvanceAfterFixedTime = false;

	// stage 시작 후 다음 stage로 넘어갈 고정 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Timing",
		meta = (EditCondition = "bAdvanceAfterFixedTime", ClampMin = "0.0"))
	float FixedAdvanceTime = 0.0f;

	// 고정 시간 전환 시 현재 몽타주 task를 종료할지 여부이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Timing",
		meta = (EditCondition = "bAdvanceAfterFixedTime"))
	bool bStopMontageOnFixedAdvance = true;

	// 몽타주 종료가 아니라 CharacterEvent를 받아야 다음 스테이지로 넘어갈지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Event")
	bool bWaitForCharacterEventToAdvance = false;

	// 다음 스테이지 진입을 허용하는 CharacterEvent 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Event",
		meta = (EditCondition = "bWaitForCharacterEventToAdvance"))
	FName AdvanceEventName = NAME_None;

	// CharacterEvent 대기 제한 시간이다. 0 이하면 몽타주 종료 시 이벤트 미수신을 실패로 처리한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Event",
		meta = (EditCondition = "bWaitForCharacterEventToAdvance", ClampMin = "0.0"))
	float EventWaitTimeout = 0.0f;
};

// Faerin TeleportDown / Crash / Throw처럼 여러 몽타주 스테이지를 순서대로 실행하는 공용 패턴 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinStagedMontagePattern : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinStagedMontagePattern();

	/*~ UGameplayAbility Interface ~*/
public:
	// 스테이지 배열의 첫 스테이지부터 Faerin 패턴을 실행한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 진행 중인 몽타주 태스크, 이벤트 대기, 라우터 등록을 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Faerin CharacterEvent Router에 스테이지 이벤트 listener를 등록한다.
	bool RegisterCharacterEventListener();

	// 등록된 CharacterEvent listener를 제거한다.
	void UnregisterCharacterEventListener();

	// 스테이지 설정상 CharacterEvent Router가 필요한지 확인한다.
	bool RequiresCharacterEventListener() const;

	// Faerin CharacterEvent를 받아 현재 스테이지의 진행 조건을 평가한다.
	void HandleFaerinCharacterEvent(FName EventName);

	// 지정 인덱스의 스테이지를 실행한다.
	bool PlayStage(int32 StageIndex);

	// 다음 스테이지로 진행하거나 전체 패턴을 종료한다.
	void AdvanceToNextStage();

	// 현재 스테이지의 목적지를 계산하고 보스를 이동시킨다.
	bool ApplyStageDestination(const FPRFaerinStagedMontageStage& Stage);

	// 스테이지 목적지 설정에 맞춰 최종 목적지를 계산한다.
	bool ResolveStageDestination(const FPRFaerinStagedMontageDestinationConfig& DestinationConfig, FVector& OutDestination) const;

	// 현재 타겟 기준 로컬 오프셋 목적지를 계산한다.
	bool ResolveTargetOffsetDestination(const FVector& TargetLocalOffset, FVector& OutDestination) const;

	// EQS 기반 목적지를 계산한다.
	bool ResolveQueryDestination(const FPRFaerinStagedMontageDestinationConfig& DestinationConfig, FVector& OutDestination) const;

	// 목적지를 지면에 투영한다.
	bool ProjectDestinationToGround(const FPRFaerinStagedMontageDestinationConfig& DestinationConfig,
		const FVector& InDestination,
		FVector& OutDestination) const;

	// 현재 타겟을 바라보도록 보스 회전을 맞춘다.
	void FaceCurrentTarget() const;

	// 보스 CharacterMovement를 즉시 정지시킨다.
	void StopBossMovement() const;

	// 현재 스테이지의 CharacterEvent 대기 타이머를 시작한다.
	void StartStageEventTimeout(const FPRFaerinStagedMontageStage& Stage);

	// CharacterEvent 대기 타이머를 제거한다.
	void ClearStageEventTimeout();

	// CharacterEvent 대기 시간이 초과되었을 때 패턴을 실패 처리한다.
	void HandleStageEventTimeout();

	// 원작 StateElapsedCondition 대응용 고정 시간 전환 타이머를 시작한다.
	void StartStageFixedAdvanceTimer(const FPRFaerinStagedMontageStage& Stage);

	// 고정 시간 전환 타이머를 정리한다.
	void ClearStageFixedAdvanceTimer();

	// 고정 시간이 지나면 다음 stage로 전환한다.
	void HandleStageFixedAdvance();

	// 패턴 Ability를 정상 또는 취소 상태로 종료한다.
	void FinishStagedPattern(bool bWasCancelled);

	// C++ 파생 패턴이 스테이지 시작 시점에 추가 로직을 실행하는 확장 지점이다.
	virtual void NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage);

	// C++ 파생 패턴이 CharacterEvent 수신 시점에 추가 로직을 실행하는 확장 지점이다.
	virtual void NativeOnStageCharacterEvent(const FPRFaerinStagedMontageStage& Stage, FName EventName);

	// 스테이지 시작 시 BP 보조 연출을 연결할 수 있는 확장 지점이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Stage")
	void BP_OnStageStarted(FName StageName);

	// 스테이지 진행 중 CharacterEvent를 BP 보조 연출에 전달하는 확장 지점이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Stage")
	void BP_OnStageCharacterEvent(FName StageName, FName EventName);

	// 현재 스테이지 몽타주가 정상 종료되었을 때 다음 진행을 판단한다.
	UFUNCTION()
	void HandleStageMontageCompleted();

	// 현재 스테이지 몽타주가 자연스럽게 블렌드아웃되었을 때 다음 진행을 판단한다.
	UFUNCTION()
	void HandleStageMontageBlendOut();

	// 현재 스테이지 몽타주가 중단되었을 때 패턴을 취소한다.
	UFUNCTION()
	void HandleStageMontageInterrupted();

protected:
	// 순서대로 실행할 Faerin 몽타주 스테이지 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage")
	TArray<FPRFaerinStagedMontageStage> MontageStages;

	// 스테이지 이벤트가 없어도 CharacterEvent Router를 필수로 요구할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Stage|Event")
	bool bRequireCharacterEventRouter = false;

	// 목적지 계산/이동 디버그를 표시할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug")
	bool bDrawDebugDestination = false;

	// 목적지 디버그 표시 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug",
		meta = (EditCondition = "bDrawDebugDestination", ClampMin = "0.0"))
	float DebugDrawDuration = 2.0f;

private:
	FTimerHandle StageEventTimeoutTimerHandle;
	FTimerHandle StageFixedAdvanceTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCharacterEventRouterComponent> ActiveEventRouter;

	int32 ActiveStageIndex = INDEX_NONE;
	bool bStagedPatternFinished = false;
	bool bAdvancingStageByFixedTimer = false;
};
