// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (패턴 시작 시 카메라 및 조준 카메라 보정 연동)
// Author: 손승우 (GodFall 패턴 시퀀스 및 지면 충격파 VFX, PoiseDamage 판정 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinGodFallComponent.generated.h"

class APRBossBaseCharacter;
class APRFaerinGodFallStaticSwordActor;
class UNiagaraComponent;
class UAnimSequenceBase;
class UPRFaerinGodFallDataAsset;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FPRFaerinGodFallEntryFinishedSignature, bool);
DECLARE_MULTICAST_DELEGATE(FPRFaerinGodFallCastStartedSignature);

UENUM()
enum class EPRFaerinGodFallEntryRuntimeState : uint8
{
	Idle,
	MovingToCastLocation,
	ChargeRising,
	TiltHolding,
	BossDropWaiting,
	BossFastDropping,
	BossDropGroundHolding,
	HazardActive
};

// Faerin Phase2 God Fall의 시전 위치 이동, Rig 재생, StaticSword 충전/배정 루프를 관리한다.
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinGodFallComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFaerinGodFallComponent();

	// Phase2 진입 God Fall 연출과 StaticSword 충전 루프를 시작한다.
	bool StartGodFallEntry(AActor* InPatternTarget);

	// 진행 중인 God Fall 진입 연출을 취소한다.
	void CancelGodFallEntry();

	// 생성되어 유지 중인 StaticSword hazard를 모두 취소한다.
	void CancelGodFallHazards();

	// 현재 phase에 맞춰 이후 StaticSword 충전/경고 속도를 갱신한다.
	void ApplyGodFallPhaseScaling(EPRBossPhase Phase);

	// Rig 상승 완료 시점에 bone 10개를 StaticSword actor로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	void ConvertRigToStaticSwords();

	// Drop 몽타주 시작 시점부터 하강 대기 연출을 시작한다.
	void NotifyGodFallBodyDropMontageStarted();

	// God Fall body 몽타주 시퀀스가 끝났을 때 보스 위치 고정을 해제한다.
	void NotifyGodFallBodyMontageSequenceFinished();

	// 현재 God Fall 데이터 에셋을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	UPRFaerinGodFallDataAsset* GetGodFallData() const { return GodFallData; }

	// God Fall entry가 진행 중인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	bool IsGodFallEntryRunning() const { return bGodFallEntryRunning; }

