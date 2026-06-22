// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel UI 위젯 구현)
#include "PRWaypointTravelWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Engine/LocalPlayer.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelMapWidget.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelNodeWidget.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelWorldWidget.h"
#include "ProjectR/World/PRWorldDataAsset.h"
#include "ProjectR/World/PRWorldRegistry.h"

UPRWaypointTravelWidget::UPRWaypointTravelWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRWaypointTravelWidget::RebuildNodeList()
{
	for (UPRWaypointTravelNodeWidget* NodeWidget : NodeWidgets)
	{
		if (IsValid(NodeWidget))
		{
			NodeWidget->RemoveFromParent();
		}
	}

	for (UPRWaypointTravelWorldWidget* WorldWidget : WorldWidgets)
	{
		if (IsValid(WorldWidget))
		{
			// 기존 월드 항목 이벤트 해제
			WorldWidget->OnWorldSelected.RemoveDynamic(this, &ThisClass::HandleWorldSelected);
		}
	}

	NodeWidgets.Reset();
	WorldWidgets.Reset();

	if (!IsValid(NodeListPanel))
	{
		return;
	}

	NodeListPanel->ClearChildren();
	NodeListPanel->SetVisibility(ESlateVisibility::Visible);

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	FPRWaypointTravelNodeOption LastVisitedNodeOption;
	if (IsValid(NodeWidgetClass.Get()) && BuildLastVisitedWaypointOption(LastVisitedNodeOption))
	{
		UPRWaypointTravelNodeWidget* NodeWidget = CreateWidget<UPRWaypointTravelNodeWidget>(
			OwningPlayerController,
			NodeWidgetClass);
		if (IsValid(NodeWidget))
		{
			// 마지막 방문 Waypoint 항목 생성
			NodeWidget->SetNodeOption(LastVisitedNodeOption);
			NodeListPanel->AddChild(NodeWidget);
			NodeWidgets.Add(NodeWidget);
		}
	}

	if (!IsValid(WorldWidgetClass.Get()))
	{
		return;
	}

	TArray<FPRWaypointTravelWorldOption> WorldOptions = BuildWorldOptions();

	for (int32 Index = WorldOptions.Num()-1; Index >= 0; --Index)
	{
		const FPRWaypointTravelWorldOption& WorldOption = WorldOptions[Index];
		UPRWaypointTravelWorldWidget* WorldWidget = CreateWidget<UPRWaypointTravelWorldWidget>(
			OwningPlayerController,
			WorldWidgetClass);
		if (!IsValid(WorldWidget))
		{
			continue;
		}

		// 월드 선택 항목 생성
		WorldWidget->SetWorldOption(WorldOption);
		WorldWidget->OnWorldSelected.RemoveDynamic(this, &ThisClass::HandleWorldSelected);
		WorldWidget->OnWorldSelected.AddDynamic(this, &ThisClass::HandleWorldSelected);
		NodeListPanel->AddChild(WorldWidget);
		WorldWidgets.Add(WorldWidget);
	}
}

void UPRWaypointTravelWidget::RebuildOverview()
{
	// Overview 화면 상태 복귀
	CurrentWorldMapId = FGameplayTag();
	ClearMapWidget();

	if (IsValid(BackButton))
	{
		// 뒤로가기 버튼 숨김
		BackButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	RebuildNodeList();
}

void UPRWaypointTravelWidget::OpenWorldMap(FGameplayTag WorldId)
{
	if (!WorldId.IsValid())
	{
		return;
	}

	// 선택 월드 지도 상태 전환
	CurrentWorldMapId = WorldId;
	RebuildMapWidget(WorldId);

	if (IsValid(BackButton))
	{
		// 월드 지도 뒤로가기 버튼 표시
		BackButton->SetVisibility(ESlateVisibility::Visible);
	}

	RebuildNodeList();
}

void UPRWaypointTravelWidget::SetWorldResetButtonVisible(bool bShouldShow)
{
	bShowWorldResetButton = bShouldShow;
	RefreshWorldResetButtonVisibility();
}


bool UPRWaypointTravelWidget::BuildLastVisitedWaypointOption(FPRWaypointTravelNodeOption& OutOption) const
{
	OutOption = FPRWaypointTravelNodeOption();

	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return false;
	}

	return BuildNodeOptionFromKey(GameState->GetLastVisitedWaypoint(), false, OutOption);
}

