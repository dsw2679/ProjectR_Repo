// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재료 아이템 상점 가격 및 아이콘 리소스 정의)
// Author: 배유찬 (강화 재료 분류 및 기본 속성 데이터 정의)
#include "PRMaterialDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"

UPRMaterialDataAsset::UPRMaterialDataAsset()
{
	ItemType = EPRItemType::Material;
	ItemInstanceClass = UPRItemInstance_Material::StaticClass();
	MaxStackCount = 999;
}
