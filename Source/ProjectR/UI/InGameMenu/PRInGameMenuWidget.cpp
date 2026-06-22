// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (In Game Menu UI 위젯 구현)
#include "PRInGameMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"

UPRInGameMenuWidget::UPRInGameMenuWidget()
{
	Layer = EPRUILayer::System;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRInGameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonEvents();
	RefreshStatusText(FText::GetEmpty());
}

void UPRInGameMenuWidget::NativeDestruct()
{
	UnbindButtonEvents();

	Super::NativeDestruct();
}

void UPRInGameMenuWidget::BindButtonEvents()
{
	UnbindButtonEvents();

	if (IsValid(SaveButton))
	{
		SaveButton->OnClicked.AddDynamic(this, &ThisClass::HandleSaveButtonClicked);
	}

	if (IsValid(ExitToMenuButton))
	{
		ExitToMenuButton->OnClicked.AddDynamic(this, &ThisClass::HandleExitToMenuButtonClicked);
	}

	if (IsValid(OptionButton))
	{
		OptionButton->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}
}

void UPRInGameMenuWidget::UnbindButtonEvents()
{
	if (IsValid(SaveButton))
	{
		SaveButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSaveButtonClicked);
	}

	if (IsValid(ExitToMenuButton))
	{
		ExitToMenuButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleExitToMenuButtonClicked);
	}

	if (IsValid(OptionButton))
	{
		OptionButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}
}

bool UPRInGameMenuWidget::SaveCurrentCharacter()
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		RefreshStatusText(FText::FromString(TEXT("플레이어 컨트롤러 오류")));
		return false;
	}

	APRPlayerState* PlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		RefreshStatusText(FText::FromString(TEXT("플레이어 상태 오류")));
		return false;
	}

	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance))
	{
		RefreshStatusText(FText::FromString(TEXT("게임 인스턴스 오류")));
		return false;
	}

	if (!GameInstance->HasActiveLocalCharacterSlot())
	{
		RefreshStatusText(FText::FromString(TEXT("활성 세이브 슬롯 없음")));
		return false;
	}

	// PlayerState와 하위 컴포넌트의 현재 상태 스냅샷
	const FPRCharacterSaveData SaveData = PlayerState->MakeSaveData();
	GameInstance->SetLocalCharacterSave(SaveData);

	// 시작 메뉴에서 선택한 활성 슬롯 대상 저장
	if (!GameInstance->SaveActiveLocalCharacterSlot())
	{
		RefreshStatusText(FText::FromString(TEXT("저장 실패")));
		return false;
	}

	RefreshStatusText(FText::FromString(TEXT("저장 완료")));
	return true;
}

void UPRInGameMenuWidget::RefreshStatusText(const FText& StatusTextValue)
{
	if (IsValid(StatusText))
	{
		StatusText->SetText(StatusTextValue);
	}
}

UPRGameInstance* UPRInGameMenuWidget::GetProjectRGameInstance() const
{
	return Cast<UPRGameInstance>(GetGameInstance());
}

UPRUIControllerComponent* UPRInGameMenuWidget::GetUIControllerComponent() const
{
	const APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	return IsValid(PlayerController) ? PlayerController->GetUIController() : nullptr;
}

void UPRInGameMenuWidget::HandleSaveButtonClicked()
{
	SaveCurrentCharacter();
}

void UPRInGameMenuWidget::HandleExitToMenuButtonClicked()
{
	RefreshStatusText(FText::FromString(TEXT("메뉴로 이동 중")));

	if (UPRGameInstance* GameInstance = GetProjectRGameInstance())
	{
		GameInstance->LeaveSession();
		return;
	}

	RefreshStatusText(FText::FromString(TEXT("게임 인스턴스 오류")));
}

void UPRInGameMenuWidget::HandleOptionButtonClicked()
{
	if (UPRUIControllerComponent* UIControllerComponent = GetUIControllerComponent())
	{
		UIControllerComponent->OpenOption();
	}
}

void UPRInGameMenuWidget::HandleCloseButtonClicked()
{
	if (UPRUIControllerComponent* UIControllerComponent = GetUIControllerComponent())
	{
		UIControllerComponent->CloseInGameMenu();
	}
}
