// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_FaerinEncounterChoice.h"

#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Character/PRPlayerCharacter.h"

UPRInteraction_FaerinEncounterChoice::UPRInteraction_FaerinEncounterChoice()
{
	InteractionType = EPRInteractionType::Instant;
	TriggerType = EPRTriggerType::Tap;
	bRequiresRange = true;
	bShowHint = true;
	ActionName = NSLOCTEXT("ProjectR", "FaerinEncounterChoiceActionName", "Faerin");
	ActionHintText = NSLOCTEXT("ProjectR", "FaerinEncounterChoiceActionHint", "대화하기");
}

bool UPRInteraction_FaerinEncounterChoice::CanInteract_Implementation(AActor* Interactor) const
{
	APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(GetPawn(Interactor));
	if (!IsValid(Player) || !IsValid(EncounterDirector))
	{
		return false;
	}

	if (EncounterDirector->HasAuthority())
	{
		return EncounterDirector->CanOpenChoiceDialogue(Player);
	}

	return EncounterDirector->CanShowChoiceDialoguePrompt(Player);
}

void UPRInteraction_FaerinEncounterChoice::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	if (!IsValid(EncounterDirector) || !EncounterDirector->HasAuthority())
	{
		return;
	}

	APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(GetPawn(Interactor));
	if (!IsValid(Player))
	{
		return;
	}

	EncounterDirector->StartChoiceDialogue(Player);
}
