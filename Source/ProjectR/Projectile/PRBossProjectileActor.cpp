// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Boss 투사체 Actor 구현)
#include "PRBossProjectileActor.h"

void APRBossProjectileActor::HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ApplyEffectToTargetWithHit(OtherActor, Hit);
	Super::HandleHit_Implementation(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
