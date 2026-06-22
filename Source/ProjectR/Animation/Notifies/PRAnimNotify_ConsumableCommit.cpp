// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 복용 중 행동 차단 상태 해제 연동)
// Author: 손승우 (적 AI 소모품 효과 연동)
// Author: 이건주 (소모품 사용 애니메이션 완료 시점 회복 효과 트리거 구현)
#include "PRAnimNotify_ConsumableCommit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/PRGameplayTags.h"

FString UPRAnimNotify_ConsumableCommit::GetNotifyName_Implementation() const
{
	return TEXT("PR Consumable Commit");
}

void UPRAnimNotify_ConsumableCommit::Notify(USkeletalMeshComponent* MeshComp,
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
		// 실제 효과 적용과 스택 소모는 서버 Ability 인스턴스만 처리한다.
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = PRGameplayTags::Event_Player_ConsumableCommit;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		PRGameplayTags::Event_Player_ConsumableCommit,
		Payload);
}
