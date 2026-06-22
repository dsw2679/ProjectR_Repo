// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Groggy 상태/패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyGroggy.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

// State.Groggy 이벤트를 받아 적을 잠시 무력화하는 Ability다.
// 진행 중인 공격 패턴을 취소하고, 몽타주/시간 조건에 따라 그로기를 종료한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyGroggy : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyGroggy();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void HandleGroggyMontageCompleted();

	UFUNCTION()
	void HandleGroggyMontageInterrupted();

	// 그로기 종료 시 태그와 게이지를 함께 복구한다.
	void ResetGroggyState();

	void FinishGroggy(bool bWasCancelled);

protected:
	// 그로기 중 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> GroggyMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bEndWhenMontageEnds = true;

	// 몽타주가 없거나 몽타주보다 긴 무력화 시간이 필요할 때 사용하는 최소 유지 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Groggy", meta = (ClampMin = "0.0"))
	float GroggyDuration = 2.0f;

private:
	FTimerHandle GroggyTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 몽타주 콜백과 타이머가 겹쳐도 종료가 한 번만 처리되도록 한다.
	bool bGroggyFinished = false;
};
