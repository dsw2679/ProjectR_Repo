// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (조준/줌 기능용 카메라 시야각(FOV) 및 거리 제어 구현)
// Author: 배유찬 (사격 카메라 흔들림 및 상호작용 카메라 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "PRCameraManager.generated.h"

/** 
 * 페이드 인-아웃 컬러 프리셋
 */
UENUM(BlueprintType)
enum class EPRFadeColorPreset : uint8
{
	None,
	Black,
	White,
};

/**
 * FOV 트랜지션과 숄더스왑 로직의 중앙 제어소 역할을 합니다.
 */
UCLASS()
class PROJECTR_API APRCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	APRCameraManager();

	/*~ AActor Interface ~*/
	// 로컬 카메라 피드백 모디파이어를 등록한다
	virtual void BeginPlay() override;

	// 모디파이어(구르기 등)가 외부에서 덮어씌울 목표 FOV (0이면 사용 안 함)
	float ModifierTargetFOV = 0.0f;

	// 모디파이어 FOV 반영 비율
	float ModifierFOVAlpha = 0.0f;
	
	// 어빌리티가 조준 중일 때 덮어씌울 목표 FOV (0.0f이면 조준 안 함)
	float OverrideAimFOV = 0.0f;

	static FLinearColor GetFadeColor(EPRFadeColorPreset InPresetColor);
	
	UFUNCTION(BlueprintCallable)
	void FadeOut(EPRFadeColorPreset ColorPreset, float InDuration, bool bShouldFadeAudio = false);
	
	UFUNCTION(BlueprintCallable)
	void FadeIn(EPRFadeColorPreset ColorPreset, float InDuration, bool bShouldFadeAudio = false);
	
protected:
	/*~ APlayerCameraManager Interface ~*/
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

private:
	// 보간을 위한 내부 변수
	float TargetFOV = 80.0f;
	
	// 보간을 위해 현재 FOV 값을 스스로 기억하는 변수
	float CurrentFOV = 80.0f;
};
