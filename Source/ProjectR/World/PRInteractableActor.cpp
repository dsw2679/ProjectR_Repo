// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Interactable Actor 및 관련 시스템 구현)
#include "PRInteractableActor.h"

#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/System/PRDeveloperSettings.h"


// Sets default values
APRInteractableActor::APRInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractableComponent = CreateDefaultSubobject<UPRInteractableComponent>(TEXT("InteractableComponent"));
	bReplicates = true;
}


FPRWorldMarkerVisualData APRInteractableActor::GetPingMarkerVisualData_Implementation() const
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

FVector APRInteractableActor::GetPingMarkerWorldLocation_Implementation() const
{
	return GetActorLocation() + PingMarkerOffset;
}
