// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (투사체 궤적 바운스(반사), 탄약 소모 연동 및 멀티플레이 동기화 구현)
// Author: 손승우 (적 AI 전용 사격 충돌 물리 및 피격 연동 구현)
// Author: 이건주 (특수 충돌 사운드(SFX) 및 파괴 가능 보스 포털 충돌 판정 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "PRProjectileTypes.h"
#include "PRProjectileBase.generated.h"

struct FGameplayEffectSpecHandle;
class UPRProjectileMovementComponent;
class USceneComponent;
class USphereComponent;
class USoundBase;

struct FPRPredictedProjectileBounceRecord
{
	// 예측 바운스 순서 번호
	uint8 BounceIndex = 0;

	// 예측 바운스 직후 위치
	FVector Location = FVector::ZeroVector;

	// 예측 바운스 직후 회전
	FRotator Rotation = FRotator::ZeroRotator;

	// 예측 바운스 직후 속도
	FVector Velocity = FVector::ZeroVector;
};

enum class EPRPredictedBounceMatchResult : uint8
{
	Missing,
	ExactIndex,
	LocationFallback,
};

UENUM()
enum class EPRProjectileRole : uint8
{
	Predicted,
	Auth,
};

UENUM(BlueprintType)
enum class EPRProjectileFinalImpactPolicy : uint8
{
	// 최종 충돌 시 파괴
	Destroy,
	// 최종 충돌 위치 정지 유지
	StopAndStay,
};

/** 
 * TODO: 발사한 Client는 예측을 사용하고, Remote Client는 Spawn 위치를 동기화 받기 때문에 둘 모두 총구 근처에서부터 발사되지만,
 * 서버는 여전히 FastForward로 인해 총구보다 멀리서부터 발사되는 것처럼 보인다.
 * 리슨서버이기에 모든 클라이언트의 투사체에 대해 시각용 액터를 따로 소환하는 것은 비용이 클 수 있으므로 우선 리슨 서버측의 부자연스러움은 감수하고 추후 방안 모색 (나이아가라 분리 등)
 * 
 * Projectile Type:
 * ProjectileRole이 Auth이면서 HasAuthority() == true인 경우 Server initiated projectile
 * ProjectileRole이 Auth이면서 HasAuthority() == false인 경우 Replicated
 * ProjectileRole이 Predicted인 경우 Client측 Local Predicted Actor (이 액터의 HasAuthority는 true)
 */
