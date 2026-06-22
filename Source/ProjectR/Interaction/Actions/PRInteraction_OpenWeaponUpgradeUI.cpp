// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Open Weapon 강화/업그레이드 UI 상호작용 액션 실행 로직 구현)
#include "PRInteraction_OpenWeaponUpgradeUI.h"

#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"

UPRInteraction_OpenWeaponUpgradeUI::UPRInteraction_OpenWeaponUpgradeUI()
{
	InteractionType = EPRInteractionType::Instant;
	TriggerType = EPRTriggerType::Tap;
	bRequiresRange = true;
	bShowHint = true;
	ActionName = NSLOCTEXT("ProjectR", "OpenWeaponUpgradeActionName", "강화");
	ActionHintText = NSLOCTEXT("ProjectR", "OpenWeaponUpgradeActionHint", "무기 강화");
}

bool UPRInteraction_OpenWeaponUpgradeUI::CanInteract_Implementation(AActor* Interactor) const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(Interactor) && IsValid(OwnerActor) && IsValid(OwnerActor->FindComponentByClass<UPRWeaponUpgradeComponent>());
}

void UPRInteraction_OpenWeaponUpgradeUI::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UPRWeaponUpgradeComponent* UpgradeComponent = OwnerActor->FindComponentByClass<UPRWeaponUpgradeComponent>();
	if (!IsValid(UpgradeComponent))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetController(Interactor));
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientOpenWeaponUpgradeUI(UpgradeComponent);
}
