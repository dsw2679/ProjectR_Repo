// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (콤보 유효 시간대 조준 판정 연동)
// Author: 손승우 (해머 및 보스 콤보 시퀀스 연계 윈도우 트리거 구현)
#include "PRAnimNotify_EnemyComboWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

FString UPRAnimNotify_EnemyComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("PR Enemy Combo Window");
}

void UPRAnimNotify_EnemyComboWindow::Notify(USkeletalMeshComponent* MeshComp,
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
		// 콤보 분기 결정은 서버 Ability 전용
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = PRCombatGameplayTags::Event_Ability_EnemyComboWindow;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		PRCombatGameplayTags::Event_Ability_EnemyComboWindow,
		Payload);
}
