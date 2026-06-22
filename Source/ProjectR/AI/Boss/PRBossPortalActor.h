// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (사격 피격 데칼 및 FX 연동)
// Author: 손승우 (페어린 보스 전투 루프 내 포털 소환 및 투사체 발사 제어 구현)
// Author: 이건주 (포털 액터 파괴 인터페이스 및 대미지 연출 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/Combat/PRDestructableInterface.h"
#include "ProjectR/Combat/PRDirectDamageReceiverInterface.h"
#include "PRBossPortalActor.generated.h"

class APRProjectileBase;
class APRFaerinRainProjectileManager;
class UGameplayEffect;
class USoundBase;
class USphereComponent;

// 포털 하나의 내부 생명주기 상태다.
UENUM(BlueprintType)
enum class EPRBossPortalState : uint8
{
	None			UMETA(DisplayName = "None"),
	Telegraphing	UMETA(DisplayName = "Telegraphing"),
	Active			UMETA(DisplayName = "Active"),
	Firing			UMETA(DisplayName = "Firing"),
	Paused			UMETA(DisplayName = "Paused"),
	Expiring		UMETA(DisplayName = "Expiring"),
	Expired			UMETA(DisplayName = "Expired")
};

// 보스 포털 패턴의 공용 Helper Actor다.
// 기존 텔레그래프/활성/만료 생명주기를 유지하면서 타겟락, 반복 발사, 일시정지를 함께 관리한다.
UCLASS(Blueprintable)
class PROJECTR_API APRBossPortalActor : public APRBossPatternActor, public IPRDestructableInterface
{
	GENERATED_BODY()

public:
	APRBossPortalActor();

	virtual void InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void RequestPatternActorExpire() override;
	virtual void CancelPatternActor() override;
	
	// 2026.06.04 이건주_ 주석처리
	// virtual bool ApplyDirectDamageFromSpec(const FGameplayEffectSpec& DamageSpec, const FHitResult& HitResult) override;
	
	/*~ IPRDestructableInterface Interface ~*/
	virtual bool ReceiveDamageContext(const FPRDestructableDamageReceiveContext& Context) override;

	// 현재 포털이 발사/위험 상태인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	bool IsPortalActive() const { return bPortalActive; }

	// 현재 포털 생명주기 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalState GetPortalState() const { return PortalState; }

	// 포털이 고정해 둔 발사 타겟을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	AActor* GetLockedPortalTarget() const { return LockedTarget; }

	// 서버에서 포털 발사 타겟을 고정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void SetAndLockPortalTarget(AActor* InTarget);

	// 포털 투사체에 전달할 GE Spec을 설정한다.
	void SetPortalProjectileEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec);

	// 서버에서 포털 텔레그래프를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void StartPortalTelegraph();

	// 서버에서 지정 시간 뒤 포털 텔레그래프를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void StartPortalTelegraphAfterDelay(UPARAM(meta = (ClampMin = "0.0")) float Delay);

	// 서버에서 포털을 활성화한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ActivatePortal();

	// 서버에서 포털을 만료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ExpirePortal();

	// 서버에서 포털을 강제 만료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ForceExpirePortal();

	// 서버에서 포털 발사를 일시정지한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void PausePortal();

	// 서버에서 일시정지된 포털 발사를 재개한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void UnpausePortal();

	// 서버에서 포털 투사체를 1회 발사한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void FirePortalProjectile();

	// 이 포털을 경량 rain 투사체(ISM 매니저) 경로로 전환한다. bEnable=false면 기존 액터 투사체 경로를 사용한다.
	// Rain Portal 시퀀스에서 토글이 켜졌을 때만 호출되며, 그 외에는 호출되지 않아 기존 동작이 유지된다.
	void SetLightweightRainProjectile(bool bEnable, APRFaerinRainProjectileManager* InManager, float InLifetime);

	// 포털 발사 타이머를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ClearPortalFireTimer();

	// 포털 투사체 스폰 Transform을 계산한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	bool BuildProjectileSpawnTransform(FTransform& OutTransform) const;

	// 현재 타겟 기준 조준 방향을 계산한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	FVector CalculateProjectileAimDirection() const;

	// 지금 포털 투사체를 발사할 수 있는지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	bool CanFirePortalProjectile() const;

	// 현재 포털 체력을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal|Health")
	float GetCurrentPortalHealth() const { return CurrentPortalHealth; }

	// 최대 포털 체력을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal|Health")
	float GetMaxPortalHealth() const { return MaxPortalHealth; }

	// 현재 포털 체력 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal|Health")
	float GetPortalHealthRatio() const;

	// 2026.06.04 이건주_ 주석처리
	// // 서버 권위에서 포털 체력을 직접 감소시킨다.
	// UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|AI|Boss|Portal|Health")
	// bool ApplyPortalDamage(float DamageAmount, const FHitResult& HitResult);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 복제된 포털 체력 변경을 BP 연출에 전달한다.
	UFUNCTION()
	void OnRep_CurrentPortalHealth(float PreviousHealth);

	// 모든 클라이언트에서 텔레그래프 시작 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalTelegraphStarted();

	// 모든 클라이언트에서 포털 활성 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalActivated();

	// 모든 클라이언트에서 포털 만료 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalExpired();

	// 모든 클라이언트에서 포털 발사 시작 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalFireStarted();

	// 모든 클라이언트에서 포털 투사체 생성 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalProjectileSpawned(APRProjectileBase* SpawnedProjectile);

	// 모든 클라이언트에서 포털 발사 일시정지 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalPaused();

	// 모든 클라이언트에서 포털 발사 재개 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalUnpaused();

	// 모든 클라이언트에서 포털 발사 완료 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalFireSequenceCompleted();

	// 모든 클라이언트에 포털 피격 연출을 요청한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalDamaged(float NewHealth, float PreviousHealth, float DamageAmount, FVector_NetQuantize HitLocation);

	// 모든 클라이언트에 포털 체력 소진 연출을 요청한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalDestroyedByDamage(FVector_NetQuantize HitLocation);

	// 포털 텔레그래프 시작 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalTelegraphStarted();

	// 포털 활성 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalActivated();

	// 포털 만료 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalExpired();

	// 포털 발사 시작 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalFireStarted();

	// 포털 투사체 생성 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalProjectileSpawned(APRProjectileBase* SpawnedProjectile);

	// 포털 일시정지 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalPaused();

	// 포털 재개 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalUnpaused();

	// 포털 발사 완료 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalFireSequenceCompleted();

	// 포털 체력이 변했을 때 BP 피격 연출을 실행한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal|Health")
	void BP_OnPortalHealthChanged(float CurrentHealth, float PreviousHealth, float MaxHealth, float DamageAmount, FVector HitLocation);

	// 포털 체력이 0이 되었을 때 BP 파괴 연출을 실행한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal|Health")
	void BP_OnPortalDestroyedByDamage(FVector HitLocation);

