// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_OpenShopUI.h"

#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"

UPRInteraction_OpenShopUI::UPRInteraction_OpenShopUI()
{
	InteractionType = EPRInteractionType::Instant;
	TriggerType = EPRTriggerType::Tap;
	bRequiresRange = true;
	bShowHint = true;
	ActionName = NSLOCTEXT("ProjectR", "OpenShopActionName", "상점");
	ActionHintText = NSLOCTEXT("ProjectR", "OpenShopActionHint", "상점 열기");
}

bool UPRInteraction_OpenShopUI::CanInteract_Implementation(AActor* Interactor) const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(Interactor) && IsValid(OwnerActor) && IsValid(OwnerActor->FindComponentByClass<UPRShopComponent>());
}

void UPRInteraction_OpenShopUI::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UPRShopComponent* ShopComponent = OwnerActor->FindComponentByClass<UPRShopComponent>();
	if (!IsValid(ShopComponent))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetController(Interactor));
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientOpenShopUI(ShopComponent);
}
