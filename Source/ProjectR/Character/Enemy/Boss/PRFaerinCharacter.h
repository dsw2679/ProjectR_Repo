// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (페어린 보스 전용 체력 UI 연동)
// Author: 배유찬 (페어린 보스 조우 상호작용 연동)
// Author: 손승우 (페어린 보스 고유 공격 패턴(GodFall/소환/검격) 및 VFX 연출 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "UObject/SoftObjectPath.h"
#include "TimerManager.h"
#include "PRFaerinCharacter.generated.h"

class UPRFaerinDebugDrawComponent;
class UPRFaerinCombatDirectorComponent;
class UPRFaerinGodFallComponent;
class UPRFaerinPatternVFXComponent;
class UPRFaerinTeleportVFXComponent;
class UMotionWarpingComponent;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UTexture;
class APRPlayerCharacter;
struct FPREnemyTargetingConfig;

// Faerin 보스 본체 클래스다.
// 패턴 분기와 포털/검 생성 로직은 넣지 않고, 보스 공통 베이스에 Faerin 기본 데이터만 얹는다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCharacter : public APRBossBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCharacter();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/*~ APRBossBaseCharacter Interface ~*/
	// 보스와 플레이어의 실제 교전 확인 후 HUD 조우 시작 요청
	virtual void RequestBossEncounterBegin() override;

	/*~ IPREnemyInterface ~*/
	virtual void CustomizeEnemyTargetingConfig(FPREnemyTargetingConfig& InOutTargetingConfig) const override;
	
	/*~ APRFaerinCharacter Interface ~*/ 
	// God Fall 맵 배치 Rig 전환과 지속 검 hazard를 담당하는 component를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	UPRFaerinGodFallComponent* GetGodFallComponent() const { return GodFallComponent; }

	// Phase3 이후 공격 전 Teleport Out / VFX 집결 연출 컴포넌트를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	UPRFaerinTeleportVFXComponent* GetTeleportVFXComponent() const { return TeleportVFXComponent; }

	// Faerin 패턴 VFX를 수동으로 켜고 끄는 컴포넌트를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	UPRFaerinPatternVFXComponent* GetPatternVFXComponent() const { return PatternVFXComponent; }

	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin")
	bool IsBossEncounterActive() const { return bBossEncounterActive; }

	// 인카운터 전투 시작 시 참가자와 주 대상자를 ThreatComponent에 주입한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void SeedEncounterTargets(
		const TArray<APRPlayerCharacter*>& Participants,
		APRPlayerCharacter* PrimaryTarget,
		float ThreatAmount = 1000.0f);

	// 근거리 텔레포트 순간 보스 몸 위치의 Niagara를 모든 클라이언트에 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnNearTeleportBodyNiagara(UNiagaraSystem* NiagaraSystem, FName AttachSocketName, float LifeSeconds);

	// Death Dissolve와 같은 방식으로 근거리 텔레포트용 몸 Dissolve Niagara/Material 파라미터를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNearTeleportDissolveVisual(
		UNiagaraSystem* DissolveNiagaraSystem,
		FName AttachSocketName,
		float DissolveDuration,
		float DissolveStartValue,
		float DissolveEndValue,
		FName DissolveScalarParameterName,
		FName NiagaraDissolveParameterName,
		UTexture* DissolveTexture,
		FVector2D DissolveTextureUV,
		float DissolveTickInterval);

	// 서버가 확정한 월드 위치에 Faerin 패턴용 Niagara를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnFaerinWorldNiagara(FSoftObjectPath NiagaraSystemPath,
		FVector_NetQuantize Location,
		FRotator Rotation,
		FVector Scale,
		float LifeSeconds);

	// 서버가 지정한 월드 위치에 Faerin SFX를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFaerinSoundAtLocation(USoundBase* Sound, FVector_NetQuantize Location);

	// 서버가 확정한 월드 위치에 Faerin 패턴용 Niagara를 key 기반으로 재생한다. 이후 같은 key로 즉시 정리할 수 있다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnFaerinTrackedWorldNiagara(FName EffectKey,
		FSoftObjectPath NiagaraSystemPath,
		FVector_NetQuantize Location,
		FRotator Rotation,
		FVector Scale,
		float LifeSeconds);

	// key 기반으로 생성된 Faerin 패턴용 Niagara를 모든 클라이언트에서 즉시 정리한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopFaerinTrackedNiagara(FName EffectKey);

	// 지정 대상 Actor의 Mesh/Socket에 Faerin 패턴용 Niagara를 모든 클라이언트에서 부착 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnFaerinAttachedNiagara(FSoftObjectPath NiagaraSystemPath,
		AActor* AttachActor,
		FName AttachSocketName,
		FVector LocationOffset,
		FRotator RotationOffset,
		FVector Scale,
		float LifeSeconds);

	// 근거리 텔레포트 재등장 위치와 VFX를 모든 클라이언트에 같은 순서로 적용한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNearTeleportReappearPresentation(
		FVector ReappearLocation,
		FRotator ReappearRotation,
		UNiagaraSystem* TeleportOutNiagaraSystem,
		UNiagaraSystem* ReappearDissolveNiagaraSystem,
		FName AttachSocketName,
		float TeleportOutLifeSeconds,
		float ReappearDissolveLifeSeconds);

	// 근거리 텔레포트 사라짐/재등장 상태를 모든 클라이언트에 맞춘다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetNearTeleportHidden(bool bShouldHide);

	// 분신이 본체에 복귀했을 때 서버에서 페어린 체력을 회복하고 회복 VFX를 전파한다.
	void ApplyFaerinCloneMergeHeal(
		float HealAmount,
		float HealMaxHealthRatio,
		UNiagaraSystem* HealNiagaraSystem,
		FName AttachSocketName,
		FVector LocationOffset,
		FRotator RotationOffset,
		FVector Scale,
		float LifeSeconds);

	// 분신 복귀 회복 Niagara를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnFaerinCloneHealNiagara(
		UNiagaraSystem* HealNiagaraSystem,
		FName AttachSocketName,
		FVector LocationOffset,
		FRotator RotationOffset,
		FVector Scale,
		float LifeSeconds);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Faerin PIE 패턴/감지 범위를 표시하는 디버그 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRFaerinDebugDrawComponent> DebugDrawComponent;

	// Faerin 원작형 공격 루프를 실행하는 전투 디렉터 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<UPRFaerinCombatDirectorComponent> CombatDirectorComponent;

	// Phase2 God Fall 진입과 StaticSword hazard 생명주기를 관리하는 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	TObjectPtr<UPRFaerinGodFallComponent> GodFallComponent;

	// Phase3 이후 본체 공격 전 Teleport Out / VFX 집결 연출을 관리하는 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin|TeleportVFX")
	TObjectPtr<UPRFaerinTeleportVFXComponent> TeleportVFXComponent;

	// 특정 패턴에서 수동 호출로 표시/정리할 Faerin 패턴 VFX 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	TObjectPtr<UPRFaerinPatternVFXComponent> PatternVFXComponent;

	// 공격 루트모션 방향 보정을 위한 Motion Warping 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