UCLASS()
class PROJECTR_API APRProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	APRProjectileBase();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 스폰 직후 1회 호출. 역할/ID 부여 (FinishSpawning 이전 호출 권장)
	void InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId);
	
	// 스폰 직후 권위 투사체에 대해 GE 초기화
	void InitGameplayEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec);

	// 서버가 직접 생성한 투사체의 초기 진행 방향과 속도를 설정한다.
	void SetProjectileInitialVelocity(const FVector& Direction, float SpeedOverride = 0.0f);

	// 투사체 이동 컴포넌트 기본 속도 조회
	float GetProjectileInitialSpeed() const;

	// 서버가 직접 생성한 투사체의 Homing 타겟을 설정한다.
	void ConfigureProjectileHoming(USceneComponent* HomingTargetComponent, float HomingAcceleration);

	// 서버 권위 투사체의 Spawn 복제 payload에 담을 Homing 스케줄을 설정한다.
	void ConfigureProjectileHomingSchedule(AActor* HomingTargetActor, float HomingAcceleration, float StartDelay, float Duration);

	// 투사체 이동/충돌에서 지정 Actor를 무시하도록 등록한다.
	void AddProjectileIgnoredActor(AActor* ActorToIgnore);

	// 서버 권위 투사체를 클라이언트 발사 시점까지 전진. HasAuthority() && bUseFastForward인 경우만 실행
	void ApplyFastForward(float ForwardSeconds);

	// 서버에서 이벤트 발생 시 RepMovement를 Push. HasAuthority()인 경우만 실행
	void PushRepMovement(EPRRepMovementEvent Event);

	// 투사체 이동 컴포넌트가 실제 바운스 처리 직후 호출하는 동기화 기록 지점
	void NotifyProjectileBounce();
	
	// 식별자 접근
	uint32 GetProjectileId() const { return ProjectileId; }

	// 역할 접근
	EPRProjectileRole GetProjectileRole() const { return ProjectileRole; }

	// Predicted 여부
	bool IsPredicted() const { return ProjectileRole == EPRProjectileRole::Predicted; }

	// 링크된 카운터파트 조회. Predicted면 Auth, Auth면 Predicted
	APRProjectileBase* GetLinkedCounterpart() const { return LinkedCounterpart.Get(); }

	UFUNCTION(BlueprintCallable)
	void DestroyProjectile(EPRProjectileDestroyReason DestroyReason = EPRProjectileDestroyReason::Impact);
	
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void OnProjectileDestroyed();

	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void OnProjectileDestroyEffectStarted(EPRProjectileDestroyReason DestroyReason);

	// 최종 충돌 정지 시 서버 권위 BP 확장 지점
	UFUNCTION(BlueprintAuthorityOnly, BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void OnProjectileStoppedAtImpact(const FHitResult& Hit);
	
	UFUNCTION(BlueprintPure)
	bool HasProjectileAuthority() const;
	
	UFUNCTION(BlueprintPure)
	bool GetShouldBounce() const;
	
protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnAuthLinked(APRProjectileBase* AuthProjectile);
	
	// ProjectileId 리플리케이션 콜백. 소유 클라이언트에서만 호출됨 (COND_OwnerOnly)
	UFUNCTION()
	void OnRep_ProjectileId();

	// RepMovement OnRep 콜백. SimulatedProxy에서 이벤트별 처리
	UFUNCTION()
	void OnRep_RepMovement();

	// SimulatedProxy: Spawn 이벤트 처리. 언히든 + 위치 설정 + 시뮬 시작
	void HandleRepSpawn();

	// SimulatedProxy: Bounce/Detonation 이벤트 처리. 위치/속도 스냅
	void HandleRepCorrection();

	// SimulatedProxy: 최종 충돌 정지 이벤트 처리
	void HandleRepStop();

	// SimulatedProxy: 최종 충돌 낙하 이벤트 처리
	void HandleRepFall();

	// 콜리전 히트 이벤트. 서버에서만 판정 처리
	UFUNCTION()
	virtual void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Overlap으로 들어오는 투사체 충돌을 Hit와 동일한 피해 경로로 처리한다.
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 투사체가 맞았지만 실제 명중/파괴로 처리하지 않을 대상을 판정한다.
	virtual bool ShouldIgnoreProjectileHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit) const;
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* TargetActor);
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTargetWithHit(AActor* TargetActor, const FHitResult& InHitResult);

