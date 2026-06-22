// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 Rain Portal 경량 투사체(Niagara 배열 렌더 매니저) 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "PRFaerinRainProjectileManager.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

// 경량 rain 투사체 1개의 시뮬 상태다.
// 개별 액터/ProjectileMovementComponent 대신, 매니저가 이 구조체 배열을 일괄 시뮬레이션한다.
// 서버/클라가 동일한 결정론 낙하 시뮬을 돌려 시각을 맞추고, 충돌/피해는 서버에서만 판정한다.
USTRUCT()
struct FPRRainProjectileSim
{
	GENERATED_BODY()

	uint32 ProjectileId = 0;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.0f;
	float Lifetime = 0.0f;
	bool bAlive = false;
	
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedNiagara;
};

// 한 틱에 생성된 rain 투사체들을 한 번에 클라이언트로 전달하기 위한 배치 스폰 명령이다.
USTRUCT()
struct FPRRainSpawnCommand
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 ProjectileId = 0;

	UPROPERTY()
	FVector_NetQuantize Position = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize100 Velocity = FVector::ZeroVector;

	UPROPERTY()
	float Lifetime = 0.0f;
};

// 페어린 Rain Portal 전용 경량 투사체 매니저다.
// 다수의 rain 투사체를 단일 Niagara 시스템으로 렌더하고, 위치 배열을 매 틱 Niagara에 push한다.
// 기존 APRProjectileBase 경로(풀 액터 + PMC + per-actor 복제)를 대체하는 선택적 경량 경로이며,
// Rain Portal에서 토글이 켜졌을 때만 사용된다.
//
// [Niagara 시스템 작성 규약]
//  - User 파라미터로 Position 배열(Niagara Array Position) 1개: 기본 이름 "Positions" (PositionsParameterName)
//    -> C++는 SetNiagaraArrayPosition으로 push한다. (Array Float3가 아니라 Array Position)
//  - User 파라미터로 int 1개: 기본 이름 "Count" (CountParameterName) — 현재 살아있는 투사체 수
//  - 에미터는 Count개의 파티클을 Positions.Get(i) 월드 위치에 배치하도록 구성한다. (Local Space OFF 권장)
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinRainProjectileManager : public AActor
{
	GENERATED_BODY()

public:
	APRFaerinRainProjectileManager();

	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaSeconds) override;

	// 서버 권위에서 경량 rain 투사체 1발을 등록한다.
	// DamageSpec/Instigator는 서버 충돌 판정 시 피해 적용에만 쓰이며 복제되지 않는다.
	void ServerEmitRainProjectile(
		const FVector& SpawnLocation,
		const FVector& InitialVelocity,
		float Lifetime,
		const FGameplayEffectSpecHandle& DamageSpec,
		AActor* DamageInstigator);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 이번 틱 생성분을 한 번에 모든 클라이언트로 전달한다.
	// 누락 시 클라에 안 보이는 투사체가 서버 피해만 주는 불일치가 생기므로 Reliable로 둔다. (틱당 배치라 빈도는 낮음)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddRainProjectiles(const TArray<FPRRainSpawnCommand>& SpawnCommands);

	// 서버에서 피격 등으로 조기 소멸한 투사체들을 한 번에 클라이언트에서 제거한다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_RemoveRainProjectiles(const TArray<uint32>& ProjectileIds);

	// 모든 머신 공통: 낙하 이동 + 수명/킬존 판정.
	void SimulateProjectiles(float DeltaSeconds);

	// 서버 전용: 플레이어와의 거리 기반 피격 판정 및 피해 적용.
	void ServerCollisionAndDamage();

	// 죽은 투사체를 배열에서 제거한다. (서버는 DamageSpec 맵도 정리)
	void CompactDeadProjectiles();

	// 현재 살아있는 투사체 위치 배열을 Niagara user 파라미터로 push한다.
	void SyncNiagaraRender();

	// 서버 전용: 이번 틱 누적된 스폰/제거 배치를 멀티캐스트로 플러시한다.
	void FlushPendingNetCommands();

	// 서버 전용: 투사체가 모두 비고 일정 시간 idle이면 매니저를 자동 정리한다.
	void UpdateAutoShutdown(float DeltaSeconds);

	// 시뮬 배열에 투사체 1개를 추가한다. (모든 머신)
	void AddProjectileLocal(const FPRRainSpawnCommand& Command);

	// 지정 ID 투사체들을 시뮬 배열에서 제거한다. (모든 머신)
	void RemoveProjectilesByIds(const TArray<uint32>& ProjectileIds);

	// 후보 플레이어가 지금 피격 대상으로 유효한지 확인한다. (다운/사망 제외)
	bool IsDamageablePlayer(const AActor* PlayerActor) const;

protected:
	// rain 투사체를 렌더하는 단일 Niagara 컴포넌트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile")
	TObjectPtr<UNiagaraComponent> ProjectileNiagara;

	// rain 투사체 렌더에 사용할 Niagara 시스템이다. 위 규약대로 Positions/Count user 파라미터를 읽도록 작성한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Visual")
	TObjectPtr<UNiagaraSystem> ProjectileNiagaraSystem;

	// 투사체 위치 배열을 push할 Niagara user 파라미터(Vector 배열) 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Visual")
	FName PositionsParameterName = TEXT("Positions");

	// 살아있는 투사체 수를 push할 Niagara user 파라미터(int) 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Visual")
	FName CountParameterName = TEXT("Count");

	// 낙하 가속도(cm/s^2)다. 0이면 등속.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Motion")
	float GravityZ = -980.0f;

	// 플레이어 피격 판정 반경(cm)이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Collision", meta = (ClampMin = "1.0"))
	float HitRadius = 60.0f;

	// true면 KillBelowZ 아래로 내려간 투사체를 소멸시켜 지면 통과를 막는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Motion")
	bool bUseKillBelowZ = false;

	// bUseKillBelowZ가 켜졌을 때 이 Z 아래로 내려가면 소멸한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Motion", meta = (EditCondition = "bUseKillBelowZ"))
	float KillBelowZ = 0.0f;

	// 첫 발사 전, 투사체가 하나도 없을 때 매니저를 유지할 최대 시간(초)이다. (누수 방지 안전장치)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Lifecycle", meta = (ClampMin = "1.0"))
	float InitialIdleTimeout = 30.0f;

	// 첫 발사 후, 투사체가 모두 비면 매니저를 정리하기까지의 유예 시간(초)이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|RainProjectile|Lifecycle", meta = (ClampMin = "0.0"))
	float EmptyShutdownGrace = 3.0f;

private:
	// 살아있는 rain 투사체 시뮬 배열이다. (모든 머신 공통)
	UPROPERTY(Transient)
	TArray<FPRRainProjectileSim> Projectiles;

	// 서버 전용: 투사체 ID별 피해 적용 데이터다.
	struct FServerProjectileDamage
	{
		FGameplayEffectSpecHandle DamageSpec;
		TWeakObjectPtr<AActor> Instigator;
	};
	TMap<uint32, FServerProjectileDamage> ServerDamageById;

	// 서버 전용: 이번 틱 스폰 배치 / 조기 제거 배치다.
	TArray<FPRRainSpawnCommand> PendingSpawnCommands;
	TArray<uint32> PendingRemoveIds;

	// Niagara로 push할 위치 배열 임시 버퍼다. (재할당 최소화)
	TArray<FVector> PositionScratch;

	uint32 NextProjectileId = 1;
	float IdleSeconds = 0.0f;
	bool bHasEverEmitted = false;
};
