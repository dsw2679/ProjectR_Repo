// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 피격 경직 시퀀스 및 화면 이펙트 구현)
// Author: 배유찬 (플래시라이트 상태 연동)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRGA_PlayerHitReact.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGameplayEffect;
class APRPlayerCharacter;

UENUM(BlueprintType)
enum class EPRPlayerHitReactType : uint8
{
	None,
	Weak,
	Strong,
	Down
};

enum class EPRPlayerDownHitReactPhase : uint8
{
	None,
	Start,
	FallLoop,
	Land,
	GetUp
};

// 플레이어 강인도 임계값 도달 이벤트를 받아 경직 몽타주와 행동 제한을 처리하는 Ability다.
UCLASS()
class PROJECTR_API UPRGA_PlayerHitReact : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerHitReact();

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
	// 피격 리액션 몽타주가 정상 종료되었을 때 입력 제한을 해제하고 Ability를 종료한다.
	UFUNCTION()
	void HandleHitReactMontageCompleted();

	// 피격 리액션 몽타주가 끊겼을 때 입력 제한을 해제하고 Ability를 종료한다.
	UFUNCTION()
	void HandleHitReactMontageInterrupted();

	// 다운 몽타주의 섹션 전환을 감지하고 착지 여부에 따라 다음 섹션을 결정한다.
	void HandleDownMontageSectionChanged(UAnimMontage* Montage, FName SectionName, bool bLooped);

	// 이벤트 태그를 피격 리액션 타입으로 변환한다.
	EPRPlayerHitReactType ResolveHitReactType(const FGameplayEventData* TriggerEventData) const;

	// 리액션 타입에 맞는 몽타주를 선택한다.
	UAnimMontage* SelectHitReactMontage(EPRPlayerHitReactType HitReactType) const;

	// 현재 무기 타입에 맞는 약한 경직 몽타주를 선택한다.
	UAnimMontage* SelectWeakHitReactMontage() const;
	
	// 현재 무기 타입에 맞는 강한 경직 몽타주를 선택한다.
	UAnimMontage* SelectStrongHitReactMontage() const;

	// 리액션 타입에 맞춰 기존 플레이어 행동을 취소한다.
	void CancelActionsForHitReact(EPRPlayerHitReactType HitReactType);

	// 회복 가능 체력 회복 딜레이 GameplayEffect를 적용한다.
	void ApplyRecoverableHealthRecoveryDelay(const FGameplayEventData* TriggerEventData);

	// 다운 리액션의 초기 상태를 설정한다.
	void StartDownHitReact();

	// 다운 몽타주가 끝날 때까지 HitReact 재발동을 막는 쿨다운 태그를 부여한다.
	void StartDownHitReactCooldown();

	// 다운 몽타주 재생 중 부여한 HitReact 쿨다운 태그를 제거한다.
	void ClearDownHitReactCooldown();

	// 다운 리액션에서 사용하는 몽타주 섹션 연결과 전환 콜백을 설정한다.
	void ConfigureDownMontageFlow();

	// 다운 낙하 반복 섹션이 착지 전까지 유지되도록 섹션 연결을 갱신한다.
	void RefreshDownMontageSectionFlow();

	// 다운 몽타주가 재생 중일 때 바닥 감지를 시작한다.
	void StartDownGroundCheck();

	// 다운 바닥 감지를 중지한다.
	void StopDownGroundCheck();

	// 다운 몽타주가 재생 중이면 바닥 감지를 갱신한다.
	void CheckDownGround();

	// 다운 몽타주가 현재 재생 중인지 반환한다.
	bool IsDownMontagePlaying() const;

	// 캡슐 아래에 착지할 바닥이 있는지 반환한다.
	bool HasGroundBelowForDownHitReact() const;

	// 다운 착지 섹션 진입을 요청한다.
	void RequestDownLand();

	// 다운 리액션 상태와 이벤트 구독을 정리한다.
	void ClearDownHitReact();

	// 다운 몽타주의 지정 섹션으로 이동한다.
	void JumpToDownSection(FName SectionName);

	// 다운 몽타주에 필요한 섹션들이 모두 있는지 확인한다.
	bool HasRequiredDownMontageSections(const UAnimMontage* Montage) const;

	// 강한 경직과 다운에서 몽타주 재생 동안 사용할 입력 제한 태그를 부여한다.
	void StartActionLock(EPRPlayerHitReactType HitReactType);

	// 입력 제한 태그를 제거한다.
	void ClearActionLock();

	// 피격 리액션 Ability를 한 번만 종료한다.
	void FinishHitReact(bool bWasCancelled);

