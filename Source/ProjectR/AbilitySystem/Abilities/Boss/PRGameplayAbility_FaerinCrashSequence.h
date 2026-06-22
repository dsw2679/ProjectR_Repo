// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Crash(지면충돌) 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinStagedMontagePattern.h"
#include "PRGameplayAbility_FaerinCrashSequence.generated.h"

class UNiagaraSystem;

// Faerin Crash 계열의 피해 적용 방식을 구분한다.
UENUM(BlueprintType)
enum class EPRFaerinCrashDamageMode : uint8
{
	Radius			UMETA(DisplayName = "Radius"),
	GlobalPlayers	UMETA(DisplayName = "Global Players")
};

// Faerin 원작 Crash / CrashDrop 계열의 DoCrashAoE 이벤트 피해를 처리하는 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinCrashSequence : public UPRGameplayAbility_FaerinStagedMontagePattern
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinCrashSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Crash 실행마다 중복 피해 상태를 초기화한 뒤 staged montage 패턴을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Crash 종료 시 이벤트 피해 캐시를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 원작 DoCrashAoE CharacterEvent를 수신하면 주변 피해를 적용한다.
	virtual void NativeOnStageCharacterEvent(const FPRFaerinStagedMontageStage& Stage, FName EventName) override;

	// Crash AoE 중심점을 계산한다.
	FVector ResolveCrashAreaCenter() const;

	// Crash 판정 시 보스 바닥에 충돌 Niagara를 재생한다.
	void SpawnCrashImpactVFX() const;

	// Crash 충돌 Niagara를 재생할 보스 바닥 위치를 계산한다.
	bool ResolveCrashImpactLocation(FVector& OutLocation) const;

	// Crash AoE 반경 안의 유효 대상에게 피해를 적용한다.
	void ApplyCrashAreaDamage();

	// 서버가 알고 있는 모든 플레이어 Pawn에게 전역 피해를 적용한다.
	void ApplyCrashGlobalPlayerDamage();

protected:
	// 원작 Crash AoE를 발생시키는 CharacterEvent 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	FName CrashAreaEventName = TEXT("DoCrashAoE");

	// Crash 피해를 radius overlap으로 줄지, 모든 플레이어에게 줄지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	EPRFaerinCrashDamageMode CrashDamageMode = EPRFaerinCrashDamageMode::Radius;

	// Crash AoE 중심에 더할 보스 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	FVector CrashAreaLocalOffset = FVector::ZeroVector;

	// Crash AoE 피해 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashDamageRadius = 450.0f;

	// Enemy AttackPower에 곱할 Crash 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashDamageMultiplier = 1.5f;

	// Crash가 부여할 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashPoiseDamage = 25.0f;

	// Crash AoE 대상 탐색에 사용할 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	TEnumAsByte<ECollisionChannel> CrashOverlapChannel = ECC_Pawn;

	// Crash/CrashDrop 판정 순간 보스 바닥에 재생할 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX")
	TObjectPtr<UNiagaraSystem> CrashImpactNiagaraSystem;

	// Crash 충돌 Niagara 위치에 더할 월드 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX")
	FVector CrashImpactLocationOffset = FVector(0.0f, 0.0f, 8.0f);

	// Crash 충돌 Niagara 회전 보정값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX")
	FRotator CrashImpactNiagaraRotationOffset = FRotator::ZeroRotator;

	// Crash 충돌 Niagara 스케일이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX")
	FVector CrashImpactNiagaraScale = FVector::OneVector;

	// 0보다 크면 Crash 충돌 Niagara를 해당 시간 뒤 강제 정리한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX", meta = (ClampMin = "0.0"))
	float CrashImpactNiagaraLifeSeconds = 2.0f;

	// Crash 충돌 위치를 바닥으로 보정할 때 위로 올릴 trace 시작 높이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX", meta = (ClampMin = "0.0"))
	float CrashImpactGroundTraceUpOffset = 500.0f;

	// Crash 충돌 위치를 바닥으로 보정할 때 아래로 내릴 trace 길이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX", meta = (ClampMin = "0.0"))
	float CrashImpactGroundTraceDownOffset = 1500.0f;

	// Crash 충돌 위치의 바닥 trace channel이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|ImpactVFX")
	TEnumAsByte<ECollisionChannel> CrashImpactGroundTraceChannel = ECC_Visibility;

	// 하나의 Ability 실행에서 DoCrashAoE가 여러 번 들어와도 한 번만 피해를 적용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	bool bApplyCrashDamageOncePerActivation = true;

	// Crash AoE 반경을 에디터 디버그로 표시할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|Debug")
	bool bDrawDebugCrashArea = false;

	// Crash AoE 디버그 표시 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|Debug",
		meta = (EditCondition = "bDrawDebugCrashArea", ClampMin = "0.0"))
	float CrashDebugDrawDuration = 1.5f;

private:
	TSet<TWeakObjectPtr<AActor>> DamagedActorsThisCrash;
	bool bCrashDamageApplied = false;
};
