// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu HUD 구현)
#include "PRMenuHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Player/PRMenuPlayerController.h"
#include "ProjectR/UI/StartMenu/PRMainTitleWidget.h"
#include "ProjectR/UI/StartMenu/PRStartMenuWidget.h"

void APRMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(PlayerOwner) || !PlayerOwner->IsLocalController())
	{
		return;
	}

	ShowMainTitle();
}

void APRMenuHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(MainTitleWidget))
	{
		MainTitleWidget->GetOnContinueRequested().RemoveAll(this);
		MainTitleWidget->RemoveFromParent();
		MainTitleWidget = nullptr;
	}

	if (IsValid(StartMenuWidget))
	{
		StartMenuWidget->RemoveFromParent();
		StartMenuWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APRMenuHUD::ShowMainTitle()
{
	if (!IsValid(MainTitleWidgetClass.Get()))
	{
		ShowStartMenu();
		return;
	}

	MainTitleWidget = CreateWidget<UPRMainTitleWidget>(PlayerOwner, MainTitleWidgetClass);
	if (!IsValid(MainTitleWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuHUD] Main title widget create failed"));
		ShowStartMenu();
		return;
	}

	MainTitleWidget->GetOnContinueRequested().AddUObject(this, &ThisClass::HandleMainTitleContinueRequested);
	MainTitleWidget->AddToViewport();

	if (APRMenuPlayerController* MenuPlayerController = Cast<APRMenuPlayerController>(PlayerOwner))
	{
		MenuPlayerController->ApplyMenuInputMode(MainTitleWidget);
	}
}

void APRMenuHUD::ShowStartMenu()
{
	if (IsValid(MainTitleWidget))
	{
		MainTitleWidget->GetOnContinueRequested().RemoveAll(this);
		MainTitleWidget->RemoveFromParent();
		MainTitleWidget = nullptr;
	}

	if (!IsValid(StartMenuWidgetClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuHUD] Start menu widget class is not set"));
		return;
	}

	if (!IsValid(StartMenuWidget))
	{
		StartMenuWidget = CreateWidget<UPRStartMenuWidget>(PlayerOwner, StartMenuWidgetClass);
		if (!IsValid(StartMenuWidget))
		{
			UE_LOG(LogTemp, Warning, TEXT("[MenuHUD] Start menu widget create failed"));
			return;
		}
	}

	if (!StartMenuWidget->IsInViewport())
	{
		StartMenuWidget->AddToViewport();
	}

	if (APRMenuPlayerController* MenuPlayerController = Cast<APRMenuPlayerController>(PlayerOwner))
	{
		MenuPlayerController->ApplyMenuInputMode(StartMenuWidget);
	}
}

void APRMenuHUD::HandleMainTitleContinueRequested()
{
	ShowStartMenu();
}
