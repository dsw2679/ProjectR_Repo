// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (어빌리티 타겟 데이터 구현)
#include "PRAbilityTargetData.h"

bool FPRTargetData_SpawnProjectile::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << ProjectileId;
	SpawnLocation.NetSerialize(Ar, Map, bOutSuccess);
	SpawnRotation.NetSerialize(Ar, Map, bOutSuccess);
	bOutSuccess = true;
	return true;
}