protected:
	// 맨손 상태에서 사용할 약한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> UnarmedWeakHitReactMontage;
	
	// 맨손 상태에서 사용할 강한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> UnarmedStrongHitReactMontage;

	// 무기 타입별 약한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TMap<EPRWeaponType, TObjectPtr<UAnimMontage>> WeakHitReactMontages;
	
	// 무기 타입별 강한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TMap<EPRWeaponType, TObjectPtr<UAnimMontage>> StrongHitReactMontages;

	// 다운에서 사용할 섹션 기반 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> DownHitReactMontage;

	// 다운 시작 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownStartSectionName = TEXT("Start");

	// 다운 낙하 반복 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownFallLoopSectionName = TEXT("FallLoop");

	// 다운 착지 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownLandSectionName = TEXT("Land");

	// 다운 기상 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownGetUpSectionName = TEXT("GetUp");

	// 피격 리액션 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

	// Ability 종료나 외부 취소 시 몽타주를 정지할 블렌드 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage", meta = (ClampMin = "0.0"))
	float MontageStopBlendOutTime = 0.15f;

	// 유효 피격 시 회복 가능 체력 자동 회복을 지연시킬 GameplayEffect 클래스다.
	// TODO: 당장은 PlayerHitreact가 발동시, 지연 이펙트도  넣으나 피해를 입어도 경직이 발생하지 않을 경우 이 로직을 다른곳으로 분리해야함
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|RecoverableHealth")
	TSubclassOf<UGameplayEffect> RecoverableHealthRecoveryDelayEffectClass;

private:
	// 현재 재생 중인 피격 리액션 몽타주 태스크다.
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 섹션 제어와 종료 정리에 사용할 현재 피격 리액션 몽타주다.
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitReactMontage;

	// 몽타주, 무기 상태, 캡슐 바닥 감지에 접근할 플레이어 캐릭터다.
	TWeakObjectPtr<APRPlayerCharacter> ActivePlayerCharacter;

	// 다운 FallLoop 구간에서 바닥 감지를 반복하는 타이머다.
	FTimerHandle DownGroundCheckTimerHandle;

	// 현재 처리 중인 피격 리액션 타입이다.
	EPRPlayerHitReactType ActiveHitReactType = EPRPlayerHitReactType::None;

	// 다운 몽타주 안에서 현재 머무는 섹션 단계다.
	EPRPlayerDownHitReactPhase DownHitReactPhase = EPRPlayerDownHitReactPhase::None;

	// 몽타주 콜백과 Ability 종료가 중복 실행되는 것을 막는다.
	bool bHitReactFinished = false;

	// FallLoop에서 Land 섹션으로 넘어가야 하는지 나타낸다.
	bool bDownLandRequested = false;

	// Strong, Down 행동불능 태그를 이 Ability가 직접 부여했는지 나타낸다.
	bool bActionLockTagAdded = false;

	// 다운 리액션 재발동 방지 태그를 이 Ability가 부여했는지 나타낸다.
	bool bDownHitReactCooldownTagAdded = false;
	
	// Strong, Down 중 이 Ability가 Ability 차단을 적용했는지 나타낸다.
	bool bActionLockAbilityBlockApplied = false;
};
