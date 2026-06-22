// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Boss 투사체 Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRProjectileBase.h"
#include "PRBossProjectileActor.generated.h"

class UPrimitiveComponent;

// 보스 패턴이 서버 권위로 직접 스폰하는 공용 투사체다.
// 기존 예측 투사체 파이프라인은 건드리지 않고, 보스 투사체 기본 Hit에서 GE 적용을 보장한다.
UCLASS(Blueprintable)
class PROJECTR_API APRBossProjectileActor : public APRProjectileBase
{
	GENERATED_BODY()

protected:
	// 권위 투사체가 대상에 적중했을 때 GE를 적용한 뒤 기본 제거 흐름을 실행한다.
	virtual void HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
