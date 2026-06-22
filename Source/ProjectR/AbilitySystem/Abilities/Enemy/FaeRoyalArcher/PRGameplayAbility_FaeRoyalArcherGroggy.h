// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Groggy 상태/패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyGroggy.h"
#include "PRGameplayAbility_FaeRoyalArcherGroggy.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

// Fae Royal Archer 전용 그로기 Ability다.
// 공중 전투/Perch 표현 상태를 정리한 뒤 원작형 피격/낙하 몽타주 후보를 선택한다.
// 원작처럼 Impact -> Fall -> Loop -> Recover 형태의 섹션 체인 몽타주를 선택적으로 제어할 수 있다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaeRoyalArcherGroggy : public UPRGameplayAbility_EnemyGroggy
{
	GENERATED_BODY()

public:
	// Royal Archer 그로기 연출 기본값을 초기화한다.
	UPRGameplayAbility_FaeRoyalArcherGroggy();

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 사용할 그로기 몽타주 후보를 결정한다.
	UAnimMontage* ResolveGroggyMontageCandidate() const;

	// 그로기 몽타주 재생 전에 Royal Archer 전용 전투 표현 상태를 정리한다.
	void PrepareRoyalArcherGroggyPresentation() const;

	// Royal Archer 전용 그로기 종료 시간을 계산한다.
	float CalculateRoyalArcherGroggyFinishDelay(bool bHasGroggyMontage) const;

	// 현재 선택된 몽타주를 섹션 체인 방식으로 제어할지 확인한다.
	bool ShouldUseSequencedGroggySections(const UAnimMontage* SelectedMontage) const;

	// 섹션 체인 몽타주가 끝나기 전에 Recover 섹션으로 넘긴다.
	void ScheduleGroggyRecoverSectionJump(float FinishDelay);

	UFUNCTION()
	void HandleRoyalArcherGroggyMontageCompleted();

	UFUNCTION()
	void HandleRoyalArcherGroggyMontageInterrupted();

	UFUNCTION()
	void JumpToGroggyRecoverSection();

	void FinishRoyalArcherGroggy(bool bWasCancelled);

protected:
	// GroggyMontage보다 우선 사용할 원작형 그로기/피격 몽타주 후보 목록이다.
	// 단일 시퀀스 몽타주뿐 아니라 Impact/Fall/Loop/Recover 섹션을 가진 체인 몽타주도 넣을 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher")
	TArray<TObjectPtr<UAnimMontage>> GroggyMontageCandidates;

	// true면 후보 목록에서 무작위로 선택하고, false면 첫 번째 유효 후보를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher")
	bool bRandomizeGroggyMontageCandidate = true;

	// true면 선택된 그로기 몽타주를 원작형 섹션 체인으로 본다.
	// 권장 섹션 구성: Impact -> Fall -> Loop -> Recover.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher|SequencedGroggy")
	bool bUseSequencedGroggySections = false;

	// 그로기 유지 중 반복/정지 포즈로 사용할 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher|SequencedGroggy", meta = (EditCondition = "bUseSequencedGroggySections"))
	FName GroggyLoopSectionName = TEXT("Loop");

	// 그로기 종료 직전에 넘어갈 회복 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher|SequencedGroggy", meta = (EditCondition = "bUseSequencedGroggySections"))
	FName GroggyRecoverSectionName = TEXT("Recover");

	// Ability 종료 몇 초 전에 Recover 섹션으로 점프할지 결정한다.
	// Recover 섹션 길이에 맞춰 BP에서 조정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher|SequencedGroggy", meta = (EditCondition = "bUseSequencedGroggySections", ClampMin = "0.0"))
	float GroggyRecoverLeadTime = 0.45f;

	// 섹션 체인 몽타주가 자연 종료되었을 때 그로기 Ability도 즉시 종료할지 결정한다.
	// Loop 섹션을 쓰는 원작형 체인에서는 false 권장.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher|SequencedGroggy", meta = (EditCondition = "bUseSequencedGroggySections"))
	bool bAllowSequencedMontageCompletionToEndGroggy = false;

private:
	FTimerHandle RoyalArcherGroggyTimerHandle;
	FTimerHandle RoyalArcherGroggyRecoverTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> RoyalArcherActiveMontageTask;

	bool bRoyalArcherGroggyFinished = false;
};