protected:
	// 다음 포털 발사를 예약한다.
	void ScheduleNextPortalFire();

	// 포털 발사 시퀀스를 완료 처리한다.
	void CompletePortalFireSequence();

	// 포털 생명주기 타이머를 모두 정리한다.
	void ClearPortalLifecycleTimers();

	// 포털에서 생성한 투사체에 원작형 충돌/추적 정책을 적용한다.
	void ConfigureSpawnedPortalProjectile(APRProjectileBase* SpawnedProjectile);

	// 포탈 투사체 Spawn payload에 포함할 homing 스케줄을 설정한다.
	void ConfigurePortalProjectileHomingSchedule(APRProjectileBase* SpawnedProjectile);

	// 포털 투사체에 적용할 GE Spec을 만든다.
	FGameplayEffectSpecHandle BuildProjectileEffectSpec() const;

	// 피해 Spec에서 포털 체력에 적용할 피해량을 계산한다.
	float ResolvePortalDamageAmountFromSpec(const FGameplayEffectSpec& DamageSpec) const;

	// 체력 소진으로 포털이 파괴되는 흐름을 실행한다.
	void HandlePortalHealthDepleted(const FHitResult& HitResult);

	// 실제 발사에 사용할 초기 진행 방향을 계산한다.
	FVector CalculateProjectileLaunchDirection() const;

	// 지정된 발사 방향으로 projectile spawn transform을 만든다.
	bool BuildProjectileSpawnTransformFromDirection(const FVector& LaunchDirection, FTransform& OutTransform) const;

