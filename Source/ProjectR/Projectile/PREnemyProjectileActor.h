// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 손승우 (로열 아처 전용 비행 화살 투사체 및 충돌 처리 구현)
// Author: 이건주 (Penitent 몬스터용 배리어 유도 탄환 투사체 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRProjectileBase.h"
#include "PREnemyProjectileActor.generated.h"

class UPrimitiveComponent;

UCLASS()
class PROJECTR_API APREnemyProjectileActor : public APRProjectileBase
{
	GENERATED_BODY()

public:
	// 일반 적 투사체 기본값을 초기화한다.
	APREnemyProjectileActor();

protected:
	/*~ APRProjectileBase Interface ~*/
	virtual void HandleHit_Implementation(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;

	// 일반 적 투사체가 같은 적 계열 Actor를 실제 명중 대상으로 처리하지 않게 한다.
	virtual bool ShouldIgnoreProjectileHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit) const override;
};
