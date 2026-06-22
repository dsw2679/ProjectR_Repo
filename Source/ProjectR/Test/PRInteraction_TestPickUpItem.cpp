// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction Test Pick Up Item 구현)
#include "PRInteraction_TestPickUpItem.h"

#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRInteraction_TestPickUpItem::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);
	
	if (!IsValid(Interactor))
	{
		return;
	}
	
	if (!IsValid(ItemData) || Amount <= 0)
	{
		return;
	}
	
	UPRInventoryComponent* InventoryComponent = UPRGameplayStatics::GetInventoryComponent(Interactor);
	if (!IsValid(InventoryComponent))
	{
		return;
	}
	
	if (InventoryComponent->AddItem(ItemData, Amount))
	{
		if (bDestroyOnPickUp)
		{
			GetOwner()->Destroy();
		}
	}
}
