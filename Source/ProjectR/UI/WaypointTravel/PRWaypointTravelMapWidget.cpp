// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel Map UI 위젯 구현)
#include "PRWaypointTravelMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelNodeWidget.h"
#include "ProjectR/World/PRWorldDataAsset.h"
#include "ProjectR/World/PRWorldRegistry.h"

UPRWaypointTravelMapWidget::UPRWaypointTravelMapWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRWaypointTravelMapWidget::SetMapContext(FGameplayTag InWorldId)
{
	// 선택 월드 지도 컨텍스트 저장
	WorldId = InWorldId;
	NodeOptions = BuildWaypointOptions(InWorldId);
	RefreshWaypointNodes();
}

void UPRWaypointTravelMapWidget::RefreshWaypointNodes()
{
	CollectWaypointNodeWidgets();

	for (UPRWaypointTravelNodeWidget* WaypointNodeWidget : WaypointNodeWidgets)
	{
		if (!IsValid(WaypointNodeWidget) || !WaypointNodeWidget->GetWaypointId().IsValid())
		{
			continue;
		}

		const FGameplayTag NodeWaypointId = WaypointNodeWidget->GetWaypointId();
		const FPRWaypointTravelNodeOption* FoundNodeOption = NodeOptions.FindByPredicate([NodeWaypointId](const FPRWaypointTravelNodeOption& NodeOption)
		{
			return NodeOption.WaypointKey.WaypointId.MatchesTagExact(NodeWaypointId);
		});

		if (FoundNodeOption != nullptr)
		{
			// WorldData와 매칭된 Waypoint 상태 적용
			WaypointNodeWidget->SetNodeOption(*FoundNodeOption);
		}
		else
		{
			// WorldData 미등록 버튼 비활성 처리
			WaypointNodeWidget->ClearNodeOption();
		}
	}
}

void UPRWaypointTravelMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 지도 버튼 상태 초기화
	RefreshWaypointNodes();
}

void UPRWaypointTravelMapWidget::NativeDestruct()
{
	// 지도 버튼 이벤트 정리
	UnbindWaypointNodeWidgets();
	WaypointNodeWidgets.Reset();

	Super::NativeDestruct();
}

void UPRWaypointTravelMapWidget::CollectWaypointNodeWidgets()
{
	UnbindWaypointNodeWidgets();
	WaypointNodeWidgets.Reset();

	if (!IsValid(WidgetTree))
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UPRWaypointTravelNodeWidget* WaypointNodeWidget = Cast<UPRWaypointTravelNodeWidget>(Widget);
		if (!IsValid(WaypointNodeWidget))
		{
			return;
		}

		// 지도 버튼 수집
		WaypointNodeWidgets.Add(WaypointNodeWidget);
	});
}

void UPRWaypointTravelMapWidget::UnbindWaypointNodeWidgets()
{
	WaypointNodeWidgets.Reset();
}

TArray<FPRWaypointTravelNodeOption> UPRWaypointTravelMapWidget::BuildWaypointOptions(FGameplayTag InWorldId) const
{
	TArray<FPRWaypointTravelNodeOption> Options;

	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(WorldRegistry) || !InWorldId.IsValid())
	{
		return Options;
	}

	UPRWorldDataAsset* WorldDataAsset = WorldRegistry->LoadWorldDataSync(InWorldId, UPRWorldRegistry::IsDevTravelEnabled());
	if (!IsValid(WorldDataAsset) || !WorldDataAsset->WorldId.MatchesTagExact(InWorldId))
	{
		return Options;
	}

	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	const bool bDevWorld = !WorldRegistry->IsWorldIdRegistered(InWorldId)
		&& WorldRegistry->IsDevWorldIdRegistered(InWorldId)
		&& UPRWorldRegistry::IsDevTravelEnabled();
	for (const FPRWaypointTravelNodeDefinition& NodeDefinition : WorldDataAsset->WaypointNodes)
	{
		if (!NodeDefinition.WaypointId.IsValid())
		{
			continue;
		}

		FPRWaypointKey WaypointKey;
		WaypointKey.WorldId = InWorldId;
		WaypointKey.WaypointId = NodeDefinition.WaypointId;

		FPRWaypointTravelNodeOption NodeOption;
		NodeOption.WaypointKey = WaypointKey;
		NodeOption.WorldDisplayName = WorldDataAsset->DisplayName;
		NodeOption.WaypointDisplayName = NodeDefinition.DisplayName;
		NodeOption.ThumbnailTexture = NodeDefinition.ThumbnailTexture;
		NodeOption.bUnlocked = IsValid(GameState) && GameState->IsWaypointUnlocked(WaypointKey);
		NodeOption.bDevTravelEnabled = !NodeOption.bUnlocked && bDevWorld;
		Options.Add(NodeOption);
	}

	return Options;
}

const UPRWorldRegistry* UPRWaypointTravelMapWidget::GetWorldRegistry() const
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(Settings))
	{
		return nullptr;
	}

	return Settings->GetWorldRegistrySync();
}