private:
	// Predicted-Auth 양방향 링크
	void LinkCounterpart(APRProjectileBase* InCounterpart);

	// 예측 투사체 바운스 기록을 서버 Bounce 이벤트와 비교하여 필요한 경우 보정
	bool ReconcilePredictedBounceFromServer(const FPRProjectileRepMovement& ServerMovement);

	// 예측 투사체의 현재 바운스 이동 상태를 큐에 기록
	void RecordPredictedBounce();

	// 예측 바운스 기록 중 서버 BounceIndex 또는 위치 허용치와 일치하는 항목을 찾아 소모
	EPRPredictedBounceMatchResult ConsumePredictedBounceRecord(
		uint8 ServerBounceIndex,
		const FVector& ServerLocation,
		float LocationTolerance,
		FPRPredictedProjectileBounceRecord& OutRecord);

	// 서버 Bounce 이벤트 기준으로 위치와 선택적 속도 보정
	void ApplyServerBounceCorrection(const FPRProjectileRepMovement& ServerMovement, bool bCorrectLocation, bool bCorrectVelocity);

	// 숨겨진 소유 클라이언트 Auth 투사체를 예측 투사체의 현재 상태와 동기화
	void SyncAuthPresentationToPredicted(APRProjectileBase* PredictedProjectile);
	
	// 소유 클라이언트에서 Auth 액터 도착 시 매니저의 Predicted와 매칭하여 링크
	void TryLinkToPredictedOnClient();

	// Spawn payload에 포함된 Homing 스케줄을 현재 월드 타이머로 실행한다.
	void StartProjectileHomingSchedule(const FPRProjectileRepHomingSchedule& HomingSchedule);

	// Homing 스케줄의 시작 시점에 실제 이동 컴포넌트 Homing을 켠다.
	void ApplyProjectileHomingScheduleStart(FPRProjectileRepHomingSchedule HomingSchedule);

	// Homing 스케줄의 종료 시점에 실제 이동 컴포넌트 Homing을 끈다.
	void StopProjectileHomingSchedule();

	// Homing 스케줄 타이머를 정리한다.
	void ClearProjectileHomingScheduleTimers();

	// 현재 클라이언트 인스턴스가 서버 투사체의 원격 프레젠테이션인지 확인한다.
	bool IsRemoteAuthProjectilePresentation() const;

	// 클라이언트 remote 투사체의 GodFall 방식 로컬 homing 프레젠테이션을 시작한다.
	void StartClientProjectileHomingPresentation(const FPRProjectileRepHomingSchedule& HomingSchedule);

	// 클라이언트 remote 투사체의 로컬 homing 프레젠테이션을 갱신한다.
	void UpdateClientProjectileHomingPresentation(float DeltaSeconds);

	// 클라이언트 remote 투사체의 로컬 homing 프레젠테이션을 정리한다.
	void StopClientProjectileHomingPresentation();

	// 클라이언트 remote 투사체가 로컬 homing 프레젠테이션을 시작하도록 알린다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartProjectileHomingPresentation(
		AActor* HomingTargetActor,
		float HomingAcceleration,
		float StartDelay,
		float Duration,
		uint8 Revision);
	
	// 서버 권위 최종 충돌 정책 처리
	void HandleFinalImpact(const FHitResult& Hit);

	// 최종 충돌 위치 정지 유지
	void StopAndStayAtImpact(const FHitResult& Hit);

	// 최종 충돌 후 중력 낙하 시작
	bool StartFinalImpactFall();

	// 최종 충돌 낙하 후 정착
	void SettleFinalImpactFall(const FHitResult& Hit);

	// 정지 상태 공통 처리
	void StopProjectileMotion();

	// 최종 충돌 후 유지 시간 적용
	void ApplyFinalImpactStayLifeSpan();

	void DrawDebugs(float DeltaSeconds);
	
protected:
	// Fast-Forward 사용 여부. false면 ApplyFastForward 호출 시 무시
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	bool bUseFastForward = true;

	// Auth가 앞에 있을 때 보간 속도 (따라잡기)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float SyncInterpSpeedCatchUp = 30.f;

	// Auth가 뒤에 있을 때 보간 속도 (감속). 작을수록 천천히 늦춰짐
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float SyncInterpSpeedSlowDown = 10.f;

	// 서버 바운스와 예측 바운스가 같은 것으로 간주되는 위치 오차 허용 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float BounceCorrectionToleranceDistance = 50.0f;

	// 서버 바운스와 예측 바운스가 같은 것으로 간주되는 속도 오차 허용 크기
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float BounceCorrectionToleranceVelocity = 100.0f;

	// 바운스 보정 시 서버 속도를 예측 투사체에 반영할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	bool bUseBounceVelocityCorrection = true;

	// 최종 충돌 처리 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|Impact")
	EPRProjectileFinalImpactPolicy FinalImpactPolicy = EPRProjectileFinalImpactPolicy::Destroy;

	// 최종 충돌 후 유지 시간, 0이면 무제한 유지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|Impact", meta = (ClampMin = "0.0"))
	float FinalImpactStayDuration = 20.0f;
	
	// ====== Components =====
	// 루트. ProjectileMovementComponent의 UpdatedComponent
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Projectile")
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Projectile")
	TObjectPtr<UPRProjectileMovementComponent> ProjectileMovementComponent;
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayEffectSpecHandle EffectSpecHandle;
	
	UPROPERTY(BlueprintReadOnly)
	UAbilitySystemComponent* InstigatorASC;

	// 파괴 오디오
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|Sound")
	USoundBase* ExplodeSound;

	// 투사체가 날아가는 동안 재생할 비행 사운드 큐다. 루트에 attach되어 투사체를 따라 재생된다. 비어 있으면 재생하지 않는다.
	// (루프형 큐를 쓰면 투사체가 파괴될 때까지 따라 재생되고, 파괴 시 함께 정리된다.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|Sound")
	TObjectPtr<USoundBase> FlightSoundCue;
	
