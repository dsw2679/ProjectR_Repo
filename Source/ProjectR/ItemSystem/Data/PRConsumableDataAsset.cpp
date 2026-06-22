// Author: 김동석 (복용 연출 연동 캐릭터 외형 데이터 속성 정의)
// Author: 배유찬 (소모품 복용 애니메이션 및 회복량 데이터 정의)
// Author: 이건주 (퀵슬롯 아이콘 및 HUD 연동 리소스 정의)
#include "PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"

UPRConsumableDataAsset::UPRConsumableDataAsset()
{
	ItemType = EPRItemType::Consumable;
	ItemInstanceClass = UPRItemInstance_Consumable::StaticClass();
	MaxStackCount = 99;
}
