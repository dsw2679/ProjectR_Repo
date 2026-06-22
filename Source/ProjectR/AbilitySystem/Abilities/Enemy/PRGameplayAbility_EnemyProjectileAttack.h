// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 손승우 (로열 아처 공중 투사체 사격 제어 구현)
// Author: 이건주 (Penitent 원거리 배리어 사격 패턴 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTagContainer.h"
#include "PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyProjectileAttack.generated.h"

class ACharacter;
class APRProjectileBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;

// 일반 적 투사체 공격 공통 Ability다.
// 서버 권위 투사체 스폰, 피해 GE Spec 주입, 공격 타겟 고정을 담당한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyProjectileAttack : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// 일반 적 투사체 공격 Ability를 초기화한다.
	UPRGameplayAbility_EnemyProjectileAttack();

	/*~ UGameplayAbility Interface ~*/
public:
	// 투사체 공격 Ability의 활성화 가능 여부를 확인한다.
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// 투사체 공격 Ability를 활성화하고 몽타주와 발사 타이머를 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 투사체 공격 Ability를 종료하고 타이머와 공격 커밋을 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

public:
	// 애님 노티파이에서 투사체 발사를 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerProjectileFireFromAnimation();

protected:
	// 공격 몽타주 정상 종료를 처리한다.
	UFUNCTION()
	void HandleAttackMontageCompleted();

	// 공격 몽타주 중단을 처리한다.
	UFUNCTION()
	void HandleAttackMontageInterrupted();

	// 투사체 발사 노티파이 이벤트를 처리한다.
	UFUNCTION()
	void HandleProjectileFireGameplayEvent(FGameplayEventData Payload);

	// 이번 공격에서 재생할 공격 몽타주를 선택한다.
	UAnimMontage* SelectAttackMontage() const;

	// 현재 설정으로 투사체 1발을 발사한다.
	virtual void FireProjectile();

	// 스폰된 투사체에 충돌 무시, 호밍 등 공통 후처리를 적용한다.
	virtual void ConfigureSpawnedProjectile(APRProjectileBase* SpawnedProjectile) const;

	// 투사체 초기 호밍 스케줄을 서버와 원격 클라이언트에 동일하게 전달한다.
	virtual void ConfigureProjectileHomingSchedule(APRProjectileBase* SpawnedProjectile) const;

	// 한 번의 발사 이벤트에서 생성할 투사체 수를 반환한다.
	virtual int32 GetProjectileFireCount() const;

	// 다중 투사체 발사 시 인덱스별 스폰 Transform을 계산한다.
	virtual FTransform GetProjectileSpawnTransformForIndex(int32 ProjectileIndex) const;

	// 다중 투사체 발사 시 인덱스별 조준 방향을 계산한다.
	virtual FVector CalculateProjectileAimDirectionForIndex(const FVector& SpawnLocation, int32 ProjectileIndex) const;

	// 투사체에 주입할 적 피해 GE Spec을 생성한다.
	virtual FGameplayEffectSpecHandle BuildProjectileEffectSpec() const;

	// 투사체 스폰 Transform을 계산한다.
	virtual FTransform GetProjectileSpawnTransform() const;

	// 투사체 조준 방향을 계산한다.
	virtual FVector CalculateProjectileAimDirection(const FVector& SpawnLocation) const;

	// 조준 보정에 사용할 투사체 속도를 계산한다.
	float ResolveProjectileSpeedForAiming() const;

	// 현재 공격 타겟을 반환한다.
	AActor* GetCurrentThreatTarget() const;

	// 공격 시작 시점의 타겟을 이번 공격 대상으로 고정한다.
	void BeginThreatAttackCommit();

	// 공격 종료 시점의 타겟 고정을 해제하고 보류 후보를 재평가한다.
	void EndThreatAttackCommit();

	// 현재 타겟 방향으로 액터 회전을 갱신한다.
	void RefreshAttackFacing(bool bApplyActorRotation) const;

	// 공격 종료 처리를 수행한다.
	void FinishProjectileAttack();

	// 설정된 쿨다운 GE를 자기 ASC에 적용한다.
	void ApplyConfiguredCooldown();

private:
	uint32 GenerateProjectileId();

protected:
	// BTTask 활성화용 Ability 식별 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag;

	// 스폰할 일반 적 투사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile")
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 투사체 피해 GE Override. 비어 있으면 Registry의 DamageGE_FromEnemy를 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	// 투사체 발사 소켓 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile")
	FName ProjectileSpawnSocketName = TEXT("Muzzle");

	// 소켓이 없을 때 캐릭터 기준으로 적용할 스폰 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile")
	FVector ProjectileSpawnOffset = FVector(80.0f, 0.0f, 90.0f);

	// 투사체 속도 Override. 0 이하면 투사체 BP 기본 속도를 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeedOverride = 0.0f;

	// 타겟 속도 예측 계수. 0이면 현재 위치만 조준한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileTargetLead = 6.0f;

	// true면 스폰 시점에 월드의 모든 적 캐릭터를 투사체 이동/충돌 무시 대상으로 등록한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile")
	bool bIgnoreEnemyActorsForProjectile = false;

	// true면 발사 직후 일정 시간 동안 타겟을 향한 호밍을 적용한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Homing")
	bool bUseInitialProjectileHoming = false;

	// 초기 호밍 가속도다. 0 이하면 호밍을 적용하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Homing", meta = (ClampMin = "0.0", EditCondition = "bUseInitialProjectileHoming"))
	float InitialProjectileHomingAcceleration = 0.0f;

	// 발사 후 호밍을 시작하기까지의 지연 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Homing", meta = (ClampMin = "0.0", EditCondition = "bUseInitialProjectileHoming"))
	float InitialProjectileHomingStartDelay = 0.0f;

	// 호밍을 유지할 시간이다. 0 이하면 켜진 뒤 계속 유지되므로 Royal Archer에서는 양수로 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Homing", meta = (ClampMin = "0.0", EditCondition = "bUseInitialProjectileHoming"))
	float InitialProjectileHomingDuration = 0.0f;

	// 체력 피해 배율
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// 고정 강인도 피해
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float PoiseDamage = 12.0f;

	// 타이머 기반 발사 fallback용 선딜 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.45f;

	// 발사 이후 Ability 종료까지의 후딜 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.45f;

	// 공격 모션 몽타주 후보 목록
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	// 몽타주 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	FName MontageStartSection = NAME_None;

	// 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// 몽타주 길이를 기준으로 종료 타이밍을 잡을지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	// 몽타주가 있을 때 노티파이 이벤트로 발사할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForFire = true;

	// Ability 시작 시 타겟 방향으로 회전할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bFaceTargetOnAbilityStart = true;

	// 공격 모션 시작 후 EndAbility에서 적용할 쿨다운 GE
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	// 쿨다운 GE가 부여하는 활성화 차단 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cooldown")
	FGameplayTag CooldownBlockedTag;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveProjectileFireEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> SelectedAttackMontage;

	FTimerHandle FireTimerHandle;
	FTimerHandle FinishTimerHandle;

	uint32 NextProjectileId = 1;
	bool bProjectileFired = false;
	bool bProjectileAttackFinished = false;
	bool bWaitingProjectileFireNotify = false;
	bool bCooldownEligibleAfterMontageStart = false;
};
