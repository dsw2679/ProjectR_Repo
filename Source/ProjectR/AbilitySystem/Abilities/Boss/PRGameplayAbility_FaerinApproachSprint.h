// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 접근 돌진 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "PRGameplayAbility_FaerinApproachSprint.generated.h"

class AAIController;
class APREnemyBaseCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

// Faerin 공격 후 스프린트 접근 액션을 Enter/Loop/End 몽타주 섹션과 거리 종료 조건으로 실행한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinApproachSprint : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinApproachSprint();

	/*~ UGameplayAbility Interface ~*/
public:
	// Director가 넘긴 접근 요청을 읽고 sprint Enter/Loop 몽타주와 추적 이동을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 진행 중인 이동, 타이머, 몽타주 Task, 이동 표현 컨텍스트를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 소유 Faerin Director에서 이번 접근 요청 데이터를 가져온다.
	bool ResolveApproachRequest(APREnemyBaseCharacter* EnemyCharacter);

	// 접근 몽타주를 재생하고 Enter->Loop 섹션 연결을 설정한다.
	bool PlaySprintMontage();

	// Enter/Loop 섹션이 자연스럽게 이어지도록 몽타주 섹션 연결을 설정한다.
	void ConfigureSprintLoopSections() const;

	// 접근 이동과 거리/시간 종료 타이머를 시작한다.
	void StartApproachMovement();

	// Enter 섹션 길이를 고려한 실제 timeout 지연 시간을 계산한다.
	float ResolveTimeoutDelaySeconds() const;

	// 주기적으로 타깃 거리를 확인하고 목적지를 갱신한다.
	void TickApproachMovement();

	// 접근 시간이 초과됐을 때 End 섹션으로 전환한다.
	void HandleApproachTimeoutElapsed();

	// 현재 타깃을 향해 런지 방식의 직접 이동을 수행한다.
	bool MoveDirectlyTowardTarget(float DeltaTime);

	// 현재 타깃 방향을 기준으로 burst 돌진 방향을 갱신한다.
	bool UpdateBurstDirectionFromTarget();

	// sprint 접근의 실제 이동 속도를 계산한다.
	float ResolveSprintMoveSpeed() const;

	// 스프린트 접근을 멈추고 End 섹션으로 전환한다.
	void BeginEndSection(bool bWasCancelled);

	// Ability를 정상/취소 상태로 마무리한다.
	void FinishApproachSprint(bool bWasCancelled);

	// 접근 중 사용하는 타이머들을 모두 정리한다.
	void ClearApproachTimers();

	// 이동 요청과 Gameplay 포커스를 정리한다.
	void StopApproachMovement() const;

	// 거리 종료 판정을 위해 현재 타깃까지의 2D 거리를 계산한다.
	float CalculateDistanceToTarget() const;

	UFUNCTION()
	void HandleSprintMontageCompleted();

	UFUNCTION()
	void HandleSprintMontageBlendOut();

	UFUNCTION()
	void HandleSprintMontageInterrupted();

protected:
	// Enter/Loop/End 섹션을 포함한 sprint 접근 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> SprintMontage;

	// 접근 시작 애니메이션 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	FName EnterSectionName = TEXT("Enter");

	// 접근 중 반복 재생할 애니메이션 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	FName LoopSectionName = TEXT("Loop");

	// 거리 조건 충족 또는 시간 초과 시 전환할 종료 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	FName EndSectionName = TEXT("End");

	// sprint 접근 몽타주의 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	// 거리 종료 판정을 갱신하는 주기다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.01"))
	float DistanceCheckInterval = 0.016f;

	// 루트모션 없이 캡슐을 직접 전진시킬 sprint 이동 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float SprintMoveSpeed = 2200.0f;

	// 이동 방향 계산에서 높이 차이를 제거할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement")
	bool bConstrainMovementToGround = true;

	// Enter 직후 바로 End로 넘어가지 않도록 보장하는 최소 loop 유지 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float MinimumLoopDurationBeforeTimeout = 1.25f;

	// TeleportLunge처럼 타깃 방향을 추적하는 초반 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float DirectionTrackingDuration = 0.18f;

	// 추적 이후 고정 방향으로 돌진을 유지하는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float StraightBurstDuration = 0.72f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCombatDirectorComponent> ActiveDirectorComponent;

	FPRFaerinApproachSprintRequest ActiveRequest;
	FTimerHandle DistanceCheckTimerHandle;
	FTimerHandle TimeoutTimerHandle;
	FVector CachedBurstDirection = FVector::ZeroVector;
	float LastMovementUpdateTime = 0.0f;
	float BurstElapsedSeconds = 0.0f;
	bool bHasCachedBurstDirection = false;
	bool bApproachFinished = false;
	bool bEndingSprint = false;
	bool bPendingEndCancelled = false;
};
