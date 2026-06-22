// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (스폰 Predicted 투사체 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ProjectR/Projectile/PRProjectileTypes.h"
#include "ProjectR/AbilitySystem/PRAbilityTargetData.h"
#include "PRAT_SpawnPredictedProjectile.generated.h"

class APRProjectileBase;
class UPRProjectileManagerComponent;


DECLARE_LOG_CATEGORY_EXTERN(LogPRSpawnPredictedProjectile, Log, All);

UCLASS()
class PROJECTR_API UPRAT_SpawnPredictedProjectile : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "True"), Category = "Ability|Tasks")
	static UPRAT_SpawnPredictedProjectile* SpawnPredictedProjectile(UGameplayAbility* OwningAbility,
		TSubclassOf<APRProjectileBase> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);


	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

public:
	// Projectile 스폰 성공시 호출 (Client: 예측 투사체, Server: 권위 투사체)
	UPROPERTY(BlueprintReadOnly)
	FProjectileSpawnedDelegate OnSuccess;

	// Projectile 스폰 실패시 호출 (예측 실패)
	UPROPERTY(BlueprintReadOnly)
	FProjectileSpawnedDelegate OnFailed;

protected:
	// 클라이언트: 예측 투사체 스폰 + TargetData 송신
	void HandleLocalPredictingClient(UPRProjectileManagerComponent* Manager);

	// 서버 + 로컬(Standalone/Listen Host): Auth 즉시 스폰
	void HandleAuthorityLocal(UPRProjectileManagerComponent* Manager);

	// 전용 서버: TargetData 대기
	void HandleAuthorityRemote();

	// TargetData 수신 콜백 (서버)
	void OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);

	// TargetData 취소 콜백 (서버)
	void OnTargetDataCancelled();

	// 예측 거부 콜백 (클라이언트). 스폰한 예측 투사체 정리
	void HandlePredictionRejected();

	// 자체 타이머 만료 시 호출. 지연 분기에서 실제 예측 스폰 + TargetData 송신 수행
	void ExecuteDelayedSpawn();

	// 클라이언트가 서버로 TargetData 송신
	void SendSpawnDataToServer(uint32 ProjectileId);

	// 종료 헬퍼
	void BroadcastFailureAndEnd();

protected:
	// ===== Spawn Params =====
	UPROPERTY()
	TSubclassOf<APRProjectileBase> ProjectileClass;
	FVector SpawnLocation;
	FRotator SpawnRotation;

	// 이 Task가 서버에서 reject된 경우 (예측 실패) 스폰한 예측 투사체를 파괴하기 위해 캐싱
	TWeakObjectPtr<APRProjectileBase> SpawnedPredictedProjectile;

	// 예측 투사체 ID 캐시 (rejection 시 매니저 맵 정리에 사용)
	uint32 CachedProjectileId = 0;

	// Manager 캐시
	TWeakObjectPtr<UPRProjectileManagerComponent> CachedManager;

	// 지연 스폰 사용 여부
	bool bUsedDelayedSpawn = false;

	// 지연 스폰 타이머. AT가 자체 소유하여 OnDestroy 시 자동 정리
	FTimerHandle DelayedSpawnTimer;

	// 지연 분기에서 만료 시점에 사용할 스폰 파라미터 보관
	FPRProjectileSpawnInfo PendingDelayedSpawnInfo;
};
