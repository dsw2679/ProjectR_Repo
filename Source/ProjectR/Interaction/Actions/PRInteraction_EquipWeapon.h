// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (장비 Weapon 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_EquipWeapon.generated.h"

/**
 * 소유 액터의 WeaponData를 상호작용 액터에게 장착하는 동작.
 * Pawn의 Inventory에 새 무기 Item을 추가한 뒤 WeaponManager로 활성 슬롯에 장착.
 */
UCLASS()
class PROJECTR_API UPRInteraction_EquipWeapon : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	/*~ UPRInteractionAction Interface ~*/
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Execute_Implementation(AActor* Interactor) override;
};
