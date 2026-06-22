// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Alert 상태/패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGameplayAbility_EnemyBase.h"
#include "TimerManager.h"
#include "PRGameplayAbility_EnemyAlert.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 일반 적이 최초로 플레이어를 발견했을 때 사용하는 Alert 몽타주 Ability이다.
 */
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyAlert : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	/** 기본 태그 차단과 패턴 중단 규칙을 초기화한다. */
	UPRGameplayAbility_EnemyAlert();

protected:
	/*~ UGameplayAbility Interface ~*/
	/** Alert 몽타주를 재생하고 Ability 종료 타이밍을 예약한다. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 타이머와 몽타주 Task를 정리한다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Alert 몽타주 정상 종료를 처리한다. */
	UFUNCTION()
	void HandleAlertMontageCompleted();

	/** Alert 몽타주 중단을 처리한다. */
	UFUNCTION()
	void HandleAlertMontageInterrupted();

	/** Alert Ability를 종료한다. */
	void FinishAlert(bool bWasCancelled);

	/** Alert 연출 중 캐릭터 이동을 멈춘다. */
	void StopAvatarMovement() const;

	/** Alert 후보 애니메이션 중 이번에 재생할 몽타주를 고른다. */
	UAnimMontage* SelectAlertMontage();

protected:
	/** 여러 Alert 몽타주 중 하나를 랜덤으로 재생하기 위한 후보 목록이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project R|Animation")
	TArray<TObjectPtr<UAnimMontage>> AlertMontageCandidates;

	/** Alert 몽타주 재생 속도이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project R|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	/** 몽타주 종료 시 Ability도 종료할지 여부이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project R|Animation")
	bool bEndWhenMontageEnds = true;

	/** 몽타주가 없거나 몽타주보다 더 오래 멈춰야 할 때 사용하는 최소 Alert 유지 시간이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project R|Alert", meta = (ClampMin = "0.0"))
	float AlertDuration = 0.8f;

	/** Alert 시작 시 AI 이동을 즉시 정지할지 여부이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project R|Alert")
	bool bStopMovementOnAlert = true;

private:
	/** Alert 유지 시간 타이머 핸들이다. */
	FTimerHandle AlertTimerHandle;

	/** 현재 실행 중인 Alert 몽타주 Task이다. */
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask = nullptr;

	/** 중복 종료를 막기 위한 플래그이다. */
	bool bAlertFinished = false;
};
