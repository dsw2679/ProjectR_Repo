// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 다운 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerDown.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UAudioComponent;
class USoundBase;

// 플레이어 멀티플레이 체력 0 이후의 다운 상태를 유지하는 Ability다.
UCLASS()
class PROJECTR_API UPRGA_PlayerDown : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerDown();

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
	// 다운 진입 몽타주가 정상 종료되었을 때 몽타주 태스크 참조를 정리한다.
	UFUNCTION()
	void HandleDownMontageCompleted();

	// 다운 진입 몽타주가 중단되었을 때 몽타주 태스크 참조를 정리한다.
	UFUNCTION()
	void HandleDownMontageInterrupted();

	// 다운 상태 제한 시간을 서버 타이머로 시작한다.
	void StartDownTimer();

	// 다운 상태 제한 시간 타이머를 정리한다.
	void StopDownTimer();

	// 다운 HUD 공유 타이머 시작
	void StartDownTimerInfo(float DurationSeconds);

	// 다운 HUD 공유 타이머 정리
	void ClearDownTimerInfo() const;

	// 서버 월드 시간 반환
	float ResolveServerWorldTimeSeconds() const;

	// 다운 상태 제한 시간이 끝났을 때 다운 사망 전환 확정 이벤트를 보낸다.
	void HandleDownTimeout();

	// 현재 Avatar에게 다운 사망 전환 확정 이벤트를 보낸다.
	void SendDownDeathConfirmedEvent() const;

	// 다운 진입 몽타주 재생 중 이동 차단 태그를 서버에서 추가하거나 제거한다.
	void SetDownEnterMovementBlocked(bool bBlocked);

	// 다운 상태의 최종 사망 태그와 이동 정지를 적용하고 Ability를 종료한다.
	void FinalizeDownDeath();

	// 다운 상태에서 최종 사망 상태 태그를 서버에서 부여하고 복제한다.
	void ApplyDownDeathStateTags();

	// GameMode에 플레이어 다운 진입을 알린다.
	void NotifyDownToGameMode() const;

	// GameMode에 플레이어 최종 사망 진입을 알린다.
	void NotifyDeathToGameMode() const;

	// 다운 진입 몽타주를 재생한다.
	void PlayDownEnterMontage();

	// 다운 전용 오디오 로컬 재생 가능 여부 확인
	bool CanPlayDownAudio() const;

	// 다운 진입 시 전용 BGM과 심장소리 루프 재생
	void PlayDownAudio();

	// 남은 시간이 짧을 때 급박한 심장소리 루프로 전환
	void StartUrgentHeartbeatLoop();

	// 다운 전용 오디오와 전환 타이머 정리
	void StopDownAudio();

	// 지정한 오디오 컴포넌트 페이드아웃 후 정리
	void FadeOutAndClearAudioComponent(TObjectPtr<UAudioComponent>& AudioComponent);

protected:
	// 다운 진입 시 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Montage")
	TObjectPtr<UAnimMontage> DownEnterMontage;

	// 다운에서 최종 사망으로 전환될 때 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Montage")
	TObjectPtr<UAnimMontage> DownToDeathMontage;

	// 다운 진입 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

	// 다운 상태에서 최종 사망까지 기다리는 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down", meta = (ClampMin = "0.0"))
	float DownTimeout = 30.0f;

	// 다운 진입 시 한 번 재생할 BGM
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio")
	TObjectPtr<USoundBase> DownEnterBGM;

	// 다운 상태 중 위험 상황을 알리는 심장소리 루프
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio")
	TObjectPtr<USoundBase> DownHeartbeatLoop;

	// 다운 제한 시간 종료 직전에 재생할 급박한 심장소리 루프
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio")
	TObjectPtr<USoundBase> DownUrgentHeartbeatLoop;

	// 다운 전용 오디오 볼륨 배율
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio", meta = (ClampMin = "0.0"))
	float DownAudioVolumeMultiplier = 1.0f;

	// 다운 전용 오디오 페이드인 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio", meta = (ClampMin = "0.0", Units = "s"))
	float DownAudioFadeInDuration = 0.1f;

	// 다운 전용 오디오 페이드아웃 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio", meta = (ClampMin = "0.0", Units = "s"))
	float DownAudioFadeOutDuration = 0.2f;

	// 최종 사망까지 남은 시간이 이 값 이하일 때 급박한 심장소리 전환
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Down|Audio", meta = (ClampMin = "0.0", Units = "s"))
	float UrgentHeartbeatRemainingTime = 10.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveDownDeathEventTask;

	// 다운 제한 시간 타이머다.
	FTimerHandle DownTimerHandle;

	// 다운 사망 전환 몽타주 종료 보정 타이머다.
	FTimerHandle DownToDeathFinalizeTimerHandle;

	// 급박한 심장소리 전환 타이머
	FTimerHandle UrgentHeartbeatTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DownEnterBGMAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DownHeartbeatAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DownUrgentHeartbeatAudioComponent;

	// 다운에서 사망 전환이 이미 시작되었는지 나타낸다.
	bool bDownToDeathStarted = false;

	// 다운 진입 몽타주가 이동 차단 태그를 직접 추가했는지 나타낸다.
	bool bDownEnterMovementBlockAdded = false;

	// 다운에서 최종 사망 처리가 이미 완료되었는지 나타낸다.
	bool bDownDeathFinalized = false;
};
