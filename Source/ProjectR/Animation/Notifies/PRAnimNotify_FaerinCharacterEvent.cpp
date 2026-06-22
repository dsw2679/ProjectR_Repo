// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 페어린 보스 Character Event 윈도우/트리거 노티파이 구현)
#include "PRAnimNotify_FaerinCharacterEvent.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"

FString UPRAnimNotify_FaerinCharacterEvent::GetNotifyName_Implementation() const
{
	return EventName == NAME_None
		? TEXT("PR Faerin Character Event")
		: FString::Printf(TEXT("PR Faerin Event: %s"), *EventName.ToString());
}

void UPRAnimNotify_FaerinCharacterEvent::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp) || EventName == NAME_None)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRFaerinCharacterEventRouterComponent* RouterComponent =
		OwnerActor->FindComponentByClass<UPRFaerinCharacterEventRouterComponent>();
	if (!IsValid(RouterComponent))
	{
		return;
	}

	RouterComponent->RouteFaerinCharacterEvent(EventName);
}
