// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy 공격 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PREnemyAttackTypes.generated.h"

// 적 근접 공격 판정의 기준 소스를 정의한다.
UENUM(BlueprintType)
enum class EPREnemyAttackCollisionSource : uint8
{
	ForwardSweep		UMETA(DisplayName = "Forward Sweep"),
	SocketOrBone		UMETA(DisplayName = "Socket Or Bone"),
	PhysicsAssetBody	UMETA(DisplayName = "Physics Asset Body"),
	HelperActor			UMETA(DisplayName = "Helper Actor")
};

// 적 공격 판정에 필요한 충돌 설정이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttackCollisionConfig
{
	GENERATED_BODY()

	// 공격 판정이 참조할 위치 소스다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	EPREnemyAttackCollisionSource CollisionSource = EPREnemyAttackCollisionSource::SocketOrBone;

	// 소켓/본 기반 판정에서 사용할 기준 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	FName AttackTraceSourceName = NAME_None;

	// 기준 소켓/본/Body에서 판정 중심을 보정하는 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	FVector AttackTraceSourceOffset = FVector::ZeroVector;

	// PhysicsAsset Body 기반 판정에서 사용할 공격 전용 Body 이름 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	TArray<FName> AttackPhysicsBodyNames;

	// 정면 Sweep 기반 판정에서 사용할 전방 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 0.0f;

	// 구체 Sweep 기반 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 0.0f;

	// PhysicsAsset Body 기반 판정에서 Body별 Sweep 반경에 곱할 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float PhysicsBodyScale = 1.0f;

	// 공격 판정에 사용할 충돌 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;
};

// 적 공격의 피해 배율과 충돌 설정을 함께 묶은 공용 데이터다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttackHitConfig
{
	GENERATED_BODY()

	// EnemyStatRow AttackPower에 곱해 Health 피해를 산출하는 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// 플레이어에게 적용할 고정 강인도 피해다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float PoiseDamage = 0.0f;

	// 이 공격이 사용할 판정 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	FPREnemyAttackCollisionConfig CollisionConfig;
};
