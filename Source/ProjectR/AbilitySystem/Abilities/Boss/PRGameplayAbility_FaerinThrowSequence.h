// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Throw 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinStagedMontagePattern.h"
#include "PRGameplayAbility_FaerinThrowSequence.generated.h"

class UAbilityTask_WaitGameplayEvent;
class UPRFaerinWeaponVisualComponent;

// Faerin Throw 계열을 투사체 스폰이 아니라 실제 검 위치 sweep 판정으로 처리하는 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinThrowSequence : public UPRGameplayAbility_FaerinStagedMontagePattern
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinThrowSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Throw 실행마다 검 sweep 상태와 notify listener를 초기화한 뒤 staged montage 패턴을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Throw 종료 시 melee hit window listener와 검 hitbox 상태를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 지정된 Throw stage가 시작되면 검 판정 참조를 준비한다.
	virtual void NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage) override;

	// PR Enemy Melee Hit Window notify 이벤트를 대기한다.
	void BindMeleeHitWindowEvents();

	// PR Enemy Melee Hit Window notify 이벤트 대기를 종료한다.
	void EndMeleeHitWindowEvents();

	// Throw 검 sweep 피해 구간을 시작한다.
	void BeginThrowHitWindow();

	// Throw 검 sweep 피해 구간을 갱신한다.
	void TickThrowHitWindow();

	// Throw 검 sweep 피해 구간을 종료한다.
	void EndThrowHitWindow();

	// 현재 검 판정 중심점을 계산한다.
	bool GetCurrentBladeTracePoint(FVector& OutTracePoint) const;

	// 이전 검 위치와 현재 검 위치 사이를 sweep해 피해를 적용한다.
	void ApplyThrowDamageTrace(const FVector& TraceStart, const FVector& TraceEnd);

	// Throw 피해를 적용할 수 있는 대상인지 확인한다.
	bool ShouldDamageActor(AActor* CandidateActor) const;

	UFUNCTION()
	void HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload);

protected:
	// 검 판정을 준비할 Throw stage 이름이다. 비워 두면 모든 stage에서 참조만 준비한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw")
	FName ThrowStageName = TEXT("Throw");

	// WeaponVisualComponent가 없을 때 사용할 검 bone/socket 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit")
	FName FallbackBladeBoneName = TEXT("Bone_FN_Weapon_Sword_Blade");

	// 검 판정 중심에 더할 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit")
	FVector HitTraceOffset = FVector::ZeroVector;

	// 검 sweep 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit", meta = (ClampMin = "0.0"))
	float HitTraceRadius = 85.0f;

	// 검 sweep 충돌 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit")
	TEnumAsByte<ECollisionChannel> HitTraceChannel = ECC_Pawn;

	// 현재 위협 대상에게만 피해를 줄지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit")
	bool bOnlyDamageThreatTarget = false;

	// BossStatRow AttackPower에 곱할 Throw 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// Throw가 부여할 고정 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Hit", meta = (ClampMin = "0.0"))
	float PoiseDamage = 10.0f;

	// Throw sweep 디버그를 표시할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Debug")
	bool bDrawDebug = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinWeaponVisualComponent> WeaponVisualComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowBeginEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowTickEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowEndEventTask;

	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	FVector PreviousBladeTracePoint = FVector::ZeroVector;
	bool bThrowHitWindowActive = false;
	bool bHasPreviousBladeTracePoint = false;
};
