// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 손승우 (로열 아처 전용 비행 화살 투사체 및 충돌 처리 구현)
// Author: 이건주 (Penitent 몬스터용 배리어 유도 탄환 투사체 구현)
#include "PREnemyProjectileActor.h"

#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"

APREnemyProjectileActor::APREnemyProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APREnemyProjectileActor::HandleHit_Implementation(UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	Super::HandleHit_Implementation(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	ApplyEffectToTargetWithHit(OtherActor, Hit);
}

bool APREnemyProjectileActor::ShouldIgnoreProjectileHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit) const
{
	if (Super::ShouldIgnoreProjectileHit(OtherActor, OtherComp, Hit))
	{
		return true;
	}

	return IsValid(Cast<APREnemyBaseCharacter>(OtherActor));
}
