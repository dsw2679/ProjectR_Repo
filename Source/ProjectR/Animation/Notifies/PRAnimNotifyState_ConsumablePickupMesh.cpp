// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 State 소모품 픽업 Mesh 윈도우/트리거 노티파이 구현)
#include "PRAnimNotifyState_ConsumablePickupMesh.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"

/*~ UAnimNotifyState Interface ~*/

FString UPRAnimNotifyState_ConsumablePickupMesh::GetNotifyName_Implementation() const
{
	return TEXT("PR Consumable Pickup Mesh");
}

void UPRAnimNotifyState_ConsumablePickupMesh::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	if (!IsValid(ConsumableData) || !IsValid(ConsumableData->GetPickupMesh()))
	{
		UE_LOG(LogTemp,Warning,TEXT("!!!!!!!!!! Pickup Mesh Data is Invalid"));
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(MeshComp->GetOwner());
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	PlayerCharacter->RequestConsumablePickupMeshBegin(ConsumableData, AttachSocketName);
}

void UPRAnimNotifyState_ConsumablePickupMesh::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(MeshComp->GetOwner());
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	PlayerCharacter->RequestConsumablePickupMeshEnd();
}
