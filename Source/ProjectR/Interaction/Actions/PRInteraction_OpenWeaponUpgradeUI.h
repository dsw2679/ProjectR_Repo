// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Open Weapon 강화/업그레이드 UI 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_OpenWeaponUpgradeUI.generated.h"

// 강화 NPC 또는 터미널 상호작용으로 강화 UI를 여는 Action이다
UCLASS()
class PROJECTR_API UPRInteraction_OpenWeaponUpgradeUI : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	// 강화 UI 상호작용 기본 표시값을 초기화한다
	UPRInteraction_OpenWeaponUpgradeUI();

public:
	// 강화 컴포넌트가 있는 대상인지 확인한다
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// 요청 플레이어에게 강화 UI 오픈을 지시한다
	virtual void Execute_Implementation(AActor* Interactor) override;
};