TArray<FPRWaypointTravelWorldOption> UPRWaypointTravelWidget::BuildWorldOptions() const
{
	TArray<FPRWaypointTravelWorldOption> WorldOptions;

	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(WorldRegistry))
	{
		return WorldOptions;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return WorldOptions;
	}
	
	if (UPRWorldRegistry::IsDevTravelEnabled())
	{
		for (const TPair<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& Pair : WorldRegistry->GetDevWorldDataById())
		{
			UPRWorldDataAsset* WorldDataAsset = Pair.Value.LoadSynchronous();
			if (!IsValid(WorldDataAsset) || !WorldDataAsset->WorldId.MatchesTagExact(Pair.Key))
			{
				continue;
			}
			
			for (const FPRWaypointTravelNodeDefinition& Node : WorldDataAsset->WaypointNodes)
			{
				FPRWaypointKey Key(WorldDataAsset->WorldId, Node.WaypointId);
				if (!Key.IsValid())
				{
					continue;
				}
				
				GameState->UnlockWaypoint(Key);
			}

			// 개발용 월드 옵션
			FPRWaypointTravelWorldOption WorldOption;
			WorldOption.WorldId = Pair.Key;
			WorldOption.WorldDisplayName = WorldDataAsset->DisplayName;
			WorldOption.ThumbnailTexture = WorldDataAsset->ThumbnailTexture;
			WorldOption.bHasUnlockedWaypoint = true;
			WorldOption.bDevOnly = true;
			WorldOptions.Add(WorldOption);
		}
	}
	else
	{
		for (const TPair<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& Pair : WorldRegistry->GetWorldDataById())
		{
			UPRWorldDataAsset* WorldDataAsset = Pair.Value.LoadSynchronous();
			if (!IsValid(WorldDataAsset) || !WorldDataAsset->WorldId.MatchesTagExact(Pair.Key))
			{
				continue;
			}

			const bool bHasUnlockedWaypoint = GameState->HasUnlockedWaypointInWorld(Pair.Key);
			if (!bHasUnlockedWaypoint)
			{
				continue;
			}

			// 활성 Waypoint 보유 월드 옵션
			FPRWaypointTravelWorldOption WorldOption;
			WorldOption.WorldId = Pair.Key;
			WorldOption.WorldDisplayName = WorldDataAsset->DisplayName;
			WorldOption.ThumbnailTexture = WorldDataAsset->ThumbnailTexture;
			WorldOption.bHasUnlockedWaypoint = bHasUnlockedWaypoint;
			WorldOption.bDevOnly = false;
			WorldOptions.Add(WorldOption);
		}
	}

	return WorldOptions;
}


void UPRWaypointTravelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Travel UI 초기화
	BindChildWidgetEvents();
	RefreshWorldResetButtonVisibility();
	RebuildOverview();
}

void UPRWaypointTravelWidget::NativeDestruct()
{
	// Travel UI 정리
	ClearMapWidget();
	UnbindChildWidgetEvents();
	NodeWidgets.Reset();
	WorldWidgets.Reset();

	Super::NativeDestruct();
}

FReply UPRWaypointTravelWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (GetUIInputAction(InKeyEvent.GetKey()) == EPRUIInputAction::Cancel)
	{
		if (CurrentWorldMapId.IsValid())
		{
			// 지도 화면 취소 입력으로 Overview 복귀
			RebuildOverview();
			return FReply::Handled();
		}

		// 취소 입력 처리
		CloseTravelWidget(true);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPRWaypointTravelWidget::BindChildWidgetEvents()
{
	if (IsValid(CloseButton))
	{
		// 닫기 입력 수신
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}

	if (IsValid(BackButton))
	{
		// 뒤로가기 입력 수신
		BackButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackButtonClicked);
		BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBackButtonClicked);
	}

	if (IsValid(ResetWorldButton))
	{
		// 월드 진행도 리셋 입력 수신
		ResetWorldButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleResetWorldButtonClicked);
		ResetWorldButton->OnClicked.AddDynamic(this, &ThisClass::HandleResetWorldButtonClicked);
	}
}

void UPRWaypointTravelWidget::UnbindChildWidgetEvents()
{
	if (IsValid(CloseButton))
	{
		// 닫기 입력 해제
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}

	if (IsValid(BackButton))
	{
		// 뒤로가기 입력 해제
		BackButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackButtonClicked);
	}

	if (IsValid(ResetWorldButton))
	{
		// 월드 진행도 리셋 입력 해제
		ResetWorldButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleResetWorldButtonClicked);
	}

	for (UPRWaypointTravelWorldWidget* WorldWidget : WorldWidgets)
	{
		if (IsValid(WorldWidget))
		{
			// 월드 선택 해제
			WorldWidget->OnWorldSelected.RemoveDynamic(this, &ThisClass::HandleWorldSelected);
		}
	}
}

bool UPRWaypointTravelWidget::RebuildMapWidget(FGameplayTag WorldId)
{
	ClearMapWidget();

	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(WorldRegistry))
	{
		return false;
	}

	UPRWorldDataAsset* WorldDataAsset = WorldRegistry->LoadWorldDataSync(WorldId, UPRWorldRegistry::IsDevTravelEnabled());
	if (!IsValid(WorldDataAsset) || !WorldDataAsset->WorldId.MatchesTagExact(WorldId))
	{
		return false;
	}

	if (!IsValid(WorldDataAsset->MapWidgetClass.Get()))
	{
		return false;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return false;
	}

	// 월드별 지도 위젯 생성 후 UIManager 스택에 Push
	UPRWaypointTravelMapWidget* MapWidget = Cast<UPRWaypointTravelMapWidget>(UIManager->PushUI(WorldDataAsset->MapWidgetClass));
	if (!IsValid(MapWidget))
	{
		return false;
	}

	CurrentMapWidget = MapWidget;
	CurrentMapWidget->SetMapContext(WorldId);
	return true;
}

