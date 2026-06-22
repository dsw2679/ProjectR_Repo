// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRPlayerMenuTypes.generated.h"

// 플레이어 메뉴 탭 유형
UENUM(BlueprintType)
enum class EPRPlayerMenuTabType : uint8
{
	Inventory = 0,
	Trait = 1,
	Bag = 2,
	InGameMenu = 3
};
