// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI World 핑/마커 타입 및 구조체 정의)
#include "PRWorldMarkerTypes.h"

FPRWorldMarkerVisualData::FPRWorldMarkerVisualData()
{
	// 기본 이름
	MarkerName = NSLOCTEXT("ProjectR", "DefaultWorldMarkerName", "MARKER");
	// 기본 색상
	Color = FLinearColor::White;
	// 기본 아이콘 크기
	IconSize = FVector2D(48.0f, 48.0f);
}
