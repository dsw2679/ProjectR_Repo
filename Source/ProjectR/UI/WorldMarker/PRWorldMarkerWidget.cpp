// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (World 핑/마커 UI 위젯 구현)
#include "PRWorldMarkerWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "ProjectR/System/PRDeveloperSettings.h"

// ===== UUserWidget =====

void UPRWorldMarkerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 입력 차단 방지
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWorldMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 설정값 캐시
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	CachedDistanceVisibleMinMeters = IsValid(Settings) ? Settings->WorldMarkerDistanceVisibleMinMeters : 0.0f;
}

// ===== 마커 갱신 =====

void UPRWorldMarkerWidget::InitializeMarker(const FPRWorldMarkerData& InMarkerData)
{
	// 표시 데이터 캐시
	MarkerData = InMarkerData;
	LocalCreatedTime = IsValid(GetWorld()) ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 초기 스타일 반영
	ApplyVisualData(MarkerData.VisualData);
	PlayPingAnimation();
}

void UPRWorldMarkerWidget::RefreshMarker(const FVector2D& ScreenPosition, float DistanceMeters, bool bOnScreen, float ArrowAngle)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		// 투영 좌표 반영
		CanvasSlot->SetPosition(ScreenPosition);
	}

	if (IsValid(DistanceText))
	{
		if (DistanceMeters < CachedDistanceVisibleMinMeters)
		{
			// 근거리 숨김
			DistanceText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 원거리 표시
			DistanceText->SetVisibility(ESlateVisibility::HitTestInvisible);
			// 미터 단위 거리 표시
			DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f M"), DistanceMeters)));
		}
	}

	if (IsValid(OV_Arrow))
	{
		// 화면 밖 방향 표시
		OV_Arrow->SetVisibility(bOnScreen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		OV_Arrow->SetRenderTransformAngle(ArrowAngle);
	}
}

void UPRWorldMarkerWidget::ApplyVisualData(const FPRWorldMarkerVisualData& VisualData)
{
	if (IsValid(MarkerNameText))
	{
		if (!VisualData.MarkerName.IsEmpty())
		{
			MarkerNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
			// 이름 텍스트 반영
			MarkerNameText->SetText(VisualData.MarkerName);	
		}
		else
		{
			MarkerNameText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(MarkerIcon))
	{
		// 아이콘 텍스처 적용
		if (!VisualData.Icon.IsNull())
		{
			MarkerIcon->SetBrushFromSoftTexture(VisualData.Icon, true);
		}

		MarkerIcon->SetColorAndOpacity(VisualData.Color);
		// 아이콘 크기 반영
		MarkerIcon->SetDesiredSizeOverride(VisualData.IconSize);
	}

	if (IsValid(ArrowIcon))
	{
		// 화살표 색상 반영
		ArrowIcon->SetColorAndOpacity(VisualData.Color);
	}
}

bool UPRWorldMarkerWidget::IsExpired(float CurrentTimeSeconds) const
{
	if (MarkerData.Duration <= 0.0f)
	{
		// 영구 마커
		return false;
	}

	// 로컬 수명 판정
	return CurrentTimeSeconds - LocalCreatedTime >= MarkerData.Duration;
}

// ===== 애니메이션 =====

void UPRWorldMarkerWidget::PlayPingAnimation()
{
	if (!IsValid(PingAnimation))
	{
		return;
	}

	// 핑 생성 연출
	PlayAnimation(PingAnimation, 0.0f, 1);
}
