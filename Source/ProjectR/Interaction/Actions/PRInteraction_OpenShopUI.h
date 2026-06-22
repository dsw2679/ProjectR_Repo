// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_OpenShopUI.generated.h"

// 상점 NPC 또는 터미널 상호작용으로 상점 UI를 여는 Action
UCLASS()
class PROJECTR_API UPRInteraction_OpenShopUI : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	// 상점 UI 상호작용 기본 표시값을 초기화
	UPRInteraction_OpenShopUI();

public:
	// 상점 컴포넌트가 있는 대상인지 확인
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// 요청 플레이어에게 상점 UI 오픈을 지시
	virtual void Execute_Implementation(AActor* Interactor) override;
};