public:
	FPRFaerinGodFallEntryFinishedSignature OnGodFallEntryFinished;
	FPRFaerinGodFallCastStartedSignature OnGodFallCastStarted;

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	APRBossBaseCharacter* GetOwnerBoss() const;
	USkeletalMeshComponent* ResolveRigMeshComponent();
	bool ValidateGodFallEntryInputs();
	bool PlayRigAnimation(UAnimSequenceBase* Animation, float PlayRate, FTimerDelegate FinishDelegate);
	void PlayRigAnimationLocal(UAnimSequenceBase* Animation, float PlayRate);
	void StartMovingToCastLocation();
	void UpdateMoveToCastLocation(float DeltaTime);
	void StartGodFallCast();
	void UpdateBossChargePresentation(float DeltaTime);
	void HoldBossAtApex();
	void BeginBossDropPresentation();
	void ExecuteBossFastDrop();
	void UpdateBossFastDropPresentation(float DeltaTime);
	void UpdateClientBossPresentation(float DeltaTime);
	void FinishBossDropPresentation();
	void HoldBossAtCastGroundLocation();
	FVector ResolveGodFallCastLocation() const;
	FRotator ResolveGodFallCastRotation() const;
	void HandleRigChargeFinished();
	void HandleRigTiltPullFinished();
	bool SpawnStaticSwordFromBone(int32 BoneArrayIndex, FName BoneName);
	void StartStaticSwordEntryOrbit();
	float ResolveStaticSwordEntryDiveDelaySeconds() const;
	void StartStaticSwordEntryDive();
	void HandleStaticSwordEntryImpactFinished(APRFaerinGodFallStaticSwordActor* FinishedSword);
	void HandleStaticSwordChargeFinished(APRFaerinGodFallStaticSwordActor* FinishedSword);
	void HandleStaticSwordAssignedAttackFinished(APRFaerinGodFallStaticSwordActor* FinishedSword);
	void TryAssignNextSword();
	void RefreshGodFallTargets(AActor* FallbackTarget = nullptr);
	bool IsValidGodFallTarget(AActor* CandidateTarget) const;
	bool IsTargetAlreadyAssigned(const AActor* CandidateTarget) const;
	void AdvanceTargetAssignmentCursor(int32 AssignedTargetIndex);
	float ResolveSwordChargeSeconds() const;
	float ResolveTargetOverheadMoveSeconds() const;
	float ResolveWarningSeconds() const;
	float ResolvePhaseTimingScale() const;
	void BroadcastEntryFinished(bool bSucceeded);
	void ClearRigTimers();
	void StartGodFallBodyNiagaraCuesLocal();
	void SpawnGodFallBodyNiagaraCueLocal(int32 CueIndex);
	void CleanupGodFallBodyNiagaraLocal();
	void ScheduleSwordRisePoiseDamage();
	void ApplySwordRisePoiseDamage();
	void CancelSwordRisePoiseDamage();
	void ApplyEntryOrbitImpactGlobalDamage();
	void DestroyPlacedRigActor();
	void SetPlacedRigHidden(bool bNewHidden);
	void ApplyBossPresentationTransform(const FVector& Location, const FRotator& Rotation);
	void BeginBossPresentationReplicationOverride();
	void EndBossPresentationReplicationOverride();
	void StartClientBossPresentationSegment(EPRFaerinGodFallEntryRuntimeState NewState,
		const FVector& StartLocation,
		const FVector& TargetLocation,
		const FRotator& StartRotation,
		const FRotator& TargetRotation,
		float DurationSeconds);
	void SetClientBossPresentationTransform(const FVector& Location, const FRotator& Rotation, bool bKeepHolding);
	void StopClientBossPresentation(const FVector& Location, const FRotator& Rotation);
	void ClearInvalidStaticSwordRefs();
	void ApplyGodFallInvulnerability();
	void ClearGodFallInvulnerability();

	// God Fall 본체 이동 연출 구간을 클라이언트가 로컬 보간하도록 시작시킨다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartBossPresentationSegment(EPRFaerinGodFallEntryRuntimeState NewState,
		FVector StartLocation,
		FVector TargetLocation,
		FRotator StartRotation,
		FRotator TargetRotation,
		float DurationSeconds);

	// God Fall 본체 위치를 클라이언트에서 즉시 맞추거나 고정한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetBossPresentationTransform(FVector Location, FRotator Rotation, bool bKeepHolding);

	// God Fall 본체 전용 클라이언트 연출 tick을 종료한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopBossPresentation(FVector Location, FRotator Rotation);

	// 맵 배치 sword rig의 표시 상태를 서버와 모든 클라이언트에서 맞춘다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPlacedRigHidden(bool bNewHidden);

	// 맵 배치 sword rig의 single node 애니메이션을 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayRigAnimation(UAnimSequenceBase* Animation, float PlayRate);

	// StaticSword 전환 후 맵 배치 sword rig를 모든 클라이언트에서 제거한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDestroyPlacedRigActor();

	// God Fall 본체 Niagara cue 재생 예약을 모든 클라이언트에서 시작한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartGodFallBodyNiagaraCues();

	// God Fall 본체 Niagara cue를 모든 클라이언트에서 정리한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastCleanupGodFallBodyNiagara();

