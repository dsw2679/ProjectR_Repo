// Author: 김동석 (월드 픽업 스폰용 드롭 메시 데이터 정의)
// Author: 배유찬 (아이템 기본 속성(이름/설명/등급) 데이터 필드 정의)
// Author: 이건주 (인벤토리 3D 프리뷰용 모델 데이터 필드 정의)
#include "PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"

UPRItemDataAsset::UPRItemDataAsset()
{
	ItemType = EPRItemType::None;
	ItemInstanceClass = UPRItemInstance::StaticClass();
}