void UPRWaypointTravelWidget::ClearMapWidget()
{
	if (IsValid(CurrentMapWidget))
	{
		// UIManager 스택에서 지도 위젯 Pop
		UPRUIManagerSubsystem* UIManager = GetUIManager();
		if (IsValid(UIManager))
		{
			UIManager->PopUI(CurrentMapWidget.Get());
		}
		else
		{
			CurrentMapWidget->RemoveFromParent();
		}
	}

	CurrentMapWidget = nullptr;
}

bool UPRWaypointTravelWidget::BuildNodeOptionFromKey(const FPRWaypointKey& WaypointKey, bool bAllowLockedNode, FPRWaypointTravelNodeOption& OutOption) const
{
	OutOption = FPRWaypointTravelNodeOption();
	if (!WaypointKey.IsValid())
	{
		return false;
	}

	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(WorldRegistry))
	{
		return false;
	}

	UPRWorldDataAsset* WorldDataAsset = WorldRegistry->LoadWorldDataSync(WaypointKey.WorldId, UPRWorldRegistry::IsDevTravelEnabled());
	if (!IsValid(WorldDataAsset) || !WorldDataAsset->WorldId.MatchesTagExact(WaypointKey.WorldId))
	{
		return false;
	}

	FPRWaypointTravelNodeDefinition NodeDefinition;
	if (!WorldDataAsset->FindWaypointNode(WaypointKey.WaypointId, NodeDefinition))
	{
		return false;
	}

	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	const bool bUnlocked = IsValid(GameState) && GameState->IsWaypointUnlocked(WaypointKey);
	const bool bDevTravelEnabled = !bUnlocked
		&& !WorldRegistry->IsWorldIdRegistered(WaypointKey.WorldId)
		&& WorldRegistry->IsDevWorldIdRegistered(WaypointKey.WorldId)
		&& UPRWorldRegistry::IsDevTravelEnabled();
	if (!bAllowLockedNode && !bUnlocked && !bDevTravelEnabled)
	{
		return false;
	}

	OutOption.WaypointKey = WaypointKey;
	OutOption.WorldDisplayName = WorldDataAsset->DisplayName;
	OutOption.WaypointDisplayName = NodeDefinition.DisplayName;
	OutOption.ThumbnailTexture = NodeDefinition.ThumbnailTexture;
	OutOption.bUnlocked = bUnlocked;
	OutOption.bDevTravelEnabled = bDevTravelEnabled;
	return true;
}

const UPRWorldRegistry* UPRWaypointTravelWidget::GetWorldRegistry() const
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(Settings))
	{
		return nullptr;
	}

	return Settings->GetWorldRegistrySync();
}

UPRUIManagerSubsystem* UPRWaypointTravelWidget::GetUIManager() const
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = OwningPlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UPRUIManagerSubsystem>();
}

void UPRWaypointTravelWidget::RefreshWorldResetButtonVisibility()
{
	if (!IsValid(ResetWorldButton))
	{
		return;
	}

	// Waypoint 상호작용 설정 기반 표시 상태
	ResetWorldButton->SetVisibility(bShowWorldResetButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UPRWaypointTravelWidget::CloseTravelWidget(bool bNotifyServerCancel)
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (bNotifyServerCancel && IsValid(PlayerController))
	{
		// 서버 취소 요청
		PlayerController->RequestCancelWaypointTravel();
	}

	if (!IsValid(PlayerController))
	{
		return;
	}

	UPRUIControllerComponent* UIController = PlayerController->GetUIController();
	if (IsValid(UIController))
	{
		// 로컬 UI 닫기
		UIController->CloseWaypointTravel();
	}
}

void UPRWaypointTravelWidget::HandleCloseButtonClicked()
{
	// 닫기 버튼 처리
	CloseTravelWidget(true);
}

void UPRWaypointTravelWidget::HandleBackButtonClicked()
{
	// Overview 복귀 처리
	RebuildOverview();
}

void UPRWaypointTravelWidget::HandleResetWorldButtonClicked()
{
	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	APRPlayGameMode* PlayGameMode = IsValid(World) ? World->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(GameState) || !IsValid(PlayGameMode))
	{
		return;
	}

	// 월드 진행도 초기화 후 기본 웨이포인트 복구
	GameState->ResetWorldProgress();
	PlayGameMode->UnlockDefaultWaypoints();
	RebuildOverview();
}

void UPRWaypointTravelWidget::HandleWorldSelected(FGameplayTag WorldId)
{
	// 월드 지도 열기 처리
	OpenWorldMap(WorldId);
}
