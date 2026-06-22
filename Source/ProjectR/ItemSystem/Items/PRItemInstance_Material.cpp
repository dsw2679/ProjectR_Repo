// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Item 인스턴스 Material 구현)
#include "PRItemInstance_Material.h"

#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"

UPRMaterialDataAsset* UPRItemInstance_Material::GetMaterialData() const
{
	return Cast<UPRMaterialDataAsset>(ItemData);
}
