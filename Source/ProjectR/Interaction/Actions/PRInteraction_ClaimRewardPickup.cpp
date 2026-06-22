// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재화/아이템 획득 및 픽업 HUD 알림 연동 구현)
// Author: 배유찬 (보상 획득 상호작용 보정 구현)
#include "PRInteraction_ClaimRewardPickup.h"

#include "ProjectR/World/Drop/PRItemDropManagerSubsystem.h"
#include "ProjectR/World/Pickable/PRRewardPickupActor.h"

UPRInteraction_ClaimRewardPickup::UPRInteraction_ClaimRewardPickup()
{
	InteractionType = EPRInteractionType::Instant;
	TriggerType = EPRTriggerType::Tap;
	bRequiresRange = true;
	bShowHint = true;
	bOnlyShowHintNearScreenCenter = true;
	ActionName = NSLOCTEXT("ProjectR", "ClaimRewardPickupActionName", "획득");
	ActionHintText = NSLOCTEXT("ProjectR", "ClaimRewardPickupActionHint", "획득");
}

bool UPRInteraction_ClaimRewardPickup::CanInteract_Implementation(AActor* Interactor) const
{
	const APRRewardPickupActor* RewardPickup = Cast<APRRewardPickupActor>(GetOwner());
	return IsValid(RewardPickup) && RewardPickup->CanBeClaimedBy(Interactor);
}

void UPRInteraction_ClaimRewardPickup::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	APRRewardPickupActor* RewardPickup = Cast<APRRewardPickupActor>(GetOwner());
	if (!IsValid(RewardPickup) || !RewardPickup->HasAuthority())
	{
		return;
	}

	UWorld* World = RewardPickup->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPRItemDropManagerSubsystem* DropManager = World->GetSubsystem<UPRItemDropManagerSubsystem>();
	if (!IsValid(DropManager))
	{
		return;
	}

	DropManager->ClaimPickup(RewardPickup, Interactor);
}