private:
	// 투사체 식별자. Auth 액터에 한해 소유 클라이언트로만 리플리케이트
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileId)
	uint32 ProjectileId = 0;

	// 이벤트 드리븐 이동 동기화.
	UPROPERTY(ReplicatedUsing = OnRep_RepMovement)
	FPRProjectileRepMovement RepMovement;

	// 본 인스턴스의 역할 (네트워크 리플리케이트 대상 아님. 인스턴스 단위 결정)
	UPROPERTY(BlueprintReadOnly, Meta = (AllowPrivateAccess = true))
	EPRProjectileRole ProjectileRole = EPRProjectileRole::Auth; // Auth를 기본값으로 하여야 복제된 투사체 초기화시 Auth확인 가능

	// 카운터파트 약참조 (Predicted-Auth 상호 링크)
	TWeakObjectPtr<APRProjectileBase> LinkedCounterpart;
	
	bool bIsLinked = false;
	bool bIsRemoteProjectile = false;
	bool bShouldSyncToAuth = false;
	bool bHasRepSpawnHandled = false;
	bool bIsFinalImpactFalling = false;

	FPRProjectileRepHomingSchedule PendingHomingSchedule;
	FPRProjectileRepHomingSchedule PendingClientHomingPresentationSchedule;
	FTimerHandle ProjectileHomingStartTimerHandle;
	FTimerHandle ProjectileHomingStopTimerHandle;
	TWeakObjectPtr<AActor> ClientPresentationHomingTarget;
	FVector ClientPresentationVelocity = FVector::ZeroVector;
	float ClientPresentationHomingAcceleration = 0.0f;
	float ClientPresentationHomingStartDelay = 0.0f;
	float ClientPresentationHomingDuration = 0.0f;
	float ClientPresentationElapsedSeconds = 0.0f;
	float ClientPresentationHomingElapsedSeconds = 0.0f;
	uint8 ClientPresentationRevision = 0;
	bool bHasPendingClientHomingPresentation = false;
	bool bClientProjectilePresentationActive = false;
	bool bClientProjectileHomingStarted = false;

	// 현재 투사체 인스턴스의 바운스 순서 번호
	uint8 CurrentBounceIndex = 0;

	// 마지막으로 보정 처리한 서버 바운스 존재 여부
	bool bHasLastReconciledServerBounceIndex = false;

	// 마지막으로 보정 처리한 서버 바운스 순서 번호
	uint8 LastReconciledServerBounceIndex = 0;

	// 소유 클라이언트 예측 투사체의 최근 바운스 이동 기록
	TArray<FPRPredictedProjectileBounceRecord> PredictedBounceRecords;
	
	TSet<TWeakObjectPtr<AActor>> HitActors;
	
#if WITH_EDITORONLY_DATA
	// 디버그 스피어 드로우 활성화 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug")
	bool bDrawDebugSphere = false;

	// 디버그 스피어 반지름 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "1.0"))
	float DebugSphereRadius = 10.f;

	// 드로우 주기 (초). 0이면 매 틱
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "0.0"))
	float DebugDrawInterval = 0.f;

	// 드로우 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "0.0"))
	float DebugDrawLifetime = 1.f;
	
	// Predicted 역할 색상
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere"))
	FColor DebugColorPredicted = FColor::Yellow;

	// Auth 역할 색상
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere"))
	FColor DebugColorAuth = FColor::Green;

	// 마지막 드로우 이후 경과 시간 누적.
	float DebugDrawAccumulator = 0.f;
#endif
	
};
