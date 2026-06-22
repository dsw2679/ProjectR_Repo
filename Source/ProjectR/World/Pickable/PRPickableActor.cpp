// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 획득 가능 Actor 및 관련 시스템 구현)
#include "PRPickableActor.h"

#include "Components/SphereComponent.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_PickUpAmmo.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PRRespawnSubsystem.h"

APRPickableActor::APRPickableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetSphereRadius(30.f);
	InteractionCollision->SetCollisionProfileName(FName("Interactable"));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(InteractionCollision);
}

void APRPickableActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bDisposable)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 액터 등록
				RespawnSubsystem->RegisterDisposableActor(this);
			}
		}
	}
}

void APRPickableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && bDisposable)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 액터 등록 해제
				RespawnSubsystem->UnregisterDisposableActor(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}
