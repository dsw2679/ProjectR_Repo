// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (사격 어빌리티 연동 기본 데이터 정의)
// Author: 이건주 (Penitent 원거리 사격 및 배리어 패턴 데이터 구축)
#pragma once

#include "CoreMinimal.h"
#include "PREnemyCombatDataAsset.h"
#include "UPRPenitentCombatDataAsset.generated.h"

// Penitent 거리 판정 구간
UENUM(BlueprintType)
enum class EPRPenitentDistanceBand : uint8
{
	NoTarget		UMETA(DisplayName = "NoTarget"),
	Melee			UMETA(DisplayName = "Melee"),
	CloserThanSpace	UMETA(DisplayName = "CloserThanSpace"),
	Spacing			UMETA(DisplayName = "Spacing"),
	FartherThanSpace	UMETA(DisplayName = "FartherThanSpace"),
	OutOfRanged		UMETA(DisplayName = "OutOfRanged")
};

// Penitent 교전 거리 조절 방향
UENUM(BlueprintType)
enum class EPRPenitentCombatSpacingDirection : uint8
{
	None		UMETA(DisplayName = "None"),
	Approach	UMETA(DisplayName = "Approach"),
	Hold		UMETA(DisplayName = "Hold"),
	Retreat		UMETA(DisplayName = "Retreat")
};

// Penitent 전투 거리와 공격 후 교전 거리 조절 시간
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPenitentCombatRangeData
{
	GENERATED_BODY()

	// 근거리 공격 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float MeleeMaxRange = 250.0f;

	// 교전 거리 조절 최소 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float SpacingMinRange = 600.0f;

	// 교전 거리 조절 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float SpacingMaxRange = 1000.0f;

	// 원거리 공격 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float RangedMaxRange = 1800.0f;

	// 공격 후 교전 거리 조절 최소 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float CombatSpacingMinTime = 2.0f;

	// 공격 후 교전 거리 조절 최대 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent|Range", meta = (ClampMin = "0.0"))
	float CombatSpacingMaxTime = 3.0f;

	// 거리 구간 오름차순 검증
	bool IsValidRangeOrder() const
	{
		return 0.0f <= MeleeMaxRange
			&& MeleeMaxRange <= SpacingMinRange
			&& SpacingMinRange <= SpacingMaxRange
			&& SpacingMaxRange <= RangedMaxRange
			&& CombatSpacingMinTime <= CombatSpacingMaxTime;
	}
};

// Penitent 전용 전투 데이터 자산
UCLASS(BlueprintType)
class PROJECTR_API UPRPenitentCombatDataAsset : public UPREnemyCombatDataAsset
{
	GENERATED_BODY()

public:
	// Penitent 거리 판정 기준
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Penitent")
	FPRPenitentCombatRangeData CombatRangeData;
};
