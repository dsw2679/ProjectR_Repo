// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (회피 판정 및 데미지 트리거 연동)
// Author: 손승우 (근접 공격 데미지 판정(히트) 트리거 구현)
#include "PRAnimNotify_EnemyMeleeHit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

FString UPRAnimNotify_EnemyMeleeHit::GetNotifyName_Implementation() const
{
	return TEXT("PR Enemy Melee Hit");
}

void UPRAnimNotify_EnemyMeleeHit::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		// 데미지 판정은 서버의 Ability 인스턴스만 실행한다.
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = PRCombatGameplayTags::Event_Ability_EnemyMeleeHit;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	// 실제 타격 판정은 이 이벤트를 기다리던 EnemyMeleeAttack Ability 인스턴스가 처리한다.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		PRCombatGameplayTags::Event_Ability_EnemyMeleeHit,
		Payload);
}
