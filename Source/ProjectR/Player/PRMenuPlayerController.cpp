// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu Player Controller 구현)
#include "PRMenuPlayerController.h"

#include "Components/Widget.h"

void APRMenuPlayerController::ApplyMenuInputMode(UWidget* FocusWidget)
{
	if (!IsLocalController())
	{
		return;
	}

	// 메뉴 전용 UI 입력 모드 적용
	FInputModeUIOnly InputMode;
	if (IsValid(FocusWidget))
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	// 메뉴 입력 중 게임 뷰포트 밖 마우스 이동 방지
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);

	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void APRMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	ApplyMenuInputMode(nullptr);
}
