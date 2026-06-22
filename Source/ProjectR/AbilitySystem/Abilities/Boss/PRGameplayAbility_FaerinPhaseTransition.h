// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보스 페이즈 전환용 BGM 제어 연동)
// Author: 손승우 (페어린 보스 페이즈 전환 시퀀스 및 AI 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPhaseTransition.h"
#include "PRGameplayAbility_FaerinPhaseTransition.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UBrainComponent;
class UPRFaerinGodFallComponent;
class UPRFaerinGodFallDataAsset;
class APRFaerinCharacter;

// Faerin 전용 PhaseTransition Ability다. Phase2 God Fall 진입과 Phase3 God Fall 속도 강화를 담당한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPhaseTransition : public UPRGameplayAbility_BossPhaseTransition
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPhaseTransition();

	/*~ UGameplayAbility Interface ~*/
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
	// God Fall 시전 위치 도착 후 본체 몽타주 시퀀스를 시작한다.
	void HandleGodFallCastStarted();

	// God Fall entry 종료 결과를 받아 실제 Phase2 commit 여부를 결정한다.
	void HandleGodFallEntryFinished(bool bSucceeded);

	// God Fall 중 Faerin 본체 몽타주를 순서대로 재생한다.
	void PlayNextBodyMontage();

	// 현재 본체 몽타주 task를 정리한다.
	void ClearBodyMontageTask();

	// God Fall entry와 본체 몽타주가 모두 끝났을 때 Phase2 전환을 확정한다.
	void TryCommitPhase2GodFallTransition();

	// God Fall 전용 연출 중 BT 이동이 보스 위치를 덮어쓰지 못하도록 AI 로직을 멈춘다.
	void PauseOwnerBrainForPhaseTransition(APRFaerinCharacter* FaerinCharacter);

	// God Fall 전용 연출이 끝나면 멈춘 AI 로직을 재개한다.
	void ResumeOwnerBrainForPhaseTransition();

	// PhaseTransition 실패 또는 취소 시 기존 phase로 되돌리고 transition 상태를 정리한다.
	void RollbackPhaseTransition();

	// 실제 phase 확정 전 BGM에 사용할 목표 phase를 예고한다.
	void BroadcastBGMPhasePreview(EPRBossPhase PreviewPhase);

	// PhaseTransition 이벤트 payload에서 목표 phase를 해석한다.
	EPRBossPhase ResolveBossPhaseFromEvent(const FGameplayEventData* TriggerEventData) const;

	// God Fall 본체 몽타주 재생 완료 시 다음 몽타주로 넘어간다.
	UFUNCTION()
	void HandleBodyMontageCompleted();

	// God Fall 본체 몽타주가 중단되면 phase transition을 취소한다.
	UFUNCTION()
	void HandleBodyMontageInterrupted();

	// Phase2->3, Phase3->4 전환용 단독 몽타주를 시작한다.
	bool TryStartStandardPhaseTransitionMontage(APRFaerinCharacter* FaerinCharacter);

	// 목표 phase에 맞는 전환 몽타주 슬롯을 반환한다.
	UAnimMontage* ResolveStandardPhaseTransitionMontage() const;

	// 대기 중인 phase 전환을 확정하고 후속 phase 스케일링을 반영한다.
	bool CommitPendingPhaseTransition();

	// Phase2에서 Phase3으로 넘어갈 때 재생할 별도 전환 몽타주 슬롯이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PhaseTransition|Animation")
	TObjectPtr<UAnimMontage> Phase2To3TransitionMontage;

	// Phase3에서 Phase4로 넘어갈 때 재생할 별도 전환 몽타주 슬롯이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PhaseTransition|Animation")
	TObjectPtr<UAnimMontage> Phase3To4TransitionMontage;

	// Phase2->3, Phase3->4 전환 몽타주 공통 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PhaseTransition|Animation", meta = (ClampMin = "0.01"))
	float StandardTransitionMontagePlayRate = 1.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinGodFallComponent> ActiveGodFallComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveBodyMontageTask;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimMontage>> BodyMontageQueue;

	TWeakObjectPtr<UBrainComponent> PausedBrainComponent;
	EPRBossPhase PendingTargetPhase = EPRBossPhase::Phase1;
	int32 ActiveBodyMontageIndex = INDEX_NONE;
	bool bPhaseTransitionCommitted = false;
	bool bGodFallCastStarted = false;
	bool bGodFallEntrySucceeded = false;
	bool bBodyMontageSequenceFinished = false;
	bool bPausedBrainLogicForPhaseTransition = false;
	bool bSuppressBodyMontageCallbacks = false;
	bool bBGMPhasePreviewed = false;
	bool bStandardPhaseTransitionMontageActive = false;
};
