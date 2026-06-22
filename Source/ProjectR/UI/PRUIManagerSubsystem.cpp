// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 매니저 서브시스템 구현)
#include "PRUIManagerSubsystem.h"

#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// 스택 관리 위젯이 뷰포트 또는 부모 패널에 붙어 있는지 확인
	bool IsManagedWidgetAttached(const UPRWidgetBase* Widget)
	{
		if (!IsValid(Widget))
		{
			return false;
		}

		if (Widget->IsInViewport())
		{
			return true;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return IsValid(Widget->GetParent())
			&& Visibility != ESlateVisibility::Collapsed
			&& Visibility != ESlateVisibility::Hidden;
	}

	// 부모 패널에 배치된 위젯인지 확인
	bool IsEmbeddedWidget(const UPRWidgetBase* Widget)
	{
		return IsValid(Widget) && IsValid(Widget->GetParent()) && !Widget->IsInViewport();
	}

	// 스택 Push 대상 위젯을 표시
	void ShowManagedWidget(UPRWidgetBase* Widget, int32 ZOrder)
	{
		if (!IsValid(Widget))
		{
			return;
		}

		if (IsEmbeddedWidget(Widget))
		{
			Widget->SetVisibility(ESlateVisibility::Visible);
			return;
		}

		if (!Widget->IsInViewport())
		{
			Widget->AddToViewport(ZOrder);
		}
	}

	// 스택 Pop 대상 위젯을 숨김
	void HideManagedWidget(UPRWidgetBase* Widget)
	{
		if (!IsValid(Widget))
		{
			return;
		}

		if (Widget->IsInViewport())
		{
			Widget->RemoveFromParent();
			return;
		}

		if (IsValid(Widget->GetParent()))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPRUIManagerSubsystem::Deinitialize()
{
	ResetSystem();
	Super::Deinitialize();
}

void UPRUIManagerSubsystem::ResetSystem()
{
	for (int32 Index = UIStack.Num() - 1; Index >= 0; --Index)
	{
		UPRWidgetBase* Widget = UIStack[Index];
		if (!IsValid(Widget))
		{
			continue;
		}
		
		HideManagedWidget(Widget);
	}
	UIStack.Reset();
	
	if (APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		// UI 입력 중 게임 뷰포트 밖 마우스 이동 방지
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
	}
}

UPRWidgetBase* UPRUIManagerSubsystem::PushUI(TSubclassOf<UPRWidgetBase> WidgetClass)
{
	if (!WidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UPRWidgetBase* Widget = CreateWidget<UPRWidgetBase>(PC, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport(GetZOrderForLayer(Widget->Layer));
	UIStack.Insert(Widget, FindLayerInsertIndex(Widget->Layer));
	UpdateInputMode();
	return Widget;
}

UPRWidgetBase* UPRUIManagerSubsystem::PushUIInstance(UPRWidgetBase* Instance)
{
	if (!IsValid(Instance))
	{
		return nullptr;
	}

	// 이미 스택에 존재하면 중복 추가 없이 표시 상태만 복구
	if (UIStack.Contains(Instance))
	{
		ShowManagedWidget(Instance, GetZOrderForLayer(Instance->Layer));
		UpdateInputMode();
		return Instance;
	}

	// 뷰포트 위젯은 직접 부착하고, 부모 패널 위젯은 표시 상태만 전환
	ShowManagedWidget(Instance, GetZOrderForLayer(Instance->Layer));

	UIStack.Insert(Instance, FindLayerInsertIndex(Instance->Layer));
	UpdateInputMode();
	return Instance;
}

UPRWidgetBase* UPRUIManagerSubsystem::PopUI(UPRWidgetBase* Instance)
{
	CleanupInvalidEntries();

	if (UIStack.IsEmpty() && !IsValid(Instance))
	{
		return nullptr;
	}

	UPRWidgetBase* Target = Instance;
	if (!Target)
	{
		Target = UIStack.Last();
	}

	if (IsValid(Target))
	{
		UIStack.Remove(Target);
		HideManagedWidget(Target);
	}

	UpdateInputMode();
	
	return Target;
}

bool UPRUIManagerSubsystem::IsUIActive(TSubclassOf<UPRWidgetBase> WidgetClass) const
{
	for (const TObjectPtr<UPRWidgetBase>& Widget : UIStack)
	{
		if (IsValid(Widget) && Widget->IsA(WidgetClass))
		{
			return true;
		}
	}
	return false;
}

UPRWidgetBase* UPRUIManagerSubsystem::GetTopWidget() const
{
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]))
		{
			return UIStack[i];
		}
	}
	return nullptr;
}

void UPRUIManagerSubsystem::UpdateInputMode()
{
	CleanupInvalidEntries();

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	// 스택 위에서부터 InputMode != None인 첫 엔트리 탐색
	EPBUIInputMode EffectiveMode = EPBUIInputMode::None;
	bool bEffectiveCursor = false;
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]) && UIStack[i]->InputMode != EPBUIInputMode::None)
		{
			EffectiveMode = UIStack[i]->InputMode;
			bEffectiveCursor = UIStack[i]->bShowMouseCursor;
			break;
		}
	}

	switch (EffectiveMode)
	{
	case EPBUIInputMode::UIOnly:
		{
			FInputModeUIOnly InputMode;
			if (UPRWidgetBase* Top = GetTopWidget())
			{
				InputMode.SetWidgetToFocus(Top->TakeWidget());
			}
			// UI 전용 입력 중 게임 뷰포트 밖 마우스 이동 방지
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			PC->FlushPressedKeys();
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::GameAndUI:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			// 게임 및 UI 입력 중 게임 뷰포트 밖 마우스 이동 방지
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::None:
	default:
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
		break;
	}

	PC->bShowMouseCursor = bEffectiveCursor;
}

int32 UPRUIManagerSubsystem::FindLayerInsertIndex(EPRUILayer Layer) const
{
	// 같은 레이어 내에서는 새 위젯이 더 위에 오도록, 더 높은 레이어보다는 아래에 오도록 삽입
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]) && UIStack[i]->Layer <= Layer)
		{
			return i + 1;
		}
	}
	return 0;
}

int32 UPRUIManagerSubsystem::GetZOrderForLayer(EPRUILayer Layer) const
{
	// 레이어당 100 단위 간격. 같은 레이어 내에서는 AddToViewport 호출 순서로 자연스럽게 적층
	constexpr int32 LayerZStep = 100;
	return static_cast<int32>(Layer) * LayerZStep;
}

void UPRUIManagerSubsystem::CleanupInvalidEntries()
{
	UIStack.RemoveAll([](const TObjectPtr<UPRWidgetBase>& Widget)
	{
		return !IsManagedWidgetAttached(Widget);
	});
}
