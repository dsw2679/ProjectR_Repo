// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponStatStatics.h"

#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

float UPRWeaponStatStatics::CalculateUpgradedBaseDamage(const UPRWeaponDataAsset* WeaponData, int32 UpgradeLevel)
{
	if (!IsValid(WeaponData))
	{
		return 0.0f;
	}

	const int32 ClampedUpgradeLevel = FMath::Max(UpgradeLevel, 0);
	return FMath::Max(WeaponData->BaseDamage * (1.0f + static_cast<float>(ClampedUpgradeLevel) * 0.1f), 0.0f);
}

float UPRWeaponStatStatics::CalculateWeaponItemBaseDamage(const UPRItemInstance_Weapon* WeaponItem)
{
	const UPRWeaponDataAsset* WeaponData = IsValid(WeaponItem) ? WeaponItem->GetWeaponData() : nullptr;
	const int32 UpgradeLevel = IsValid(WeaponItem) ? WeaponItem->GetUpgradeLevel() : 0;
	return CalculateUpgradedBaseDamage(WeaponData, UpgradeLevel);
}
