// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Weapon Animation 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRWeaponAnimationTypes.generated.h"

UENUM(BlueprintType)
enum class EPRWeaponAnimationState : uint8
{
	Idle,
	Shoot,
	Reload
};
