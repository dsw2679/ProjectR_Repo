// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (조준 방어 상태 연동 투사체 판정)
// Author: 손승우 (로열 아처 및 보스 원거리 투사체 스폰 트리거 구현)
// Author: 이건주 (Penitent 사격 시 투사체 발사 트리거 구현)
#include "PRAnimNotify_EnemySpawnProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

FString UPRAnimNotify_EnemySpawnProjectile::GetNotifyName_Implementation() const
{
	return TEXT("PR Enemy Spawn Projectile");
}

void UPRAnimNotify_EnemySpawnProjectile::Notify(USkeletalMeshComponent* MeshComp,
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
		// 투사체 스폰은 서버의 Ability 인스턴스만 실행
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = PRCombatGameplayTags::Event_Ability_EnemyProjectileFire;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	// 실제 투사체 스폰은 이 이벤트를 기다리던 EnemyProjectileAttack Ability 인스턴스가 처리
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		PRCombatGameplayTags::Event_Ability_EnemyProjectileFire,
		Payload);
}
