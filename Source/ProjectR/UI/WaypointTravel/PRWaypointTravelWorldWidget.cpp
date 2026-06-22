// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel World UI 위젯 구현)
#include "PRWaypointTravelWorldWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPRWaypointTravelWorldWidget::SetWorldOption(const FPRWaypointTravelWorldOption& InWorldOption)
{
	// 월드 선택 항목 상태 저장
	WorldOption = InWorldOption;
	bHasWorldOption = true;
	RefreshWorldPresentation();
}

void UPRWaypointTravelWorldWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 월드 선택 항목 초기화
	BindChildWidgetEvents();
	RefreshWorldPresentation();
}

void UPRWaypointTravelWorldWidget::NativeDestruct()
{
	// 월드 선택 항목 정리
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRWaypointTravelWorldWidget::BindChildWidgetEvents()
{
	if (IsValid(WorldButton))
	{
		// 버튼 클릭 수신
		WorldButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleWorldButtonClicked);
		WorldButton->OnClicked.AddDynamic(this, &ThisClass::HandleWorldButtonClicked);
	}
}

void UPRWaypointTravelWorldWidget::UnbindChildWidgetEvents()
{
	if (IsValid(WorldButton))
	{
		// 버튼 클릭 해제
		WorldButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleWorldButtonClicked);
	}
}

void UPRWaypointTravelWorldWidget::RefreshWorldPresentation()
{
	const bool bSelectable = bHasWorldOption && (WorldOption.bHasUnlockedWaypoint || WorldOption.bDevOnly);
	if (IsValid(WorldButton))
	{
		// 월드 선택 가능 상태 반영
		WorldButton->SetIsEnabled(bSelectable);
	}

	if (IsValid(WorldNameText))
	{
		// 월드 이름 표시
		WorldNameText->SetText(bHasWorldOption ? WorldOption.WorldDisplayName : FText::GetEmpty());
	}
	
	if (IsValid(WorldThumbnail))
	{
		if (bHasWorldOption)
		{
			WorldThumbnail->SetBrushFromTexture(WorldOption.ThumbnailTexture.LoadSynchronous());	
		}
	}

	if (IsValid(StateText))
	{
		// 개발 모드 상태 표시
		StateText->SetText(bHasWorldOption && WorldOption.bDevOnly ? NSLOCTEXT("ProjectR", "WaypointWorld_Dev", "DEV") : FText::GetEmpty());
	}
}

void UPRWaypointTravelWorldWidget::HandleWorldButtonClicked()
{
	if (!bHasWorldOption || !WorldOption.WorldId.IsValid())
	{
		return;
	}

	// 월드 선택 이벤트 전달
	OnWorldSelected.Broadcast(WorldOption.WorldId);
}
