// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel Node UI 위젯 구현)
#include "PRWaypointTravelNodeWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/Player/PRPlayerController.h"

void UPRWaypointTravelNodeWidget::SetNodeOption(const FPRWaypointTravelNodeOption& InNodeOption)
{
	// 목적지 노드 상태 저장
	NodeOption = InNodeOption;
	bHasNodeOption = true;
	RefreshNodeText();
}

void UPRWaypointTravelNodeWidget::ClearNodeOption()
{
	// 미등록 Waypoint 버튼 상태 초기화
	NodeOption = FPRWaypointTravelNodeOption();
	bHasNodeOption = false;
	RefreshNodeText();
}

void UPRWaypointTravelNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 목적지 버튼 초기화
	BindChildWidgetEvents();
	RefreshNodeText();
}

void UPRWaypointTravelNodeWidget::NativeDestruct()
{
	// 목적지 버튼 정리
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRWaypointTravelNodeWidget::BindChildWidgetEvents()
{
	if (IsValid(NodeButton))
	{
		// 버튼 클릭 수신
		NodeButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleNodeButtonClicked);
		NodeButton->OnClicked.AddDynamic(this, &ThisClass::HandleNodeButtonClicked);
	}
}

void UPRWaypointTravelNodeWidget::UnbindChildWidgetEvents()
{
	if (IsValid(NodeButton))
	{
		// 버튼 클릭 해제
		NodeButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleNodeButtonClicked);
	}
}

void UPRWaypointTravelNodeWidget::RefreshNodeText()
{
	const bool bTravelEnabled = bHasNodeOption && (NodeOption.bUnlocked || NodeOption.bDevTravelEnabled);
	if (IsValid(NodeButton))
	{
		// 이동 가능 상태 반영
		NodeButton->SetIsEnabled(bTravelEnabled);
	}

	if (IsValid(NodeThumbnail))
	{
		NodeThumbnail->SetBrushFromTexture(NodeOption.ThumbnailTexture.LoadSynchronous());
	}

	if (IsValid(WorldNameText))
	{
		// 맵 이름 표시
		WorldNameText->SetText(bHasNodeOption ? NodeOption.WorldDisplayName : FText::GetEmpty());
	}

	if (IsValid(WaypointNameText))
	{
		// 웨이포인트 이름 표시
		WaypointNameText->SetText(bHasNodeOption ? NodeOption.WaypointDisplayName : FText::GetEmpty());
	}

	if (IsValid(StateText))
	{
		// 잠금과 개발 모드 상태 표시
		FText StateDisplayText = FText::GetEmpty();
		if (!bHasNodeOption)
		{
			StateDisplayText = NSLOCTEXT("ProjectR", "WaypointNode_Unregistered", "미등록");
		}
		else if (NodeOption.bDevTravelEnabled)
		{
			StateDisplayText = NSLOCTEXT("ProjectR", "WaypointNode_Dev", "DEV");
		}
		else if (!NodeOption.bUnlocked)
		{
			StateDisplayText = NSLOCTEXT("ProjectR", "WaypointNode_Locked", "잠김");
		}

		StateText->SetText(StateDisplayText);
	}
}

void UPRWaypointTravelNodeWidget::HandleNodeButtonClicked()
{
	if (!bHasNodeOption || (!NodeOption.bUnlocked && !NodeOption.bDevTravelEnabled))
	{
		return;
	}

	if (!NodeOption.WaypointKey.IsValid())
	{
		return;
	}

	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	// 서버 이동 요청
	PlayerController->RequestWaypointTravel(NodeOption.WaypointKey);
}
