// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (투사체 매니저 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRProjectileTypes.h"
#include "Components/ActorComponent.h"
#include "PRProjectileManagerComponent.generated.h"

class APRProjectileBase;
inline constexpr uint32 NULL_PROJECTILE_ID = 0;

USTRUCT()
struct FPRLinkedProjectile
{
	GENERATED_BODY()

	TWeakObjectPtr<APRProjectileBase> PredictedProjectile;
	TWeakObjectPtr<APRProjectileBase> AuthProjectile;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRProjectileManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRProjectileManagerComponent();

	// 임의 액터(Pawn/Controller 등)에서 매니저를 검색. Pawn -> Controller 순으로 탐색
	static UPRProjectileManagerComponent* FindForActor(AActor* InActor);

	// ForwardPredictionTime 산출(초). 서버 측 Auth 투사체 Fast-Forward 양 (현재 스코프에선 미사용)
	float GetForwardPredictionTime() const;

	// 핑이 임계값보다 높을 때 클라이언트 Fake 스폰을 지연시키는 시간(초)
	float GetProjectileSpawnDelay() const;

	// Projectile Id 발급. 단조 증가, 0은 무효
	uint32 GenerateNewProjectileId();

	// 예측 투사체 등록
	void RegisterPredictedProjectile(uint32 Id, APRProjectileBase* ProjectileActor);

	// 예측 투사체 등록 해제 (Predicted/Auth 양쪽 모두 사라진 경우 엔트리 제거)
	void UnregisterPredictedProjectile(uint32 Id);

	// 예측 투사체 조회
	APRProjectileBase* FindPredictedProjectile(uint32 Id) const;

	// Auth 투사체가 도착했음을 매니저에 통보 (소유 클라/서버 공통 등록 경로)
	void NotifyAuthArrived(uint32 Id, APRProjectileBase* AuthProjectile);

	// 클라이언트 측 예측 투사체 즉시 생성. ProjectileId가 0이면 신규 발급
	// 지연 스폰이 필요한 경우 호출자(AT 등)가 자체 타이머로 본 함수 호출 시점을 제어
	APRProjectileBase* SpawnPredictedProjectile(FPRProjectileSpawnInfo& SpawnInfo);

	// 서버 측 권위 투사체 생성. ProjectileId는 클라가 보낸 값을 그대로 사용
	APRProjectileBase* SpawnAuthProjectile(FPRProjectileSpawnInfo& SpawnInfo);

	// Predicted/Auth 페어 맵에서 지움
	void ClearProjectile(uint32 Id);
	
protected:
	// 핑 보정 퍼지 팩터(ms). 엔진 ExactPing 과대추정 보정용
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	float PredictionLatencyReduction = 20.f;

	// 0.0 ~ 1.0. 0=서버 위치 그대로, 1=클라 위치까지 완전 전진
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ClientBiasPct = 0.5f;

	// 이 핑(ms)을 초과하면 클라 Prediction 스폰을 지연
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	float MaxPredictionPing = 120.f;

private:
	// 컴포넌트가 부착된 액터로부터 OwningPlayerController 추출
	APlayerController* ResolveOwningController() const;

	// 단조 증가 ID 발급용 카운터
	uint32 NextSpawnedProjectileId = 1;

	// Predicted/Auth 페어 맵
	UPROPERTY(Transient)
	TMap<uint32, FPRLinkedProjectile> SpawnedProjectilesMap;
};
