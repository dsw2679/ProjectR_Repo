// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (조준/줌 기능용 카메라 시야각(FOV) 및 거리 제어 구현)
// Author: 배유찬 (사격 카메라 흔들림 및 상호작용 카메라 연동 구현)
#include "ProjectR/Game/PRCameraManager.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier_Recoil.h"

APRCameraManager::APRCameraManager()
{
	// 기본 카메라 설정
	DefaultFOV = 80.0f;
	ViewPitchMin = -70.0f;
	ViewPitchMax = 70.0f;
	bAlwaysApplyModifiers = true;
}

void APRCameraManager::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningController = GetOwningPlayerController();
	if (IsValid(OwningController) && OwningController->IsLocalController())
	{
		AddNewCameraModifier(UPRCameraModifier_Recoil::StaticClass());
	}
}

FLinearColor APRCameraManager::GetFadeColor(EPRFadeColorPreset InPresetColor)
{
	switch (InPresetColor)
	{
	case EPRFadeColorPreset::Black:
		return FLinearColor::Black;
	case EPRFadeColorPreset::White:
		return FLinearColor::White;
	}
	return FLinearColor::Black;
}

void APRCameraManager::FadeOut(EPRFadeColorPreset ColorPreset, float InDuration, bool bShouldFadeAudio)
{
	FLinearColor TargetColor = GetFadeColor(ColorPreset);
	StartCameraFade(0.f, 1.f, InDuration, TargetColor, bShouldFadeAudio, /* bHoldWhenFinished */true);
}

void APRCameraManager::FadeIn(EPRFadeColorPreset ColorPreset, float InDuration, bool bShouldFadeAudio)
{
	FLinearColor TargetColor = GetFadeColor(ColorPreset);
	StartCameraFade(1.f, 0.f, InDuration, TargetColor, bShouldFadeAudio, false);
}

void APRCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	
	// 현재 카메라가 바라보고 있는 타겟이 PRPlayerCharacter인지 확인합니다.
	APRPlayerCharacter* PRCharacter = Cast<APRPlayerCharacter>(OutVT.Target);
	if (IsValid(PRCharacter))
	{
		// 캐릭터의 상태에 따라 TargetFOV를 결정합니다.
		TargetFOV = DefaultFOV;
		
		// 캐릭터가 실제로 이동 중인지 확인
		bool bIsMoving = PRCharacter->GetVelocity().Size2D() > 10.0f;
		
		if (OverrideAimFOV > 0.0f)
		{
			TargetFOV = OverrideAimFOV;
		}
		// 질주 상태 & 이동 중일 때
		else if (PRCharacter->IsSprinting() && bIsMoving)
		{
			TargetFOV = 90.0f; // 역동감을 위해 시야각을 넓힘
		}
		else if (PRCharacter->IsWalking())
		{
			TargetFOV = 60.0f; // 역동감을 위해 시야각을 넓힘
		}
		
		if (ModifierTargetFOV > 0.0f)
		{
			TargetFOV = FMath::Lerp(TargetFOV, ModifierTargetFOV, FMath::Clamp(ModifierFOVAlpha, 0.0f, 1.0f));
		}

		// 3. 현재 FOV에서 목표 FOV로 부드럽게 보간합니다 (InterpSpeed: 10.0f).
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 10.0f);
		OutVT.POV.FOV = CurrentFOV;
		
		// 사용이 끝난 모디파이어 값은 매 프레임 초기화 (모디파이어가 비활성화되면 0이 됨)
		ModifierTargetFOV = 0.0f;
		ModifierFOVAlpha = 0.0f;
	}
}
