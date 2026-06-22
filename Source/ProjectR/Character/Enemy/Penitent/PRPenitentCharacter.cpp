// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (Penitent 몬스터 Character 구현)
#include "PRPenitentCharacter.h"

#include "MotionWarpingComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Projectile/PRBarrierAnchorActor.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AI/Components/PRSoldierArmoredDebugDrawComponent.h"

APRPenitentCharacter::APRPenitentCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 일치
	CharacterID = TEXT("Penitent");
	
	DebugDrawComponent = CreateDefaultSubobject<UPRSoldierArmoredDebugDrawComponent>(TEXT("DebugDrawComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

void APRPenitentCharacter::HandleDeadTagChanged(bool bEntered)
{
	Super::HandleDeadTagChanged(bEntered);

	// 사망 진입 검사
	if (!bEntered || !HasAuthority())
	{
		return;
	}

	UPRAbilitySystemComponent* PenitentAbilitySystemComponent = GetEnemyAbilitySystemComponent();
	if (!IsValid(PenitentAbilitySystemComponent)
		|| !PenitentAbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		return;
	}

	CleanupSummonedBarrier(true);
}

void APRPenitentCharacter::SetSpawnedBarrierActor(APRGroundBoxProjectileBase* InBarrierActor, APRBarrierAnchorActor* InAnchorActor)
{
	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}

	if (HasAuthority() && IsValid(SpawnedBarrierAnchorActor) && SpawnedBarrierAnchorActor != InAnchorActor)
	{
		// 기존 앵커 제거
		SpawnedBarrierAnchorActor->Destroy();
	}

	SpawnedBarrierActor = InBarrierActor;
	SpawnedBarrierAnchorActor = InAnchorActor;

	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.AddUniqueDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}
}

void APRPenitentCharacter::ClearSpawnedBarrierActor(APRGroundBoxProjectileBase* BarrierActorToClear)
{
	if (IsValid(BarrierActorToClear) && SpawnedBarrierActor != BarrierActorToClear)
	{
		return;
	}

	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}

	SpawnedBarrierActor = nullptr;

	if (HasAuthority() && IsValid(SpawnedBarrierAnchorActor))
	{
		// 앵커 제거
		SpawnedBarrierAnchorActor->Destroy();
	}

	SpawnedBarrierAnchorActor = nullptr;
}

void APRPenitentCharacter::CleanupSummonedBarrier(bool bRequestBarrierEnd)
{
	APRGroundBoxProjectileBase* BarrierActor = SpawnedBarrierActor.Get();

	// 사망 소멸 요청
	if (bRequestBarrierEnd && HasAuthority() && IsValid(BarrierActor))
	{
		BarrierActor->RequestGroundBoxEnd();
	}

	ClearSpawnedBarrierActor(BarrierActor);

	UPRAbilitySystemComponent* PenitentAbilitySystemComponent = GetEnemyAbilitySystemComponent();
	if (!IsValid(PenitentAbilitySystemComponent))
	{
		return;
	}

	// 배리어 보유 상태 태그 정리
	PenitentAbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	PenitentAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

bool APRPenitentCharacter::HasActiveBarrier() const
{
	return IsValid(SpawnedBarrierActor);
}

void APRPenitentCharacter::HandleSpawnedBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier)
{
	if (SpawnedBarrierActor != DestroyedBarrier)
	{
		return;
	}

	CleanupSummonedBarrier(false);
}
