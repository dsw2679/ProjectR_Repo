// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRNPCBase.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/System/PRDeveloperSettings.h"

APRNPCBase::APRNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	NPCInteractable = CreateDefaultSubobject<UPRInteractableComponent>(TEXT("NPCInteractable"));
	bReplicates = true;
}

UPRInteractableComponent* APRNPCBase::GetInteractableComponent() const
{
	return NPCInteractable;
}

FPRWorldMarkerVisualData APRNPCBase::GetPingMarkerVisualData_Implementation() const
{
	if (bOverridesPingMarker)
	{
		return PingMarkerVisualOverride;
	}
	
	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetWorldMarkerPreset(EPRWorldMarkerPreset::Interactable);
	}
	
	return FPRWorldMarkerVisualData();
}

FVector APRNPCBase::GetPingMarkerWorldLocation_Implementation() const
{
	return GetActorLocation() + PingMarkerOffset;
}

