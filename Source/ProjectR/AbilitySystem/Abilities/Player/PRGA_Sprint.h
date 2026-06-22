// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 돌진 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Sprint.generated.h"

class UGameplayEffect;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTR_API UPRGA_Sprint : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Sprint();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

private:
	/** 질주 상태를 시작한다 */
	void StartSprint();

	/** 스태미너 회복 딜레이 GameplayEffect를 적용한다 */
	void ApplyStaminaRegenDelay();

	/** 현재 스태미너가 질주 가능한 상태인지 확인한다 */
	bool HasEnoughStaminaToSprint(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 스태미너 변경 콜백을 등록한다 */
	void RegisterStaminaChangeDelegate();

	/** 스태미너 변경 콜백을 해제한다 */
	void UnregisterStaminaChangeDelegate();

	/** 스태미너가 고갈되면 질주를 종료한다 */
	void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);

	/** 질주 중 이동 입력 상실을 감시하는 타이머를 시작한다 */
	void StartMovementInputCheck();

	/** 질주 중 이동 입력 상실 감시 타이머를 정리한다 */
	void StopMovementInputCheck();

	/** 이동 입력이 사라졌는지 확인하고 질주를 종료한다 */
	void HandleMovementInputCheck();

protected:
	/** 질주 종료 후 회복 딜레이를 부여할 GameplayEffect 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Sprint|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffectClass;

	/** 이 값 이하로 스태미너가 내려가면 질주를 종료한다 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Sprint|Stamina")
	float SprintStaminaEndThreshold = 0.0f;

	/** 질주 중 이동 입력 상실 여부를 확인하는 간격 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Sprint|Movement", meta = (ClampMin = "0.01"))
	float MovementInputCheckInterval = 0.05f;

private:
	/** 스태미너 변경 델리게이트 핸들 */
	FDelegateHandle StaminaChangedDelegateHandle;

	/** 이동 입력 상실 감시 타이머 핸들 */
	FTimerHandle MovementInputCheckTimerHandle;

	/** 이번 활성화에서 질주가 실제로 시작되었는지 추적한다 */
	bool bSprintStarted = false;
};
