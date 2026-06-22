// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Barrier Summon 공격 패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_PenitentBarrierSummon.generated.h"

class APRGroundBoxProjectileBase;
class UPRBarrierAbilityDataAsset;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

// Penitent 배리어 소환 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentBarrierSummon : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// Ability 태그와 차단 태그 초기화
	UPRGameplayAbility_PenitentBarrierSummon();

	/*~ UGameplayAbility Interface ~*/
	// 배리어 소환 가능 여부 확인
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// 배리어 소환 몽타주와 노티파이 대기 시작
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 배리어 소환 태스크 정리
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 배리어 소환 노티파이 이벤트 처리
	UFUNCTION()
	void HandleBarrierSummonGameplayEvent(FGameplayEventData Payload);

	// 배리어 소환 몽타주 완료 처리
	UFUNCTION()
	void HandleBarrierSummonMontageCompleted();

	// 배리어 소환 몽타주 중단 처리
	UFUNCTION()
	void HandleBarrierSummonMontageInterrupted();

	// 배리어 소환 실행
	bool ExecuteBarrierSummon();

protected:
	// 배리어 공용 설정 데이터
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	TObjectPtr<UPRBarrierAbilityDataAsset> BarrierData;

	// 배리어 소환 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage")
	TObjectPtr<UAnimMontage> BarrierSummonMontage;

	// 배리어 소환 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

	// 배리어 소환 몽타주 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier|Montage")
	FName MontageStartSection = NAME_None;

private:
	// 배리어 소환 몽타주 태스크
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 배리어 소환 노티파이 대기 태스크
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveBarrierEventTask;

	// 배리어 소환 실행 여부
	bool bBarrierActionExecuted = false;
};
