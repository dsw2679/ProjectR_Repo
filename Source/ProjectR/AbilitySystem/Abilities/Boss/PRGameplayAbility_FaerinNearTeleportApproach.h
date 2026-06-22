// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Near 순간이동 접근 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRGameplayAbility_FaerinNearTeleportApproach.generated.h"

class AAIController;
class ACharacter;
class APREnemyBaseCharacter;
class UEnvQuery;
class UNiagaraSystem;
class USoundBase;
class UTexture;

// Faerin 3페이즈 이전 접근 루프에서 사용하는 근거리 텔레포트 접근 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinNearTeleportApproach : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinNearTeleportApproach();

	// SprintOrNearTeleport 접근 정책에서 이 GA를 선택할 확률을 반환한다.
	float GetApproachSelectionChance() const { return ApproachSelectionChance; }

	/*~ UGameplayAbility Interface ~*/
public:
	// Director가 넘긴 요청을 읽고 사라짐 후 자기 주변 재등장 접근을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 진행 중인 타이머와 임시 숨김/충돌 상태를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 소유 Faerin Director에서 이번 근거리 텔레포트 요청 데이터를 가져온다.
	bool ResolveTeleportRequest(APREnemyBaseCharacter* EnemyCharacter);

	// 사라짐 VFX를 먼저 보여주고 실제 텔레포트 전환 타이머를 예약한다.
	void BeginDisappear(ACharacter* BossCharacter);

	// 사라짐 디졸브가 보이는 시간을 보장한 뒤 숨김 상태와 재등장 대기 시간을 적용한다.
	void CommitDisappearAndScheduleReappear();

	// 자기 주변 EQS 목적지를 계산한 뒤 보스를 재등장시킨다.
	void ReappearNearSelf();

	// 현재 보스 위치 기준 EQS 결과로 재등장 위치와 회전을 계산한다.
	bool ResolveReappearTransform(FVector& OutLocation, FRotator& OutRotation) const;

	// NavMesh/EQS 후보를 실제 보스 캡슐이 들어갈 수 있는 최종 텔레포트 위치로 확정한다.
	bool FinalizeReappearTeleportSpot(AActor& AvatarActor, bool bTreatLocationAsFloorPoint, FVector& InOutLocation, FRotator& InOutRotation) const;

	// TargetBack 방식으로 타겟의 등 뒤 재등장 위치를 계산한다.
	bool ResolveTargetBackReappearLocation(FVector& OutLocation) const;

	// 타겟 기준 앞/뒤 후보 위치를 NavMesh와 방향 조건으로 검증한다.
	bool ResolveTargetRelativeReappearLocation(const AActor& TargetActor, float DirectionSign, FVector& OutLocation) const;

	// 근거리 텔레포트 위치 선정 EQS를 실행한다.
	bool ResolveEQSReappearLocation(FVector& OutLocation) const;

	// EQS 실패 시 현재 보스 주변 NavMesh에서 재등장 후보 위치를 찾는다.
	bool ResolveNavmeshFallbackReappearLocation(FVector& OutLocation) const;

	// EQS 위치 선정 실패 시 보스의 HomeLocation을 재등장 위치로 사용한다.
	bool ResolveHomeReappearLocation(FVector& OutLocation) const;

	// 보스 몸에 연결되는 근거리 텔레포트 순간 Niagara를 모든 클라이언트에 요청한다.
	void SpawnBodyNiagara(UNiagaraSystem* NiagaraSystem, float LifeSeconds) const;

	// Death GA의 Dissolve 처리 방식과 동일하게 근거리 텔레포트 몸 Dissolve 시각 연출을 요청한다.
	void PlayNearTeleportDissolveVisual(UNiagaraSystem* NiagaraSystem, float Duration, float StartValue, float EndValue) const;

	// 보스의 숨김/충돌 상태를 원래 전투 가능 상태로 되돌린다.
	void RestoreBossPresentation(ACharacter* BossCharacter) const;
	void BeginBossMovementReplicationOverride(ACharacter* BossCharacter);
	void EndBossMovementReplicationOverride(ACharacter* BossCharacter);

	// 현재 Ability를 종료한다.
	void FinishNearTeleport(bool bWasCancelled);

