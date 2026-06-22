// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (회피 시 스태미나 소모 및 무적 프레임, 구르기 애니메이션 구현)
// Author: 배유찬 (회피 중 플래시라이트 상태 연동)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Animation/PRAnimationTypes.h"
#include "ProjectR/Input/PRActionInputConsumer.h"
#include "PRPlayerDodgeAbility.generated.h"

class ACharacter;
class APRPlayerCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGameplayEffect;
class UPRCameraModifier;

/**
 * 플레이어 회피 Ability다.
 * 입력 방향에 따라 전방 구르기 또는 백스텝을 결정하고, 몽타주 재생과 입력 스킵을 직접 관리한다.
 */
UCLASS()
class PROJECTR_API UPRPlayerDodgeAbility : public UPRGameplayAbility, public IPRActionInputConsumer
{
	GENERATED_BODY()

public:
	UPRPlayerDodgeAbility();

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

	/*~ IPRActionInputConsumer Interface ~*/
	/** 라우터가 전달한 입력을 소비하고, 노티파이 스테이트가 연 스킵 구간이면 회피를 조기 종료한다 */
	virtual bool HandleRoutedActionInput() override;

	/** 회피 몽타주 끝부분의 입력 스킵 가능 구간을 열거나 닫는다 */
	virtual void SetRoutedInputCancelWindow(bool bCanCancel, float BlendOutTime) override;

protected:
	/** 클라이언트가 회피 발동 순간의 방향 정보를 서버에 한 번 전달한다 */
	UFUNCTION(Server, Reliable)
	void Server_SendDodgeDirection(FVector_NetQuantize Direction, bool bHasMovementInput);

private:
	/** 회피 몽타주가 정상 종료되었을 때 Ability를 종료한다 */
	UFUNCTION()
	void HandleDodgeMontageCompleted();

	/** 회피 몽타주가 끊겼을 때 Ability를 취소 종료한다 */
	UFUNCTION()
	void HandleDodgeMontageInterrupted();

	/** 안전 종료 타이머가 현재 회피 실행과 일치하면 Ability를 종료한다 */
	void HandleDodgeSafetyTimeout(int32 FinishedDodgeExecutionId);

	/** 무적 프레임 시작 타이머가 현재 회피 실행과 일치하면 무적을 시작한다 */
	void HandleIFrameStartTimer(int32 FinishedDodgeExecutionId);

	/** 무적 프레임 종료 타이머가 현재 회피 실행과 일치하면 무적을 종료한다 */
	void HandleIFrameEndTimer(int32 FinishedDodgeExecutionId);

	/** 무적 GameplayEffect를 적용한다 */
	void StartIFrame();

	/** 적용 중인 무적 GameplayEffect를 제거한다 */
	void EndIFrame();

	/** 이전 회피 실행에서 남은 타이머를 정리한다 */
	void ClearDodgeTimers();

	/** 회피 방향, 회피 종류, 몽타주 재생, 보조 피드백을 실제로 시작한다 */
	void ExecuteDodge(FVector Direction, bool bHasMovementInput);

	/** 현재 무장 상태와 회피 종류에 맞는 몽타주를 선택한다 */
	UAnimMontage* SelectDodgeMontage(const APRPlayerCharacter* Character, EPRDodgeAnimationType AnimationType) const;

	/** 선택된 회피 몽타주를 GAS 몽타주 태스크로 재생한다 */
	void PlayDodgeMontage(UAnimMontage* DodgeMontage);

	/** 현재 회피 몽타주를 지정한 블렌드 시간으로 정지한다 */
	void StopDodgeMontage(float BlendOutTime);

	/** 회피 종료가 여러 콜백에서 중복 처리되지 않도록 한 번만 EndAbility를 호출한다 */
	void FinishDodge(bool bWasCancelled);