protected:
	// 사격 판정을 받는 포털 충돌 영역이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Health")
	TObjectPtr<USphereComponent> PortalDamageCollision;

	// BeginPlay에서 자동으로 텔레그래프를 시작할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bAutoStartPortal = false;

	// 텔레그래프 시작 후 실제 활성까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActivationDelay = 0.75f;

	// 활성 후 만료까지 유지되는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActiveDuration = 14.0f;

	// 만료 이벤트 후 Actor를 제거할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bDestroyWhenExpired = true;

	// 발사 완료 후 포털을 만료할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire")
	bool bDestroyAfterFireComplete = true;

	// Pause 중에도 발사를 허용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire")
	bool bCanFireWhilePaused = false;

	// 포털이 생성(텔레그래프 시작)될 때 포털 위치에 재생할 사운드 큐다. 모든 클라이언트에서 재생된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|SFX")
	TObjectPtr<USoundBase> PortalSpawnSoundCue;

	// 포털이 만료되어 사라질 때 포털 위치에 재생할 사운드 큐다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|SFX")
	TObjectPtr<USoundBase> PortalExpireSoundCue;

	// 포털이 플레이어 피해로 파괴될 때 피격 위치에 재생할 사운드 큐다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|SFX")
	TObjectPtr<USoundBase> PortalDestroyedSoundCue;

	// 포털이 발사할 투사체 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 활성 후 첫 발사까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0.0"))
	float InitialFireDelay = 1.0f;

	// 반복 발사 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0.0"))
	float FireInterval = 4.75f;

	// 포털 하나가 발사할 최대 투사체 수다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0"))
	int32 MaxProjectilesToFire = 3;

	// 투사체 속도 보정값이다. 0이면 투사체 BP 기본값을 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeedOverride = 3500.0f;

	// 타겟 이동 예측 보정 계수다. 원작 값 5는 0.05배 보정으로 해석한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileTargetLead = 5.0f;

	// 추적 투사체 사용 여부다. 1차에서는 스폰 정책 구분용 플래그로만 보관한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	bool bUseTrackingProjectile = true;

	// 추적 투사체 Homing 가속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", EditCondition = "bUseTrackingProjectile"))
	float ProjectileHomingAcceleration = 26000.0f;

	// Homing을 유지할 시간이다. 0 이하면 타격/만료 전까지 계속 추적한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", EditCondition = "bUseTrackingProjectile"))
	float ProjectileHomingDuration = 0.3f;

	// 투사체가 생성된 뒤 homing을 켜기까지 기다리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", EditCondition = "bUseTrackingProjectile"))
	float ProjectileHomingStartDelay = 0.0f;

	// true면 homing projectile의 초기 발사 방향을 포탈 전방 원뿔 안에서 랜덤으로 정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (EditCondition = "bUseTrackingProjectile"))
	bool bUseTrackingConeLaunch = true;

	// homing projectile 초기 발사가 전방 축에서 최소한 이 각도 이상 벗어나도록 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", ClampMax = "90.0", EditCondition = "bUseTrackingProjectile && bUseTrackingConeLaunch"))
	float TrackingLaunchConeMinAngleDegrees = 0.0f;

	// homing projectile 초기 발사가 전방 축에서 최대한 이 각도 이하로 벗어나도록 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", ClampMax = "90.0", EditCondition = "bUseTrackingProjectile && bUseTrackingConeLaunch"))
	float TrackingLaunchConeMaxAngleDegrees = 80.0f;

	// 포털 투사체가 보스/몬스터 계열을 통과하도록 충돌 무시 목록에 등록할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	bool bIgnoreEnemyActorsForProjectile = true;

	// 투사체 스폰 위치의 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	FVector ProjectileSpawnLocalOffset = FVector::ZeroVector;

	// 투사체 스폰 회전 추가 보정값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	FRotator ProjectileRotationOffset = FRotator::ZeroRotator;

	// true면 target 조준 대신 FixedProjectileDirectionLocal을 actor 기준 고정 발사 방향으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	bool bUseFixedProjectileDirection = false;

	// 고정 발사 모드에서 사용할 actor local 방향이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (EditCondition = "bUseFixedProjectileDirection"))
	FVector FixedProjectileDirectionLocal = FVector::ForwardVector;

	// 포털 투사체에 적용할 피해 GE다. 비어 있으면 AbilitySystemRegistry의 적 피해 GE를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Damage")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	// 포털 투사체 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Damage", meta = (ClampMin = "0.0"))
	float ProjectileDamageMultiplier = 1.0f;

	// 포털 투사체가 플레이어에게 적용할 고정 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Damage", meta = (ClampMin = "0.0"))
	float ProjectilePoiseDamage = 0.0f;

	// 플레이어 사격으로 포털 체력을 감소시킬지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Health")
	bool bCanTakePlayerWeaponDamage = true;

	// 포털 최대 체력이다. 플레이어 무기 피해가 이 값을 깎는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Health", meta = (ClampMin = "1.0"))
	float MaxPortalHealth = 120.0f;

	// 플레이어 무기 피해가 포털 체력에 적용될 때 곱하는 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Health", meta = (ClampMin = "0.0"))
	float PlayerWeaponDamageToPortalMultiplier = 1.0f;

	// 현재 포털 내부 생명주기 상태다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalState PortalState = EPRBossPortalState::None;

	// 포털이 고정한 발사 타겟이다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	TObjectPtr<AActor> LockedTarget;

	// 현재 포털 활성 상태다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bPortalActive = false;

	// 서버 권위로 관리되고 클라이언트에 복제되는 포털 현재 체력이다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPortalHealth, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Health")
	float CurrentPortalHealth = 0.0f;

private:
	FTimerHandle PortalTelegraphStartTimerHandle;
	FTimerHandle PortalActivationTimerHandle;
	FTimerHandle PortalExpireTimerHandle;
	FTimerHandle PortalFireTimerHandle;

	FGameplayEffectSpecHandle ProjectileEffectSpecHandle;
	int32 CurrentProjectileFireCount = 0;
	uint32 NextPortalProjectileId = 1;
	bool bPortalHealthDepleted = false;

	// 경량 rain 투사체 경로 사용 여부다. 기본 false(기존 액터 투사체 경로). Rain Portal에서만 켜진다.
	bool bUseLightweightRainProjectile = false;

	// 경량 경로에서 투사체를 등록할 매니저다.
	TWeakObjectPtr<APRFaerinRainProjectileManager> RainProjectileManager;

	// 경량 rain 투사체의 수명(초)이다.
	float LightweightProjectileLifetime = 3.0f;
};
