// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Camera Modifier 구현)
#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "PRCameraModifier.generated.h"

/**
 * 구르기(Roll)나 피격과 같은 어빌리티 기반의 액션중일때
 * 임시로 카메라의 Z축 높이를 낮추거나 시야각(FOV)을 왜곡하는 이펙트를 더합니다.
 */
UCLASS()
class PROJECTR_API UPRCameraModifier : public UCameraModifier
{
	GENERATED_BODY()
	
public:
	UPRCameraModifier();

	/*~ UCameraModifier Interface ~*/
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;

	/** 외부(어빌리티 등)에서 목표 FOV와 오프셋을 세팅할 수 있게 합니다. */
	void SetActionCameraSettings(float InTargetFOV, FVector InLocationOffset);

protected:
	// 임시로 왜곡할 목표 시야각
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float TargetFOV = 110.0f;

	// 임시로 이동시킬 카메라 위치 (예: Z축을 -30만큼 낮춰 역동감 부여)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	FVector LocationOffset = FVector(0.0f, 0.0f, -30.0f); 
};
