// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (사망 시 아이템/재화 드롭 시퀀스 연동 구현)
// Author: 배유찬 (보스 사망 시 게이트/장벽 비활성화 제어 구현)
// Author: 손승우 (몬스터 사망 디졸브 연출 및 사망 판정 공용화 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyDeath.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UNiagaraSystem;
class UTexture;

// Death Dissolve 시작 기준이다. 기본값은 기존 동작처럼 몽타주 종료 이후다.
UENUM(BlueprintType)
enum class EPRDeathDissolveTimingMode : uint8
{
	// Death Montage가 끝난 뒤 지정 지연 시간 후 Dissolve를 시작한다.
	AfterMontageEnd UMETA(DisplayName = "After Montage End"),

	// Death Montage가 시작된 뒤 지정 지연 시간 후 Dissolve를 시작한다.
	AfterMontageStart UMETA(DisplayName = "After Montage Start")
};

// State.Dead 이벤트를 받아 적을 사망 상태로 고정하고, 사망 몽타주와 삭제 타이밍을 관리하는 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyDeath : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyDeath();

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
	UFUNCTION()
	void HandleDeathMontageCompleted();

	UFUNCTION()
	void HandleDeathMontageInterrupted();

	// 사망 연출 이후 Avatar Actor를 제거한다.
	void DestroyDeathAvatar();

	// DeathDestroyDelay 또는 별도 연출 지연 뒤 Actor 제거를 예약한다.
	void ScheduleDeathDestroy(float Delay);

	// 사망 시각 연출을 모든 클라이언트에 요청한다.
	void RequestDeathDissolveVisual();

	// 서버 사망 확정 시 드롭 매니저에 드롭 처리를 요청한다.
	void RequestMonsterDrop(const FGameplayEventData* TriggerEventData);

	// 몽타주와 Dissolve 길이를 포함해 Actor 삭제까지 필요한 시간을 계산한다.
	float CalculateDeathDestroyDelay() const;

	// 사망 Ability 종료 처리를 한 번만 수행한다.
	void FinishDeath();

protected:
	// 사망 중 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 사망 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// Dissolve를 사용하지 않는 경우 몽타주 종료 시 Ability를 종료할지 결정한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bEndAbilityWhenMontageEnds = false;

	// true면 CharacterMovement를 DisableMovement로 잠가 사망 이후 움직임을 막는다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death")
	bool bDisableMovementOnDeath = true;

	// true면 사망 연출 이후 Actor를 제거한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death")
	bool bDestroyActorOnDeath = true;

	// Dissolve를 사용하지 않을 때 Actor 제거까지 대기할 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death", meta = (ClampMin = "0.0"))
	float DeathDestroyDelay = 3.0f;

	// true면 사망 몽타주 마지막 자세에서 메시 Dissolve를 진행한 뒤 Actor를 제거한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve")
	bool bUseDissolveOnDeath = false;

	// Dissolve 시작 기준이다. 기본값은 기존 공용 적처럼 Death Montage 종료 이후다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	EPRDeathDissolveTimingMode DissolveTimingMode = EPRDeathDissolveTimingMode::AfterMontageEnd;

	// DissolveTimingMode가 AfterMontageEnd일 때, 몽타주 마지막 자세 고정 후 Dissolve 시작까지 대기할 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath", ClampMin = "0.0"))
	float DissolveDelayAfterMontage = 0.0f;

	// DissolveTimingMode가 AfterMontageStart일 때, Death Montage 시작 후 Dissolve 시작까지 대기할 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath", ClampMin = "0.0"))
	float DissolveDelayAfterMontageStart = 0.65f;

	// Dissolve가 진행되는 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath", ClampMin = "0.0"))
	float DissolveDuration = 1.0f;

	// Dissolve 시작 시 재질 파라미터 값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	float DissolveStartValue = 0.0f;

	// Dissolve 종료 시 재질 파라미터 값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	float DissolveEndValue = 1.0f;

	// Dissolve 재질에서 사용하는 Scalar Parameter 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	FName DissolveScalarParameterName = TEXT("DissolveAmount");

	// Dissolve 중 재생할 Niagara 시스템이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	TObjectPtr<UNiagaraSystem> DissolveNiagaraSystem;

	// Niagara에 전달할 Dissolve 진행도 User Parameter 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	FName NiagaraDissolveParameterName = TEXT("User.DissolveAmount");

	// Dissolve Niagara가 사용하는 노이즈 텍스처다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	TObjectPtr<UTexture> DissolveTexture;

	// Dissolve Niagara 노이즈 텍스처 UV 스케일이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath"))
	FVector2D DissolveTextureUV = FVector2D(1.0f, 1.0f);

	// Dissolve 보간 타이머 간격이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Dissolve", meta = (EditCondition = "bUseDissolveOnDeath", ClampMin = "0.001"))
	float DissolveTickInterval = 0.016f;

private:
	FTimerHandle DeathDestroyTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 몽타주 콜백이 여러 번 들어와도 사망 종료 처리를 한 번만 수행한다.
	bool bDeathFinished = false;
};
