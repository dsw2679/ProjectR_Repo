// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (World 핑/마커 UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PRWorldMarkerTypes.h"
#include "PRWorldMarkerWidget.generated.h"

class UImage;
class UOverlay;
class UTextBlock;
class UWidgetAnimation;

// 단일 월드 마커 위젯
UCLASS(Abstract, Blueprintable, BlueprintType)
class PROJECTR_API UPRWorldMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 마커 데이터 초기화
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void InitializeMarker(const FPRWorldMarkerData& InMarkerData);

	// 마커 화면 상태 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void RefreshMarker(const FVector2D& ScreenPosition, float DistanceMeters, bool bOnScreen, float ArrowAngle);

	// 마커 표시 스타일 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void ApplyVisualData(const FPRWorldMarkerVisualData& VisualData);

	// 마커 식별자 반환
	FGuid GetMarkerId() const { return MarkerData.MarkerId; }

	// 마커 데이터 반환
	const FPRWorldMarkerData& GetMarkerData() const { return MarkerData; }

	// 마커 만료 여부 반환
	bool IsExpired(float CurrentTimeSeconds) const;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

protected:
	// 핑 생성 연출 재생
	void PlayPingAnimation();

protected:
	// 마커 중심 아이콘
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UImage> MarkerIcon;

	// 오프스크린 화살표 영역
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UOverlay> OV_Arrow;

	// 오프스크린 화살표 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UImage> ArrowIcon;

	// 마커 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UTextBlock> MarkerNameText;

	// 마커 거리 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UTextBlock> DistanceText;

	// 핑 생성 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PingAnimation;

private:
	// 현재 마커 데이터
	UPROPERTY(Transient)
	FPRWorldMarkerData MarkerData;

	// 로컬 생성 시간
	float LocalCreatedTime = 0.0f;

	// 거리 텍스트 표시 시작 거리
	float CachedDistanceVisibleMinMeters = 0.0f;
};
