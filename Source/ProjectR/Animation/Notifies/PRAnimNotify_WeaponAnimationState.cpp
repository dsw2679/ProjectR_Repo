// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (애니메이션 Weapon Animation State 윈도우/트리거 노티파이 구현)
#include "PRAnimNotify_WeaponAnimationState.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"

FString UPRAnimNotify_WeaponAnimationState::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("PR Weapon Anim: %s"), *UEnum::GetValueAsString(AnimationState));
}

void UPRAnimNotify_WeaponAnimationState::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(MeshComp->GetOwner());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return;
	}

	// 각 머신에서 로컬 활성 슬롯 무기 메시 애니메이션 상태를 갱신한다
	WeaponManager->RequestWeaponAnimation(AnimationState);
}