protected:
	// 사라지는 순간 보스 몸에 붙일 디졸브 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> DisappearDissolveNiagaraSystem;

	// 사라지는 순간 보스 몸에 붙일 텔레포트 인 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> TeleportInNiagaraSystem;

	// 재등장 순간 보스 몸에 붙일 텔레포트 아웃 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> TeleportOutNiagaraSystem;

	// 텔레포트 인 Niagara를 이 시간 뒤 강제로 비활성화/삭제한다. 0 이하면 Niagara 자체 AutoDestroy만 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Lifetime", meta = (ClampMin = "0.0"))
	float TeleportInBodyNiagaraLifeSeconds = 0.0f;

	// 텔레포트 아웃 Niagara를 이 시간 뒤 강제로 비활성화/삭제한다. 0 이하면 Niagara 자체 AutoDestroy만 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Lifetime", meta = (ClampMin = "0.0"))
	float TeleportOutBodyNiagaraLifeSeconds = 0.0f;

	// 재등장 순간 보스 몸에 붙일 역방향 디졸브 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> ReappearDissolveNiagaraSystem;

	// 1,2페이즈 근거리 텔레포트로 사라질 때 보스 위치에 재생할 사운드 큐다. 비어 있으면 재생하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|SFX")
	TObjectPtr<USoundBase> NearTeleportDisappearSoundCue;

	// 1,2페이즈 근거리 텔레포트로 재등장할 때 재등장 위치에 재생할 사운드 큐다. 비어 있으면 재생하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|SFX")
	TObjectPtr<USoundBase> NearTeleportReappearSoundCue;


	// 사라짐 디졸브 Niagara/Material 파라미터 보간 시간이다. 0 이하면 즉시 종료값으로 적용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve", meta = (ClampMin = "0.0"))
	float DisappearDissolveDuration = 0.5f;

	// 재등장 역디졸브 Niagara/Material 파라미터 보간 시간이다. 0 이하면 즉시 종료값으로 적용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve", meta = (ClampMin = "0.0"))
	float ReappearDissolveDuration = 0.5f;

	// 사라짐 디졸브 시작 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	float DisappearDissolveStartValue = 1.0f;

	// 사라짐 디졸브 종료 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	float DisappearDissolveEndValue = 0.0f;

	// 재등장 역디졸브 시작 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	float ReappearDissolveStartValue = 0.0f;

	// 재등장 역디졸브 종료 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	float ReappearDissolveEndValue = 1.0f;

	// Dissolve 재질에서 사용하는 Scalar Parameter 이름이다. Death GA와 기본값을 맞춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	FName DissolveScalarParameterName = TEXT("DissolveAmount");

	// Dissolve Niagara에 전달할 진행도 User Parameter 이름이다. Death GA와 기본값을 맞춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	FName NiagaraDissolveParameterName = TEXT("User.DissolveAmount");

	// Dissolve Niagara가 사용하는 노이즈 텍스처다. 비워 두면 전달하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	TObjectPtr<UTexture> DissolveTexture;

	// Dissolve Niagara 노이즈 텍스처 UV 스케일이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve")
	FVector2D DissolveTextureUV = FVector2D(1.0f, 1.0f);

	// Dissolve Material/Niagara 진행값 갱신 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX|Dissolve", meta = (ClampMin = "0.001"))
	float DissolveTickInterval = 0.016f;

	// 몸 Niagara를 붙일 소켓 이름이다. 비워 두면 Mesh 루트에 붙는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	FName BodyNiagaraAttachSocketName = NAME_None;

	// 보스가 숨겨지기 전에 디졸브/텔레포트 인 VFX를 보여줄 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX", meta = (ClampMin = "0.0"))
	float DisappearPresentationSeconds = 0.5f;

	// SprintOrNearTeleport 정책에서 근거리 텔레포트를 선택할 확률이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|Selection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ApproachSelectionChance = 0.35f;

	// 사라진 뒤 자기 주변 위치에 재등장하기까지의 전체 대기 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|Execution", meta = (ClampMin = "0.0"))
	float ReappearDelaySeconds = 0.5f;

	// 사라진 위치 기준 근거리 텔레포트가 이동할 수 있는 최대 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|Execution", meta = (ClampMin = "0.0"))
	float MaxDistanceFromSelf = 500.0f;

	// 자기 주변 근거리 텔레포트 목적지를 고를 EQS다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TObjectPtr<UEnvQuery> QueryTemplate;

	// 근거리 텔레포트 EQS 실행 방식이다. 후보 랜덤 선택이면 내부 유틸에서 AllMatching으로 보정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> QueryRunMode = EEnvQueryRunMode::SingleResult;

	// 근거리 텔레포트 EQS 후보 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::RandomTopCandidates;

	// 근거리 텔레포트 EQS Named Float Param 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 근거리 텔레포트 상위 후보 최대 선택 개수다. 0 이하면 개수 제한이 없다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS", meta = (ClampMin = "0"))
	int32 TopCandidateCount = 5;

	// 근거리 텔레포트 최고 점수 대비 허용할 최소 후보 점수 비율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.85f;

	// 재등장 위치를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS", meta = (ClampMin = "0.0"))
	FVector ReappearNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);

	// EQS 실패 시 현재 보스 주변 NavMesh 후보를 탐색하는 횟수다. 0이면 HomeLocation fallback만 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|NavmeshFallback", meta = (ClampMin = "0"))
	int32 NavmeshFallbackAttempts = 12;

	// NavMesh fallback 후보가 현재 위치에서 최소한 떨어져야 하는 2D 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|NavmeshFallback", meta = (ClampMin = "0.0"))
	float NavmeshFallbackMinDistanceFromSelf = 120.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCombatDirectorComponent> ActiveDirectorComponent;

	FPRFaerinNearTeleportRequest ActiveRequest;
	FTimerHandle ReappearTimerHandle;
	FVector DisappearLocation = FVector::ZeroVector;
	FVector CachedReappearLocation = FVector::ZeroVector;
	FRotator CachedReappearRotation = FRotator::ZeroRotator;
	bool bOriginalActorCollisionEnabled = true;
	bool bHasCachedReappearTransform = false;
	bool bNearTeleportFinished = false;
	bool bPresentationRestoredDuringTeleport = false;
	bool bSavedBossReplicateMovement = true;
	bool bHasSavedBossReplicateMovement = false;
};
