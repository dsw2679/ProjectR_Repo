// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재화/아이템 획득 및 픽업 HUD 알림 연동 구현)
// Author: 배유찬 (보상 획득 상호작용 보정 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_ClaimRewardPickup.generated.h"

// 보상 픽업 입력을 드롭 매니저로 전달하는 즉발 상호작용이다
UCLASS()
class PROJECTR_API UPRInteraction_ClaimRewardPickup : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_ClaimRewardPickup();

	/*~ UPRInteractionAction Interface ~*/
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Execute_Implementation(AActor* Interactor) override;
};
