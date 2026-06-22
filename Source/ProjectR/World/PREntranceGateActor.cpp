// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Entrance 보스 입장 게이트 Actor 및 관련 시스템 구현)
#include "PREntranceGateActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_EntranceGate.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"

APREntranceGateActor::APREntranceGateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(SceneRoot);
	InteractionCollision->SetBoxExtent(FVector(100.0f, 100.0f, 120.0f));
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable"));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 입장 게이트 맵 이동 액션 등록
	InteractableComponent->InteractionActions.Add(CreateDefaultSubobject<UPRInteraction_EntranceGate>(TEXT("EntranceGateAction")));
}
