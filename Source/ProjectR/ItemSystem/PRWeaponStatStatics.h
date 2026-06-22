// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRWeaponStatStatics.generated.h"

class UPRItemInstance_Weapon;
class UPRWeaponDataAsset;

// 무기 스탯 계산 공용 함수 모음
UCLASS()
class PROJECTR_API UPRWeaponStatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 강화 단계가 반영된 기본 공격력 계산
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Stat")
	static float CalculateUpgradedBaseDamage(const UPRWeaponDataAsset* WeaponData, int32 UpgradeLevel);

	// 무기 인스턴스의 현재 강화 상태가 반영된 기본 공격력 계산
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Stat")
	static float CalculateWeaponItemBaseDamage(const UPRItemInstance_Weapon* WeaponItem);
};
