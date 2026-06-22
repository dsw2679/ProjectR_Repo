// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Flashlight 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/SpotLightComponent.h"
#include "PRFlashlightComponent.generated.h"

/**
 * 플레이어 조준 방향 기반 플래시라이트
 */
UCLASS(ClassGroup=(Lights), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFlashlightComponent : public USpotLightComponent
{
	GENERATED_BODY()

public:
	UPRFlashlightComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 로컬 플레이어 플래시라이트 활성화 처리 */
	void SetFlashlightEnabled(bool bNewEnabled);

	/** 플래시라이트 활성화 여부 반환 */
	bool IsFlashlightEnabled() const { return bFlashlightEnabled; }

protected:
	/** AimOffset 기준 Yaw 하한값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Flashlight|Aim")
	float MinAimYaw = -90.0f;

	/** AimOffset 기준 Yaw 상한값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Flashlight|Aim")
	float MaxAimYaw = 90.0f;

	/** AimOffset 기준 Pitch 하한값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Flashlight|Aim")
	float MinAimPitch = -60.0f;

	/** AimOffset 기준 Pitch 상한값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Flashlight|Aim")
	float MaxAimPitch = 60.0f;

	/** 방향 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Flashlight|Aim")
	float RotationInterpSpeed = 18.0f;

private:
	/** 조준 방향 반영 */
	void UpdateFlashlightRotation(float DeltaTime);

private:
	bool bFlashlightEnabled = false;
};
