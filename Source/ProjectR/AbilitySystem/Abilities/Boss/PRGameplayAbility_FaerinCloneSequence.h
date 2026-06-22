// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCloneCharacter.h"
#include "PRGameplayAbility_FaerinCloneSequence.generated.h"

class APRFaerinCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UPRFaerinCharacterEventRouterComponent;

// Faerin 분신 패턴에서 분신 대상을 고르는 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinCloneTargetSelectionPolicy : uint8
{
	NearestToFaerin,
	FarthestFromFaerin
};

// Faerin Phase2 분신 소환 패턴 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinCloneSequence : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinCloneSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// 생존 플레이어 수와 현재 공격 대상 기준으로 분신을 소환한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 본체 공격 대상 커밋이 남아 있으면 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 소환 패턴 실행에 필요한 본체와 설정을 확인한다.
	bool InitializeCloneSequence(APRFaerinCharacter*& OutFaerin, AActor*& OutPrimaryTarget) const;

	// 현재 월드의 생존 플레이어를 수집한다.
	void GatherAlivePlayers(TArray<AActor*>& OutAlivePlayers) const;

	// 분신이 공격할 대상 목록을 결정한다.
	void ResolveCloneTargets(AActor* PrimaryTarget, const TArray<AActor*>& AlivePlayers, TArray<AActor*>& OutCloneTargets) const;

	// 대상 정렬 정책에 맞춰 후보를 정렬한다.
	void SortCloneTargetCandidates(const APRFaerinCharacter* Faerin, TArray<AActor*>& CandidateTargets) const;

	// 특정 타겟을 담당할 분신을 스폰한다.
	bool SpawnCloneForTarget(APRFaerinCharacter* Faerin, AActor* CloneTarget, int32 CloneIndex) const;

	// 분신 스폰 위치와 회전을 계산한다.
	FTransform ResolveCloneSpawnTransform(const APRFaerinCharacter* Faerin, AActor* CloneTarget, int32 CloneIndex) const;

	// 지정 액터가 분신 대상으로 유효한 생존 플레이어인지 확인한다.
	bool IsAlivePlayerTarget(AActor* CandidateActor) const;

	// 소환 몽타주 CharacterEvent를 받을 수 있도록 listener를 등록한다.
	bool RegisterCharacterEventListener();

	// 소환 몽타주 CharacterEvent listener를 정리한다.
	void UnregisterCharacterEventListener();

	// 소환 몽타주의 지정 이벤트 시점에 분신 생성을 실행한다.
	void HandleFaerinCharacterEvent(FName EventName);

	// 대기 중인 타겟 목록 기준으로 실제 분신을 생성한다.
	bool SpawnPendingClonesAtNotify();

	// 분신 소환 위치 바닥에 경고/소환 Niagara를 생성한다.
	void SpawnCloneSummonGroundVFX(APRFaerinCharacter* Faerin, const APRFaerinCloneCharacter* SpawnedClone) const;

	// 분신 캡슐 기준 바닥 VFX 위치를 계산한다.
	FVector ResolveCloneGroundVFXLocation(const APRFaerinCloneCharacter* SpawnedClone) const;

	// 소환 몽타주가 정상 종료되었을 때 Ability 종료 조건을 처리한다.
	UFUNCTION()
	void HandleSummonMontageCompleted();

	// 소환 몽타주가 취소/중단되었을 때 Ability를 취소한다.
	UFUNCTION()
	void HandleSummonMontageInterrupted();

protected:
	// 실제 스폰할 Faerin 분신 BP 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone")
	TSubclassOf<APRFaerinCloneCharacter> CloneClass;

	// 분신 한 패턴에서 최대 생성할 수 있는 수다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone", meta = (ClampMin = "1", ClampMax = "2"))
	int32 MaxCloneCount = 2;

	// PrimaryTarget 제외 후 여러 대상이 있을 때 고르는 정책이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone")
	EPRFaerinCloneTargetSelectionPolicy TargetSelectionPolicy = EPRFaerinCloneTargetSelectionPolicy::NearestToFaerin;

	// Faerin 기준 분신 스폰 오프셋 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	TArray<FVector> SpawnOffsets;

	// 스폰 오프셋을 Faerin 회전에 맞춰 회전할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	bool bRotateSpawnOffsetsByOwner = true;

	// 스폰 시 분신이 담당 타겟을 바라보게 할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	bool bFaceAssignedTargetOnSpawn = true;

	// 최종 스폰 회전에 더할 yaw 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	float SpawnYawOffset = 0.0f;

	// 바닥 스냅 트레이스 시작 높이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn", meta = (ClampMin = "0.0"))
	float SpawnGroundTraceUpDistance = 400.0f;

	// 바닥 스냅 트레이스 하강 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn", meta = (ClampMin = "0.0"))
	float SpawnGroundTraceDownDistance = 1200.0f;

	// 바닥 스냅 후 캐릭터 기준 위치에 더할 높이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	float SpawnGroundHeightOffset = 120.0f;

	// 바닥 스냅에 사용할 트레이스 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Spawn")
	TEnumAsByte<ECollisionChannel> SpawnGroundTraceChannel = ECC_Visibility;

	// 스폰된 분신이 사용할 런타임 설정이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Runtime")
	FPRFaerinCloneRuntimeConfig CloneRuntimeConfig;

	// 분신 소환에 사용할 본체 몽타주다. ShiftPlayer 몽타주 복사본을 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon|Animation")
	TObjectPtr<UAnimMontage> SummonMontage;

	// 분신 소환 몽타주의 시작 섹션이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon|Animation")
	FName SummonMontageStartSection = NAME_None;

	// 분신 소환 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon|Animation", meta = (ClampMin = "0.01"))
	float SummonMontagePlayRate = 1.0f;

	// AnimNotify(PRAnimNotify_FaerinCharacterEvent)가 전달해야 하는 분신 소환 이벤트 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon")
	FName SummonCharacterEventName = TEXT("SummonClone");

	// CharacterEvent로 분신 생성 직후 Ability를 종료할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon")
	bool bEndAbilityAfterSummonEvent = false;

	// CharacterEvent 이후 소환 몽타주 종료까지 Ability를 유지할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Clone|Summon|Animation")
	bool bEndAbilityOnSummonMontageCompleted = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveSummonMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCharacterEventRouterComponent> ActiveEventRouter;

	UPROPERTY(Transient)
	TObjectPtr<APRFaerinCharacter> ActiveFaerin;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> PendingCloneTargets;

	bool bPatternAttackCommitted = false;
	bool bCloneActorsSpawned = false;
};
