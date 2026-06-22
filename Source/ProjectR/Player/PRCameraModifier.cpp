// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Camera Modifier 구현)
#include "ProjectR/Player/PRCameraModifier.h"

#include "ProjectR/Game/PRCameraManager.h"

UPRCameraModifier::UPRCameraModifier()
{
	// 언리얼 모디파이어의 내장 기능: 부드럽게 들어가고(In) 부드럽게 빠짐(Out)
	AlphaInTime = 0.1f;  // 효과가 완전히 적용되기까지 0.1초
	AlphaOutTime = 0.2f; // 효과가 사라지며 원상 복구되기까지 0.2초
}

void UPRCameraModifier::SetActionCameraSettings(float InTargetFOV, FVector InLocationOffset)
{
	TargetFOV = InTargetFOV;
	LocationOffset = InLocationOffset;
}

bool UPRCameraModifier::ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	// 1. 나를 소유한 매니저(PRCameraManager)를 찾아서 목표 FOV값 전달
	if (APRCameraManager* PRCamera = Cast<APRCameraManager>(CameraOwner))
	{
		PRCamera->ModifierTargetFOV = TargetFOV;
		PRCamera->ModifierFOVAlpha = Alpha;
	}

	// 2. 위치(Z축 하강 등)는 모디파이어가 계속 직접 처리
	FVector LocalOffset = InOutPOV.Rotation.RotateVector(LocationOffset);
	InOutPOV.Location += LocalOffset * Alpha;

	// 다른 모디파이어(예: 반동, 쉐이크)도 이어서 연산할 수 있도록 false 반환
	return false;
}
