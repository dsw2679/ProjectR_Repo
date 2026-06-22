// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State Enemy 근접 Hit 윈도우 윈도우/트리거 노티파이 구현)
#include "PRAnimNotifyState_EnemyMeleeHitWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

namespace
{
	void SendEnemyMeleeWindowEvent(USkeletalMeshComponent* MeshComp, const FGameplayTag EventTag)
	{
		if (!IsValid(MeshComp))
		{
			return;
		}

		AActor* OwnerActor = MeshComp->GetOwner();
		if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
		{
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = OwnerActor;
		Payload.Target = OwnerActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			OwnerActor,
			EventTag,
			Payload);
	}
}

FString UPRAnimNotifyState_EnemyMeleeHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("PR Enemy Melee Hit Window");
}

void UPRAnimNotifyState_EnemyMeleeHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SendEnemyMeleeWindowEvent(MeshComp, PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin);
}

void UPRAnimNotifyState_EnemyMeleeHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	SendEnemyMeleeWindowEvent(MeshComp, PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick);
}

void UPRAnimNotifyState_EnemyMeleeHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SendEnemyMeleeWindowEvent(MeshComp, PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd);
}
