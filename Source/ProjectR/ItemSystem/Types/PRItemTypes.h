// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (상점 거래용 아이템 분류 타입 정의)
// Author: 배유찬 (아이템 등급 및 장비 슬롯 분류 타입 정의)
// Author: 이건주 (인벤토리 3D 프리뷰용 아이템 분류 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRItemTypes.generated.h"

UENUM(BlueprintType)
enum class EPRItemType : uint8
{
	None,
	Weapon,
	Mod,
	PrimaryWeapon,
	SecondaryWeapon,
	Consumable,
	Material,
	Equipment,
	Ammo
};

UENUM(BlueprintType)
enum class EPRItemRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};