protected:
	// God Fall rig, bone, StaticSword 수치를 담은 data asset이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	TObjectPtr<UPRFaerinGodFallDataAsset> GodFallData;

	// 맵에 이미 배치된 Faerin_Swords_Rig actor다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	TObjectPtr<AActor> PlacedSwordRigActor;

	// 맵 배치 rig actor 안의 SkeletalMeshComponent를 직접 지정할 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	TObjectPtr<USkeletalMeshComponent> PlacedSwordRigMeshComponent;

	// Phase2 God Fall 시전 위치를 home location 대신 별도 actor로 고정할 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|CastLocation")
	TObjectPtr<AActor> GodFallCastAnchorActor;

	// 별도 anchor actor가 없을 때 Faerin home location을 God Fall 시전 위치로 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|CastLocation")
	bool bUseHomeLocationAsCastLocation = true;

	// God Fall 시전 위치에 도착했을 때 사용할 yaw 회전값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|CastLocation")
	float GodFallCastYawDegrees = 0.0f;

	// BeginPlay에서 God Fall 시전 전까지 rig actor를 숨길지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	bool bHideRigActorUntilEntry = true;

	// StaticSword 전환 후 맵 배치 rig actor를 제거할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	bool bDestroyRigActorAfterConversion = true;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<APRFaerinGodFallStaticSwordActor>> ActiveStaticSwords;

	TArray<TWeakObjectPtr<AActor>> ActivePatternTargets;
	EPRFaerinGodFallEntryRuntimeState EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::Idle;
	EPRBossPhase CurrentGodFallPhase = EPRBossPhase::Phase2;
	FTimerHandle RigChargeTimerHandle;
	FTimerHandle RigTiltPullTimerHandle;
	FTimerHandle BossDropTimerHandle;
	FTimerHandle SwordRisePoiseDamageTimerHandle;
	int32 PendingEntryOrbitImpactSwordCount = 0;
	int32 FinishedEntryOrbitImpactSwordCount = 0;
	bool bEntryOrbitImpactGlobalDamageApplied = false;
	bool bStaticSwordEntryOrbitStarted = false;
	float StaticSwordEntryOrbitStartServerTimeSeconds = -1.0f;
	TArray<FTimerHandle> BodyNiagaraCueTimerHandles;
	TArray<FTimerHandle> BodyNiagaraCleanupTimerHandles;
	int32 NextTargetAssignmentIndex = 0;
	FVector BossCastGroundLocation = FVector::ZeroVector;
	FVector BossChargeApexLocation = FVector::ZeroVector;
	FRotator BossCastRotation = FRotator::ZeroRotator;
	FRotator BossChargeStartRotation = FRotator::ZeroRotator;
	FRotator BossChargeApexRotation = FRotator::ZeroRotator;
	FVector ClientBossSegmentStartLocation = FVector::ZeroVector;
	FVector ClientBossSegmentTargetLocation = FVector::ZeroVector;
	FRotator ClientBossSegmentStartRotation = FRotator::ZeroRotator;
	FRotator ClientBossSegmentTargetRotation = FRotator::ZeroRotator;
	float BossPresentationElapsedSeconds = 0.0f;
	float ClientBossSegmentElapsedSeconds = 0.0f;
	float ClientBossSegmentDurationSeconds = 0.0f;
	bool bBodyDropMontageStarted = false;
	bool bBodyMontageSequenceFinished = false;
	bool bGodFallEntryRunning = false;
	bool bGodFallInvulnerabilityApplied = false;
	bool bGodFallConvertedToStaticSwords = false;
	bool bSavedBossReplicateMovement = true;
	bool bHasSavedBossReplicateMovement = false;
	bool bClientBossPresentationActive = false;
	bool bClientBossPresentationHolding = false;
	EPRFaerinGodFallEntryRuntimeState ClientBossPresentationState = EPRFaerinGodFallEntryRuntimeState::Idle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveBodyNiagaraComponents;
	
	FTimerHandle SwordInitHandle;
};
