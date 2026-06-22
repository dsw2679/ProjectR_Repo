// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossSpawnPatternActors.h"
#include "PRGameplayAbility_BossPortalSequence.generated.h"

class APRBossPortalActor;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UPRFaerinCharacterEventRouterComponent;

// Faerin 계열 포털 패턴의 실행 타입이다.
UENUM(BlueprintType)
enum class EPRBossPortalPatternType : uint8
{
	Missile		UMETA(DisplayName = "Missile"),
	Barrage		UMETA(DisplayName = "Barrage"),
	Attached	UMETA(DisplayName = "Attached"),
	Torrent		UMETA(DisplayName = "Torrent"),
	Pair		UMETA(DisplayName = "Pair")
};

// 포털 Helper Actor를 어느 타이밍에 생성할지 정의한다.
UENUM(BlueprintType)
enum class EPRBossPortalSpawnTimingMode : uint8
{
	ImmediateOnActivate	UMETA(DisplayName = "Immediate On Activate"),
	OnCharacterEvent	UMETA(DisplayName = "On Character Event")
};

// 보스 포털 Helper Actor를 생성하고 Ability 종료/취소 시 정리하는 공용 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_BossPortalSequence : public UPRGameplayAbility_BossSpawnPatternActors
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossPortalSequence();

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
	// 현재 설정에 따라 포털 Actor를 생성하고 시작 처리한다.
	bool SpawnConfiguredPortals();

	// 생성된 포털들의 텔레그래프 시작을 설정값에 맞춰 예약한다.
	virtual void StartSpawnedPortals();

	// 포털 시퀀스 유지 시간이 끝났을 때 Ability를 정상 종료한다.
	void FinishPortalSequence();

	// Ability가 보유한 포털 Actor를 만료 처리한다.
	void ExpireSpawnedPortals();

	// CharacterEvent Router에 이번 Ability listener를 등록한다.
	bool RegisterCharacterEventListener();

	// CharacterEvent Router에서 이번 Ability listener를 제거한다.
	void UnregisterCharacterEventListener();

	// Faerin CharacterEvent가 전달됐을 때 포털 스폰 조건을 확인한다.
	void HandleFaerinCharacterEvent(FName EventName);

	// 포털 소환 몽타주가 정상 종료됐을 때 처리한다.
	UFUNCTION()
	void HandlePortalSummonMontageCompleted();

	// 포털 소환 몽타주가 취소/중단됐을 때 처리한다.
	UFUNCTION()
	void HandlePortalSummonMontageInterrupted();

	// 설정된 유지 시간에 따라 Ability 종료 타이머를 시작한다.
	void StartPortalSequenceFinishTimer();

protected:
	// 이번 포털 시퀀스의 패턴 타입이다. 1차에서는 BP/데이터 분기와 디버그 기준으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalPatternType PortalPatternType = EPRBossPortalPatternType::Missile;

	// 포털 Actor 생성 타이밍이다. 기존 자산 호환을 위해 기본값은 즉시 생성이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalSpawnTimingMode SpawnTimingMode = EPRBossPortalSpawnTimingMode::ImmediateOnActivate;

	// CharacterEvent 기반 스폰에서 기다릴 원작 이벤트 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	FName SpawnCharacterEventName = TEXT("SummonPortal_Missile");

	// CharacterEvent 기반 스폰 전에 재생할 포털 소환 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Animation", meta = (EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	TObjectPtr<UAnimMontage> PortalSummonMontage;

	// 포털 소환 몽타주 시작 섹션이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Animation", meta = (EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	FName PortalSummonMontageStartSection = NAME_None;

	// 포털 소환 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Animation", meta = (ClampMin = "0.0", EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	float PortalSummonMontagePlayRate = 1.0f;

	// CharacterEvent로 포털을 만든 뒤 Ability를 바로 끝낼지 여부다. 독립 생존 포털은 bExpireSpawnedPortalsOnEnd=false와 함께 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	bool bEndAbilityAfterEventSpawn = false;

	// CharacterEvent로 포털을 만든 뒤 소환 몽타주 완료까지 Ability를 유지할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Animation", meta = (EditCondition = "SpawnTimingMode == EPRBossPortalSpawnTimingMode::OnCharacterEvent"))
	bool bEndAbilityOnSummonMontageCompleted = true;

	// 포털 생성 직후 텔레그래프를 시작할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bStartPortalsAfterSpawn = true;

	// 포털을 여러 개 생성할 때 각 포털 텔레그래프 시작 사이에 둘 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0", EditCondition = "bStartPortalsAfterSpawn"))
	float PortalStartInterval = 0.0f;

	// Ability 종료/취소 시 생성한 포털을 만료 처리할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bExpireSpawnedPortalsOnEnd = true;

	// 포털 시퀀스 유지 시간이다. 0 이하이면 포털 생성 직후 Ability를 종료한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float PortalSequenceDuration = 15.0f;

	// 이번 Ability 실행에서 생성한 포털 Actor 목록이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	TArray<TObjectPtr<APRBossPortalActor>> SpawnedPortalRefs;

private:
	FTimerHandle PortalSequenceTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActivePortalSummonMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCharacterEventRouterComponent> ActiveEventRouter;

	bool bPortalActorsSpawned = false;
};
