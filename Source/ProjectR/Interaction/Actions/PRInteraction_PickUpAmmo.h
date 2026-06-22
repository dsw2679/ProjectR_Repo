// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (필드 탄약 획득 및 인벤토리 탄약 보관 구현)
// Author: 이건주 (Mod 버프 연동 탄약 보너스 획득 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_PickUpAmmo.generated.h"

/**
 * 픽업 액터의 탄약량을 상호작용한 액터의 슬롯 예비탄에 적립한다.
 * 캡 초과로 일부만 흡수되면 픽업이 잔여량을 유지한 채 월드에 남는다.
 */
UCLASS()
class PROJECTR_API UPRInteraction_PickUpAmmo : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(AActor* Interactor) override;
};
