// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (사격 반동 및 스켈레탈 메시 사격 애니메이션 연동 구현)
// Author: 배유찬 (기본 사격 투사체 발사 제어 및 VFX/데미지 파이프라인 연동 구현)
// Author: 손승우 (사격 시 보스 캐스팅 캔슬 연동)
// Author: 이건주 (사격 시 무기 Mod 및 자원 소모 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRFireAbilityTypes.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Fire.generated.h"

struct FPRProjectileSpawnInfo;
class APRProjectileBase;
class APRWeaponActor;
class UAnimMontage;      
class UPRWeaponDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogFire, Log, All);

/**
 * 플레이어 사격 어빌리티 베이스
 */
UCLASS(Abstract)
class PROJECTR_API UPRGA_Fire : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Fire();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/*~ UPRGameplayAbility Interface ~*/
	virtual void OnFailActivateAbility(const UAbilitySystemComponent* InOwnerASC, const FGameplayAbilitySpec* InAbilitySpec) const override;
	void OnOutOfAmmo(const UAbilitySystemComponent* InOwnerASC) const;

public:
	// 총구 위치 
	UFUNCTION(BlueprintPure)
	virtual FVector GetMuzzleLocation();

	// 로컬 화면 Viewpoint 추출
	virtual FPRFireViewpoint GetFireViewpoint() const;

	// 1차 트레이스: 카메라에서 조준 방향으로 트레이스해 조준 끝점과 카메라 HitResult 산출
	virtual FVector ResolveAimPoint(const FPRFireViewpoint& View, float InTraceDistance, FHitResult* OutCameraHitResult = nullptr) const;

	// 2차 트레이스: 총구에서 조준 끝점으로 트레이스해 실제 발사 히트 결과 산출
	virtual FHitResult PerformMuzzleTrace(const FVector& MuzzleLoc, const FVector& AimPoint) const;
	void SendRecoilEvent();

	// per-shot 서버 보고 (unreliable). 유실 허용
	UFUNCTION(Server, Unreliable)
	void Server_ReportShot(FPRFireShotPayload Payload);

	// 발사 1회 처리. 로컬에서 트레이스 + 디버그라인 + 서버 보고 흐름을 수행한다
	UFUNCTION(BlueprintCallable)
	virtual void FireHitScan();

	// 투사체 1개 발사. 결과는 OnProjectileSpawnSuccess/Failed 가상 핸들러로 통지
	UFUNCTION(BlueprintCallable)
	virtual void FireProjectile(TSubclassOf<APRProjectileBase> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);
	
	
	// 조준 기준 투사체 발사 트랜스폼 계산. 1차 카메라 트레이스로 조준점을 구하고, 총구->조준점 방향을 회전으로 환산
	// 위치는 총구. 조준점이 총구 뒤쪽이거나 거리가 너무 짧으면 카메라 정면 방향으로 폴백
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FTransform GetProjectileLaunchTransform();
	
protected:
	// 현재 활성 무기 데이터 기준 최대 사격 거리 반환
	float GetMaxFireDistance() const;

	// 04.28 김동석 추가, // strength, speed는 삭제
	// 연속 발사 횟수를 초기화한다
	void ResetConsecutiveShots();

	// 서버가 샷을 확정 처리. 데미지 적용 (현재는 로그만)
	virtual void ServerConfirmShot(const FPRFireShotPayload& Payload);

	// Payload의 HitResult 기반 데미지 GameplayEffect 적용
	virtual void ApplyDamageFromShot(const FPRFireShotPayload& Payload);

	// 히트스캔 발사 결과를 Trail FX Payload로 변환해 예측 재생과 서버 전파 요청
	void PlayFireFX_Trail(const FVector& StartLocation, const FVector& EndLocation, const FHitResult* HitResult);
	
	// Trail 없는 FireFX
	void PlayFireFX();
	
	// 현재 어빌리티 컨텍스트에서 지정 무기 몽타주 재생
	void PlayWeaponMontage(UAnimMontage* Montage, float PlayRate);

	// 플레이어 무기 데미지 GE를 타겟에 적용한다. HitResult를 EffectContext에 포함시킨다
	void ApplyDamage(AActor* TargetActor, const FHitResult* HitResult = nullptr);

	// AbilityTask가 투사체 스폰 성공 시 호출. 파생 클래스에서 추가 처리(VFX/SFX 등) 오버라이드 용도
	UFUNCTION()
	virtual void OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);

	// AbilityTask가 투사체 스폰 실패/예측 거부 시 호출. 파생 클래스에서 후속 처리 오버라이드 용도
	UFUNCTION()
	virtual void OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);
	
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);
	
protected:
	// true이면 무기 데이터의 FireInterval 대신 FireIntervalOverride 사용
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|FullAuto")
	bool bOverrideFireInterval = false;

	// Override가 true일 때 사용할 발사 간격(초). 0.1 = 600 RPM
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|FullAuto")
	float FireIntervalOverride = 0.1f;

	// 사격 트레이스에 사용할 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	TEnumAsByte<ECollisionChannel> FireTraceChannel = PRCollisionChannels::ECC_Combat;
	
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	TEnumAsByte<ECollisionChannel> CameraTraceChannel = ECC_Visibility;

	// 발사 시점에 재생할 Trail FX Cue 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|FX")
	FGameplayTag TrailCueTag;
	
	// 발사 시점에 재생할 발사 FX Cue 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|FX")
	FGameplayTag SimpleFireCueTag;
	
	// 카메라 트레이스(1차, 시안색) 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	bool bDrawCameraTrace = true;

	// 총구 트레이스(2차, 빨강) 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	bool bDrawMuzzleTrace = true;

	// 디버그 라인 지속 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	float DebugDrawDuration = 1.0f;
	
	// 활성 무기 캐시 (활성화 시 1회 획득)
	TWeakObjectPtr<APRWeaponActor> CurrentWeapon;

	// 자식 어빌리티가 ActivateAbility 진입 시 무기 데이터 또는 Override로 캐싱. ApplyCooldown에서 SetByCaller로 주입
	float CachedFireInterval = 0.1f;

	// GetCooldownTags가 반환할 컨테이너. 생성자에서 Cooldown.Ability.Fire.Primary 채움
	FGameplayTagContainer CooldownTagsContainer;

	// 로컬 단조 증가 ShotID (1부터)
	uint32 NextShotId = 0;

	// 현재 입력 유지 중 누적된 연속 발사 횟수
	int32 ConsecutiveShots = 0;
	
	FActiveGameplayEffectHandle CooldownHandle;
};
