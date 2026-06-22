// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Barrier Launch 공격 패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_PenitentBarrierLaunch.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UPRBarrierAbilityDataAsset;

// Penitent 배리어 발사 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentBarrierLaunch : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// Ability 태그 초기화
	UPRGameplayAbility_PenitentBarrierLaunch();

	/*~ UGameplayAbility Interface ~*/
	// 배리어 발사 가능 여부 확인
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// 배리어 발사 몽타주와 노티파이 대기 시작
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 배리어 발사 태스크 정리
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 배리어 발사 노티파이 이벤트 처리
	UFUNCTION()
	void HandleBarrierLaunchGameplayEvent(FGameplayEventData Payload);

	// 배리어 발사 몽타주 완료 처리
	UFUNCTION()
	void HandleBarrierLaunchMontageCompleted();

	// 배리어 발사 몽타주 중단 처리
	UFUNCTION()
	void HandleBarrierLaunchMontageInterrupted();

	// 배리어 발사 실행
	bool ExecuteBarrierLaunch();

protected:
	// 배리어 공용 설정 데이터
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	TObjectPtr<UPRBarrierAbilityDataAsset> BarrierData;

	// 배리어 발사 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage")
	TObjectPtr<UAnimMontage> BarrierLaunchMontage;

	// 배리어 발사 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

	// 배리어 발사 몽타주 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage")
	FName MontageStartSection = NAME_None;

private:
	// 배리어 발사 몽타주 태스크
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 배리어 발사 노티파이 대기 태스크
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveBarrierEventTask;

	// 배리어 발사 실행 여부
	bool bBarrierActionExecuted = false;
};
