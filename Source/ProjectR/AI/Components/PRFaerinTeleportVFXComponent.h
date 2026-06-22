// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 순간이동 VFX 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/Data/PRFaerinCombatLoopDataAsset.h"
#include "PRFaerinTeleportVFXComponent.generated.h"

class APREnemyBaseCharacter;
class UAnimMontage;
class UGameplayEffect;
class UMaterialInstanceDynamic;
class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UTexture;

DECLARE_MULTICAST_DELEGATE_OneParam(FPRFaerinTeleportVFXFinishedSignature, bool);

// Phase3 이후 본체 공격 전에 Teleport Out, 디졸브, 두 구체 VFX 집결, 선택적 Teleport In 연출을 실행한다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinTeleportVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFaerinTeleportVFXComponent();

	// 선택된 패턴 계획을 기준으로 Teleport VFX 래퍼를 시작한다.
	bool StartTeleportVFX(const FPRFaerinPatternPlan& PatternPlan, AActor* TargetActor);

	// 진행 중인 Teleport VFX 래퍼를 취소하고 본체 표시 상태를 복구한다.
	void CancelTeleportVFX();

	// 현재 Teleport VFX 래퍼가 실행 중인지 반환한다.
	bool IsTeleportVFXRunning() const { return bTeleportVFXRunning; }

public:
	FPRFaerinTeleportVFXFinishedSignature OnTeleportVFXFinished;

