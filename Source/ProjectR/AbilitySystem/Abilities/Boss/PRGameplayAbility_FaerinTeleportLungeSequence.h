// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 순간이동 Lunge 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "PRGameplayAbility_FaerinTeleportLungeSequence.generated.h"

class ACharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UPRFaerinCharacterEventRouterComponent;
class UPRFaerinWeaponVisualComponent;

// Faerin TeleportLunge Ability의 내부 몽타주 진행 단계다.
UENUM()
enum class EPRFaerinTeleportLungeStage : uint8
{
	None,
	TeleportOut,
	TeleportIn,
	LungeStart,
	LungeLoop,
	LungeEnd
};

// Faerin 원작 Lunge 구조를 ProjectR 보스 패턴 Ability로 구현한다.
// 일반 근접 콤보가 아니라 추적 구간과 직선 돌진 구간을 가진 장거리 패턴으로 처리한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinTeleportLungeSequence : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinTeleportLungeSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// TeleportOut, TeleportIn, LungeStart 순서의 런지 시퀀스를 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 런지 이동/판정/이벤트 리스너를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Faerin CharacterEvent Router에 런지 이벤트 listener를 등록한다.
	bool RegisterCharacterEventListener();

	// 등록한 런지 이벤트 listener를 제거한다.
	void UnregisterCharacterEventListener();

	// 원작 StartLunge / StopLunge 이벤트를 받아 런지 구간을 제어한다.
	void HandleFaerinCharacterEvent(FName EventName);

	// 현재 타겟 방향으로 보스 회전을 맞춘다.
	void FaceCurrentTarget() const;

	// 지정 몽타주를 현재 단계로 재생한다.
	bool PlayStageMontage(UAnimMontage* Montage, EPRFaerinTeleportLungeStage Stage);

	// TeleportIn 단계로 진입한다.
	void PlayTeleportInStage();

	// LungeStart 단계로 진입한다.
	void PlayLungeStartStage();

	// LungeLoop 표현을 재생하고 이동 타이머를 유지한다.
	void PlayLungeLoopStage();

	// LungeEnd 단계로 진입한다.
	void PlayLungeEndStage();

	// 원작 Track/Straight 이동 구간을 시작한다.
	void BeginLungeMovement();

	// 런지 이동과 히트 윈도우를 정리한다.
	void StopLungeMovementAndHitWindow();

	// 런지 이동 구간을 타이머 기반으로 갱신한다.
	void UpdateLungeMovement();

	// 런지 히트 윈도우를 연다.
	void BeginLungeHitWindow();

	// 런지 히트 윈도우를 닫는다.
	void EndLungeHitWindow();

	// 현재 검 판정 기준점을 계산한다.
	bool GetCurrentBladeTracePoint(FVector& OutTracePoint) const;

	// 런지 진행 중 검 위치를 따라 Sweep 판정을 갱신한다.
	void UpdateLungeHitTrace();

	// 지정 지점 사이의 Sweep 판정과 피해 적용을 수행한다.
	void ApplyLungeDamageTrace(const FVector& TraceStart, const FVector& TraceEnd);

	// 현재 후보 액터가 런지 피해 대상인지 확인한다.
	bool ShouldDamageActor(AActor* CandidateActor) const;

	// 런지 이동 전 CharacterMovement 기본값을 캐시한다.
	void CacheMovementDefaults(ACharacter* BossCharacter);

	// 런지 종료 후 CharacterMovement 기본값을 복구한다.
	void RestoreMovementDefaults(ACharacter* BossCharacter);

	// Ability를 정상/취소 상태로 마무리한다.
	void FinishLungeSequence(bool bWasCancelled);

	// 몽타주가 정상 종료됐을 때 다음 단계로 이동한다.
	UFUNCTION()
	void HandleStageMontageCompleted();

	// 중간 단계 몽타주가 자연스럽게 블렌드아웃될 때 다음 몽타주를 바로 이어붙인다.
	UFUNCTION()
	void HandleStageMontageBlendOut();

	// 몽타주가 중단됐을 때 Ability를 취소한다.
	UFUNCTION()
	void HandleStageMontageInterrupted();

protected:
	// TeleportIn 전에 검 분리/순간이동 아웃 연출을 재생하는 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> TeleportOutMontage;

	// 외부 Teleport VFX 래퍼가 선행되는 페이즈에서는 중복 TeleportOut을 막기 위해 끈다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	bool bPlayTeleportOutStage = true;

	// 텔레포트 인 연출 몽타주다. 1차에서는 선택 사용하며 원작 선행 여부는 에디터 검증으로 확정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> TeleportInMontage;

	// 외부 Teleport VFX 래퍼가 TeleportIn을 담당하는 경우 중복 재생을 막기 위해 끈다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	bool bPlayTeleportInStage = true;

	// 런지 시작 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> LungeStartMontage;

	// 런지 이동 루프 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> LungeLoopMontage;

	// 런지 종료 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation")
	TObjectPtr<UAnimMontage> LungeEndMontage;

	// 런지 몽타주 공통 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// 타겟 추적 구간 유지 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float TrackDuration = 0.45f;

	// 추적 후 직선 돌진 구간 유지 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float StraightDuration = 0.35f;

	// 원작 Lunge 이동 속도 기준값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float LungeSpeed = 7500.0f;

	// 원작 Lunge 가속도 기준값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.0"))
	float LungeAcceleration = 9999999.0f;

	// 런지 이동 갱신 주기다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement", meta = (ClampMin = "0.005"))
	float LungeTickInterval = 0.016f;

	// 런지 이동 방향에서 높이 차이를 제거할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Movement")
	bool bConstrainToGround = true;

	// 원작 StartLunge / StopLunge 이벤트를 히트 윈도우 제어에 사용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	bool bUseCharacterEventsForHitWindow = true;

	// WeaponVisualComponent가 없을 때 사용할 검 본/소켓 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	FName FallbackBladeBoneName = TEXT("Bone_FN_Weapon_Sword_Blade");

	// 검 본/HitBox 기준 판정 중심점 보정값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	FVector HitTraceOffset = FVector::ZeroVector;

	// 런지 Sweep 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit", meta = (ClampMin = "0.0"))
	float HitTraceRadius = 85.0f;

	// 런지 Sweep 충돌 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	TEnumAsByte<ECollisionChannel> HitTraceChannel = ECC_Pawn;

	// 현재 ThreatTarget만 피해 대상으로 제한할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	bool bOnlyDamageThreatTarget = true;

	// BossStatRow AttackPower에 곱해 Health 피해를 산출하는 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// 플레이어에게 적용할 고정 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit", meta = (ClampMin = "0.0"))
	float PoiseDamage = 0.0f;

	// 런지 Sweep 디버그 표시 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle LungeUpdateTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCharacterEventRouterComponent> ActiveEventRouter;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinWeaponVisualComponent> WeaponVisualComponent;

	EPRFaerinTeleportLungeStage ActiveLungeStage = EPRFaerinTeleportLungeStage::None;
	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	FVector PreviousBladeTracePoint = FVector::ZeroVector;
	FVector StraightLungeDirection = FVector::ZeroVector;
	float CachedMaxWalkSpeed = 0.0f;
	float CachedMaxAcceleration = 0.0f;
	float LungeElapsedTime = 0.0f;
	bool bHasPreviousBladeTracePoint = false;
	bool bLungeMovementActive = false;
	bool bLungeHitWindowActive = false;
	bool bHasCachedMovementDefaults = false;
	bool bLungeSequenceFinished = false;
};
