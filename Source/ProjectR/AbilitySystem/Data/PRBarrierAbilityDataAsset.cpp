// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Barrier 어빌리티 데이터 에셋 구현)
#include "PRBarrierAbilityDataAsset.h"

#include "ProjectR/Projectile/PRBarrierAnchorActor.h"

UPRBarrierAbilityDataAsset::UPRBarrierAbilityDataAsset()
{
	// 기본 앵커 클래스
	BarrierAnchorActorClass = APRBarrierAnchorActor::StaticClass();
}