protected:
	/*~ UActorComponent Interface ~*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	APREnemyBaseCharacter* GetOwnerEnemy() const;
	bool ResolveConvergeLocation(
		const FPRFaerinPatternPlan& PatternPlan,
		AActor* TargetActor,
		FVector& OutLocation) const;
	FRotator ResolveConvergeRotation(
		const FPRFaerinPatternPlan& PatternPlan,
		const FVector& ConvergeLocation,
		AActor* TargetActor) const;
	bool ProjectLocationToNavigation(const FVector& RawLocation, FVector& OutLocation) const;
	bool ShouldPlayTeleportInStage(const FPRFaerinPatternPlan& PatternPlan) const;
	bool ShouldApplyRevealSplashDamage() const;
	float ResolveMontageDuration(UAnimMontage* Montage, float PlayRate, float OverrideSeconds) const;
	void BuildVFXPaths(
		const FVector& BossStartLocation,
		const FVector& ConvergeLocation,
		FVector& OutLeftStart,
		FVector& OutLeftWander,
		FVector& OutRightStart,
		FVector& OutRightWander) const;
	FVector BuildWanderLocation(const FVector& OriginLocation, const FVector& TravelDirection) const;
	void BeginHiddenTeleportVFXStage();
	void FinishHiddenTeleportVFXStage();
	void FinishTeleportInStage();
	void CompleteTeleportVFX(bool bSucceeded);
	void ApplyRevealSplashDamage();
	void BeginOwnerReplicationOverride();
	void EndOwnerReplicationOverride();
	void StartTeleportOutPresentationLocal(FVector BossStartLocation, FRotator BossStartRotation);
	void BeginHiddenPresentationLocal(
		FVector ConvergeLocation,
		FRotator ConvergeRotation,
		FVector LeftStartLocation,
		FVector LeftWanderLocation,
		FVector RightStartLocation,
		FVector RightWanderLocation,
		float InWanderSeconds,
		float InConvergeSeconds,
		bool bInDisableCollisionWhileHidden);
	void FinishTeleportPresentationLocal(FVector FinalLocation, FRotator FinalRotation, bool bPlayTeleportInStage);
	void CancelTeleportPresentationLocal(FVector FinalLocation, FRotator FinalRotation);
	void UpdateTeleportVFXPresentation(float DeltaTime);
	void UpdateDissolvePresentation(float DeltaTime);
	void SetTeleportVFXPairLocation(const FVector& LeftLocation, const FVector& RightLocation);
	void CleanupTeleportVFXLocal();
	void CleanupTransientNiagaraLocal();
	void ResolveRevealSplashNiagaraTransform(
		const APREnemyBaseCharacter* BossCharacter,
		const FVector& SplashCenter,
		FVector& OutLocation,
		FRotator& OutRotation) const;
	void PlayRevealSplashPresentationLocal(FVector SplashLocation, FRotator SplashRotation, float SplashRadius);
	void SpawnBodyNiagaraLocal(UNiagaraSystem* NiagaraSystem);
	UNiagaraComponent* SpawnTeleportVFXLocal(const FVector& Location) const;
	UNiagaraComponent* SpawnTransientNiagaraAtLocationLocal(
		UNiagaraSystem* NiagaraSystem,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		float LifeSeconds = -1.0f);
	void PlayMontageLocal(UAnimMontage* Montage, float PlayRate) const;
	void StartDissolveLocal(float StartValue, float EndValue, float DurationSeconds);
	void PrepareDissolveMaterialsLocal();
	void StartDissolveNiagaraLocal(float StartValue);
	void CleanupDissolveNiagaraLocal();
	void SetDissolveValueLocal(float DissolveValue);
	void CleanupDissolveMaterialsLocal();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartTeleportOutPresentation(FVector BossStartLocation, FRotator BossStartRotation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBeginHiddenTeleportVFXPresentation(
		FVector BossStartLocation,
		FRotator BossStartRotation,
		FVector ConvergeLocation,
		FRotator ConvergeRotation,
		FVector LeftStartLocation,
		FVector LeftWanderLocation,
		FVector RightStartLocation,
		FVector RightWanderLocation,
		float InWanderSeconds,
		float InConvergeSeconds,
		bool bInDisableCollisionWhileHidden);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFinishTeleportVFXPresentation(FVector FinalLocation, FRotator FinalRotation, bool bPlayTeleportInStage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRevealSplashPresentation(FVector SplashLocation, FRotator SplashRotation, float SplashRadius);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCancelTeleportVFXPresentation(FVector FinalLocation, FRotator FinalRotation);

protected:
	// Teleport Out 진입에 사용할 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Animation")
	TObjectPtr<UAnimMontage> TeleportOutMontage;

	// Teleport Out 몽타주 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Animation", meta = (ClampMin = "0.0"))
	float TeleportOutMontagePlayRate = 1.0f;

	// Teleport In 정책이 켜진 패턴에서 사용할 등장 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Animation")
	TObjectPtr<UAnimMontage> TeleportInMontage;

	// Teleport In 몽타주 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Animation", meta = (ClampMin = "0.0"))
	float TeleportInMontagePlayRate = 1.0f;

	// Teleport Out 시작 후 디졸브를 시작할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve", meta = (ClampMin = "0.0"))
	float TeleportOutDissolveStartDelay = 0.45f;

	// Teleport Out 시작 후 실제로 보스를 숨기는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve", meta = (ClampMin = "0.0"))
	float TeleportOutVanishDelay = 0.75f;

	// Teleport Out 디졸브 보간 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve", meta = (ClampMin = "0.0"))
	float TeleportOutDissolveSeconds = 0.3f;

	// Teleport In 역디졸브 보간 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve", meta = (ClampMin = "0.0"))
	float TeleportInRevealSeconds = 0.35f;

	// Teleport In 시작 후 실제 공격 GA로 넘기기까지 기다릴 시간이다. 0이면 몽타주 길이를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Animation", meta = (ClampMin = "0.0"))
	float TeleportInFinishDelay = 0.0f;

	// 보스 머티리얼에 적용할 디졸브 scalar parameter 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	FName DissolveScalarParameterName = TEXT("DissolveAmount");

	// Teleport Out/In 중 보스 머티리얼 디졸브를 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	bool bUseMaterialDissolve = true;

	// Dissolve Niagara가 사용할 진행도 User Parameter 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	FName NiagaraDissolveParameterName = TEXT("User.DissolveAmount");

	// Dissolve 중 Mesh에 붙여 재생할 Niagara 시스템이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	TObjectPtr<UNiagaraSystem> DissolveNiagaraSystem;

	// Dissolve Niagara가 사용할 노이즈 텍스처다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	TObjectPtr<UTexture> DissolveTexture;

	// Dissolve Niagara 노이즈 텍스처 UV 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Dissolve")
	FVector2D DissolveTextureUV = FVector2D(1.0f, 1.0f);

	// 사라지는 시점 본체 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	TObjectPtr<UNiagaraSystem> BodyDisappearNiagaraSystem;

	// TeleportOut 시작 뒤 BodyDisappearNiagaraSystem을 재생할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float BodyDisappearNiagaraDelaySeconds = 0.0f;

	// 공중에서 이동하며 집결하는 두 개의 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	TObjectPtr<UNiagaraSystem> TeleportVFXNiagaraSystem;

	// 두 텔레포트 VFX 프로젝타일에 attach해 이동 동안 재생할 사운드 큐다. (각 프로젝타일에서 재생, 비어 있으면 미재생)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|SFX")
	TObjectPtr<USoundBase> TeleportVFXProjectileSoundCue;

	// 두 텔레포트 VFX 프로젝타일이 집결(합쳐질) 때 집결 위치에 재생할 사운드 큐다. 비어 있으면 재생하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|SFX")
	TObjectPtr<USoundBase> TeleportVFXMergeSoundCue;

	// Teleport In 등장 시 본체 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	TObjectPtr<UNiagaraSystem> BodyRevealNiagaraSystem;

	// Body Niagara가 루프형이어도 남지 않도록 로컬에서 강제 정리하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float TransientNiagaraLifeSeconds = 1.5f;

	// 본체 Niagara 위치를 가져올 Mesh 소켓 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	FName BodyNiagaraAttachSocketName = NAME_None;

	// 두 VFX가 보스 좌우에서 시작하는 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXStartSideOffset = 140.0f;

	// 두 VFX 시작 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	float VFXStartHeightOffset = 120.0f;

	// 공중 배회 위치의 좌우 확산 기준 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXWanderSideOffset = 650.0f;

	// 공중 배회 위치의 전후 드리프트 기준 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	float VFXWanderForwardOffset = 450.0f;

	// 공중 배회 위치의 기준 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	float VFXWanderHeightOffset = 360.0f;

	// 공중 배회 목표가 시작 위치에서 최소한 떨어질 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXWanderMinDistance = 1400.0f;

	// 공중 배회 목표가 시작 위치에서 최대한 떨어질 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXWanderMaxDistance = 2400.0f;

	// 사라진 뒤 공중에서 흩어져 이동하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXWanderSeconds = 0.35f;

	// 두 VFX가 공격 지점으로 동시에 집결하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	float VFXConvergeSeconds = 1.0f;

	// 집결 지점을 NavMesh로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX", meta = (ClampMin = "0.0"))
	FVector ConvergeNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);

	// 본체가 숨겨져 있는 동안 충돌을 끌지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	bool bDisableCollisionWhileHidden = true;

	// Teleport VFX 이후 보스가 다시 보이는 순간 주변 splash 피해를 줄지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash")
	bool bApplyRevealSplashDamage = true;

	// Reveal splash 피해를 적용하기 시작할 최소 페이즈다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	EPRBossPhase RevealSplashMinimumPhase = EPRBossPhase::Phase4;

	// Reveal splash에 사용할 피해 GameplayEffect다. 비어 있으면 AbilitySystemRegistry의 Enemy damage GE를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	TSubclassOf<UGameplayEffect> RevealSplashDamageEffectClass;

	// Reveal splash 중심 위치에 더할 보스 로컬 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	FVector RevealSplashLocalOffset = FVector::ZeroVector;

	// Reveal splash 피해 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage", ClampMin = "0.0"))
	float RevealSplashRadius = 420.0f;

	// Enemy AttackPower에 곱할 Reveal splash 피해 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage", ClampMin = "0.0"))
	float RevealSplashDamageMultiplier = 0.8f;

	// Reveal splash가 부여할 강인도 피해다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage", ClampMin = "0.0"))
	float RevealSplashPoiseDamage = 10.0f;

	// Reveal splash 시점에 재생할 Niagara 시스템이다. 비워 두면 피해만 적용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|VFX",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	TObjectPtr<UNiagaraSystem> RevealSplashNiagaraSystem;

	// Reveal splash Niagara를 소환할 보스 Mesh 소켓/본 이름이다. 비워 두면 피해 중심 위치에 소환한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|VFX",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	FName RevealSplashNiagaraSocketName = NAME_None;

	// Reveal splash Niagara 재생 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|VFX",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	FVector RevealSplashNiagaraScale = FVector::OneVector;

	// Reveal splash Niagara를 자동 정리하기까지 기다리는 시간이다. 0이면 자동 정리하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|VFX",
		meta = (EditCondition = "bApplyRevealSplashDamage", ClampMin = "0.0"))
	float RevealSplashNiagaraLifeSeconds = 1.25f;

	// Reveal splash 반경을 Niagara User Parameter로 전달할 이름이다. 비워 두면 전달하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|VFX",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	FName RevealSplashRadiusParameterName = TEXT("User.Radius");

	// Reveal splash 대상 탐색 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	TEnumAsByte<ECollisionChannel> RevealSplashOverlapChannel = ECC_Pawn;

	// Reveal splash 피해 반경을 디버그로 표시할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|Debug",
		meta = (EditCondition = "bApplyRevealSplashDamage"))
	bool bDrawDebugRevealSplash = false;

	// Reveal splash 디버그 표시 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX|Phase4Splash|Debug",
		meta = (EditCondition = "bDrawDebugRevealSplash", ClampMin = "0.0"))
	float RevealSplashDebugDrawDuration = 1.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> LeftTeleportVFXComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RightTeleportVFXComponent;

	// 두 VFX 프로젝타일에 attach한 비행 사운드 오디오 컴포넌트다. VFX 정리 시 함께 정지·파괴한다.
	// (오디오 owner는 보스라서 attach 컴포넌트 파괴만으로는 자동 정지되지 않으므로 직접 보관·정리한다.)
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LeftTeleportVFXAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RightTeleportVFXAudioComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> LocalTransientNiagaraComponents;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveDissolveNiagaraComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LocalDissolveDynamicMaterials;

	FTimerHandle TeleportOutDissolveTimerHandle;
	FTimerHandle BodyDisappearNiagaraTimerHandle;
	FTimerHandle TeleportOutVanishTimerHandle;
	FTimerHandle HiddenFinishTimerHandle;
	FTimerHandle TeleportInFinishTimerHandle;
	FVector CachedBossStartLocation = FVector::ZeroVector;
	FRotator CachedBossStartRotation = FRotator::ZeroRotator;
	FVector CachedConvergeLocation = FVector::ZeroVector;
	FRotator CachedConvergeRotation = FRotator::ZeroRotator;
	FVector LocalLeftStartLocation = FVector::ZeroVector;
	FVector LocalLeftWanderLocation = FVector::ZeroVector;
	FVector LocalRightStartLocation = FVector::ZeroVector;
	FVector LocalRightWanderLocation = FVector::ZeroVector;
	FVector LocalConvergeLocation = FVector::ZeroVector;
	float LocalPresentationElapsedSeconds = 0.0f;
	float LocalWanderSeconds = 0.0f;
	float LocalConvergeSeconds = 0.0f;
	float LocalDissolveStartValue = 1.0f;
	float LocalDissolveEndValue = 1.0f;
	float LocalDissolveDurationSeconds = 0.0f;
	float LocalDissolveElapsedSeconds = 0.0f;
	bool bTeleportVFXRunning = false;
	bool bLocalPresentationActive = false;
	bool bLocalDissolveActive = false;
	bool bLocalOriginalCollisionEnabled = true;
	bool bLocalDisableCollisionWhileHidden = true;
	bool bCachedPlayTeleportInStage = false;
	bool bSavedOwnerReplicateMovement = true;
	bool bHasSavedOwnerReplicateMovement = false;
};