	/** 스태미너 회복 딜레이 GameplayEffect를 적용한다 */
	void ApplyStaminaRegenDelay();

protected:
	/** 회피 종료 후 회복 딜레이를 부여할 GameplayEffect 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffectClass;

	/** 무적 상태를 부여할 GameplayEffect 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
	TSubclassOf<UGameplayEffect> InvulnerableEffectClass;

	/** 무적 프레임이 시작되기 전 지연 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
	float IFrameStartDelay = 0.01f;

	/** 무적 프레임 지속 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
	float IFrameDuration = 0.35f;

	/** 몽타주 종료 콜백이 누락되었을 때 Ability를 끝내는 최대 안전 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
	float DodgeDuration = 3.0f;

	/** 맨손 상태에서 입력 방향 회피 요청 시 재생할 전방 구르기 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> UnarmedForwardRollMontage;

	/** 맨손 상태에서 무입력 회피 요청 시 재생할 백스텝 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> UnarmedBackStepMontage;

	/** 주무기 장착 상태에서 입력 방향 회피 요청 시 재생할 전방 구르기 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> PrimaryForwardRollMontage;

	/** 주무기 장착 상태에서 무입력 회피 요청 시 재생할 백스텝 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> PrimaryBackStepMontage;

	/** 보조무기 장착 상태에서 입력 방향 회피 요청 시 재생할 전방 구르기 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> SecondaryForwardRollMontage;

	/** 보조무기 장착 상태에서 무입력 회피 요청 시 재생할 백스텝 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	TObjectPtr<UAnimMontage> SecondaryBackStepMontage;

	/** 회피 몽타주 재생 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	float DodgeMontagePlayRate = 1.0f;

	/** Ability 종료나 외부 취소로 회피 몽타주를 정지할 때 사용할 기본 블렌드 아웃 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	float DodgeMontageStopBlendOutTime = 0.15f;

	/** 입력 스킵 구간에서 회피를 조기 종료할 때 사용할 기본 블렌드 아웃 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge|Montage")
	float DodgeInputCancelBlendOutTime = 0.18f;

private:
	/** 현재 재생 중인 회피 몽타주 태스크 */
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	/** 현재 Ability가 재생을 요청한 회피 몽타주 */
	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	/** 활성화된 카메라 모디파이어 */
	UPROPERTY()
	TObjectPtr<UPRCameraModifier> ActiveCameraModifier;

	/** 적용 중인 무적 효과 핸들 */
	FActiveGameplayEffectHandle InvulnerableEffectHandle;

	/** 회피 최대 안전 종료 타이머 핸들 */
	FTimerHandle DodgeEndTimerHandle;

	/** 무적 시작 타이머 핸들 */
	FTimerHandle IFrameStartTimerHandle;

	/** 무적 종료 타이머 핸들 */
	FTimerHandle IFrameEndTimerHandle;

	/** 서버가 클라이언트의 회피 방향 데이터를 받았는지 추적한다 */
	bool bServerReceivedDirection = false;

	/** 서버 실행 경로에서 사용할 회피 방향이다 */
	FVector SavedDirection = FVector::ZeroVector;

	/** 서버 실행 경로에서 사용할 이동 입력 여부다 */
	bool bSavedHasMovementInput = false;

	/** 회피 종료가 이미 요청되었는지 추적한다 */
	bool bDodgeEndRequested = false;

	/** 이번 활성화에서 회피가 실제로 시작되었는지 추적한다 */
	bool bDodgeStarted = false;

	/** 현재 입력으로 회피를 스킵할 수 있는 구간인지 추적한다 */
	bool bCanCancelDodgeByInput = false;

	/** 현재 스킵 구간에서 사용할 몽타주 블렌드 아웃 시간이다 */
	float CurrentDodgeInputCancelBlendOutTime = 0.0f;

	/** 회피 실행을 구분하기 위한 누적 번호 */
	int32 DodgeExecutionId = 0;

	/** 현재 진행 중인 회피 실행 번호 */
	int32 ActiveDodgeExecutionId = 0;
};
