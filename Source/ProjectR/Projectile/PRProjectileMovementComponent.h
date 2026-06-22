// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (반사 궤적 예측 시뮬레이션 및 비행 경로 동기화 연산 구현)
// Author: 이건주 (서포트 드론 유도 비행 궤적 연산 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PRProjectileMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRProjectileMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	UPRProjectileMovementComponent();
	
	virtual bool ShouldBounce(const FHitResult& Hit);

protected:
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;

public:
	UFUNCTION(BlueprintPure)
	bool GetShouldBounce() const {return bShouldBounce;}
	
protected:
	// 최대 바운스 허용 횟수. 0이면 바운스 불가
	UPROPERTY(EditDefaultsOnly, Category=ProjectileBounces, meta = (ClampMin = "0"))
	int32 MaxBounceCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	TArray<TEnumAsByte<ECollisionChannel>> BounceChannels;
	
private:
	// 현재까지 바운스 된 횟수
	int32 CurrentBounceCount = 0;
};
