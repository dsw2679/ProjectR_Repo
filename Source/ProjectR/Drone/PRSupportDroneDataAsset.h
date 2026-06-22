// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Support Drone 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRSupportDroneDataAsset.generated.h"

class APRProjectileBase;
class APRSupportDroneActor;

// 플레이어 보조 드론의 이동, 탐색, 공격 설정 데이터
UCLASS(BlueprintType)
class PROJECTR_API UPRSupportDroneDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 플레이어 기준 추종 목표 위치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Follow")
	FVector FollowOffset = FVector(-120.0f, 90.0f, 170.0f);

	// 추종 보간 속도다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Follow", meta = (ClampMin = "0.0"))
	float FollowInterpSpeed = 7.0f;

	// 플레이어와 너무 멀어진 드론을 즉시 복귀시키는 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Follow", meta = (ClampMin = "0.0"))
	float TeleportDistance = 2500.0f;

	// 부유 연출 높이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Hover", meta = (ClampMin = "0.0"))
	float HoverAmplitude = 18.0f;

	// 부유 연출 주기다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Hover", meta = (ClampMin = "0.0"))
	float HoverFrequency = 1.4f;

	// 적 탐색 반경이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Target", meta = (ClampMin = "0.0"))
	float TargetSearchRadius = 2200.0f;

	// 대상 재탐색 주기다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Target", meta = (ClampMin = "0.05"))
	float TargetRefreshInterval = 0.25f;

	// 플레이어 최근 공격 대상 유지 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Target", meta = (ClampMin = "0.0"))
	float AssistTargetHoldDuration = 3.0f;

	// 발사 사거리다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Attack", meta = (ClampMin = "0.0"))
	float AttackRange = 2000.0f;

	// 발사 주기다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Attack", meta = (ClampMin = "0.05"))
	float AttackInterval = 1.2f;

	// 시야선 검사 여부다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Attack")
	bool bRequireLineOfSight = true;

	// 시야선 검사 채널이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Attack")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	// 드론 미사일 클래스다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Projectile")
	TSubclassOf<APRProjectileBase> MissileProjectileClass;

	// 미사일 초기 속도 재정의다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Projectile", meta = (ClampMin = "0.0"))
	float MissileSpeedOverride = 0.0f;

	// 미사일 호밍 가속도다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Projectile", meta = (ClampMin = "0.0"))
	float MissileHomingAcceleration = 8000.0f;

	// 미사일 호밍 시작 지연이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Projectile", meta = (ClampMin = "0.0"))
	float MissileHomingStartDelay = 0.05f;

	// 미사일 호밍 유지 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Projectile", meta = (ClampMin = "0.0"))
	float MissileHomingDuration = 0.0f;

	// 미사일 피해량이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Damage", meta = (ClampMin = "0.0"))
	float MissileDamage = 25.0f;

	// 미사일 그로기 피해량이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Damage", meta = (ClampMin = "0.0"))
	float MissileGroggyDamage = 5.0f;

	// 소환 지속 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone|Lifetime", meta = (ClampMin = "0.0"))
	float LifeSpan = 20.0f;
};