private:
	// 현재 보스 Mesh 위치를 기준으로 근거리 텔레포트 Niagara를 재생한다.
	void SpawnNearTeleportBodyNiagaraLocal(UNiagaraSystem* NiagaraSystem, FName AttachSocketName, float LifeSeconds) const;

	// 서버가 전달한 soft path를 클라이언트에서 로드해 월드 Niagara를 재생한다.
	void SpawnFaerinWorldNiagaraLocal(const FSoftObjectPath& NiagaraSystemPath,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		float LifeSeconds) const;

	// 서버가 전달한 soft path를 클라이언트에서 로드해 key 기반 월드 Niagara를 재생한다.
	void SpawnFaerinTrackedWorldNiagaraLocal(FName EffectKey,
		const FSoftObjectPath& NiagaraSystemPath,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		float LifeSeconds);

	// key 기반 월드 Niagara를 즉시 정리한다.
	void StopFaerinTrackedNiagaraLocal(FName EffectKey);

	// 서버가 전달한 soft path를 클라이언트에서 로드해 대상 Actor의 Mesh/Socket에 Niagara를 부착 재생한다.
	void SpawnFaerinAttachedNiagaraLocal(const FSoftObjectPath& NiagaraSystemPath,
		AActor* AttachActor,
		FName AttachSocketName,
		const FVector& LocationOffset,
		const FRotator& RotationOffset,
		const FVector& Scale,
		float LifeSeconds) const;


	// 근거리 텔레포트 사라짐/재등장 Dissolve Material/Niagara 진행 값을 초기화하고 보간한다.
	void StartNearTeleportDissolveVisual(
		UNiagaraSystem* DissolveNiagaraSystem,
		FName AttachSocketName,
		float DissolveDuration,
		float DissolveStartValue,
		float DissolveEndValue,
		FName DissolveScalarParameterName,
		FName NiagaraDissolveParameterName,
		UTexture* DissolveTexture,
		const FVector2D& DissolveTextureUV,
		float DissolveTickInterval);
	void TickNearTeleportDissolveVisual();
	void CompleteNearTeleportDissolveVisual();
	void ApplyNearTeleportDissolveVisualValue(float DissolveValue);

	// EventManager로 보스 조우 시작 알림 브로드캐스트
	void BroadcastBossEncounterBegin();

	// EventManager로 보스 조우 종료 알림 브로드캐스트
	void BroadcastBossEncounterEnd();

	void SetBossEncounterActive(bool bActive);
	void HandleBossEncounterActiveChanged();

	UFUNCTION()
	void OnRep_BossEncounterActive();

private:
	UPROPERTY(ReplicatedUsing = OnRep_BossEncounterActive)
	bool bBossEncounterActive = false;

	TMap<FName, TWeakObjectPtr<UNiagaraComponent>> TrackedFaerinNiagaraComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> NearTeleportDissolveDynamicMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNearTeleportDissolveNiagara;
	FTimerHandle NearTeleportDissolveTickTimerHandle;
	float NearTeleportDissolveElapsedTime = 0.0f;
	float NearTeleportDissolveDuration = 0.0f;
	float NearTeleportDissolveStartValue = 0.0f;
	float NearTeleportDissolveEndValue = 1.0f;
	float NearTeleportDissolveTickInterval = 0.016f;
	FName NearTeleportDissolveScalarParameterName = TEXT("DissolveAmount");
	FName NearTeleportNiagaraDissolveParameterName = TEXT("User.DissolveAmount");

	bool bBossEncounterEventBroadcasted = false;
};
