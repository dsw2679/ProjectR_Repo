// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (어빌리티 타겟 데이터 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "PRAbilityTargetData.generated.h"

// 클라이언트가 서버로 전달하는 투사체 스폰 정보. GAS TargetData로 송신
USTRUCT()
struct PROJECTR_API FPRTargetData_SpawnProjectile : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	// 클라이언트가 발급한 투사체 ID
	UPROPERTY()
	uint32 ProjectileId = 0;

	// 스폰 위치
	UPROPERTY()
	FVector SpawnLocation = FVector::ZeroVector;

	// 스폰 회전
	UPROPERTY()
	FRotator SpawnRotation = FRotator::ZeroRotator;

	/*~ FGameplayAbilityTargetData Interface ~*/
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FPRTargetData_SpawnProjectile::StaticStruct();
	}
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FPRTargetData_SpawnProjectile{Id=%u}"), ProjectileId);
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRTargetData_SpawnProjectile> : public TStructOpsTypeTraitsBase2<FPRTargetData_SpawnProjectile>
{
	enum { WithNetSerializer = true };
};
