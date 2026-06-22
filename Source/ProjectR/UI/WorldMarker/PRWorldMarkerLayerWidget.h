// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (World 핑/마커 Layer UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PRWorldMarkerTypes.h"
#include "PRWorldMarkerLayerWidget.generated.h"

class UCanvasPanel;
class UPRWorldMarkerWidget;
struct FGameplayTag;
struct FInstancedStruct;

// HUD 내부 월드 마커 레이어 위젯
UCLASS(Abstract, Blueprintable, BlueprintType)
class PROJECTR_API UPRWorldMarkerLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 마커 추가 또는 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void AddOrUpdateMarker(const FPRWorldMarkerData& MarkerData);

	// 마커 제거
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void RemoveMarker(FGuid MarkerId);

	// 모든 마커 제거
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WorldMarker")
	void ClearMarkers();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// 월드 마커 이벤트 처리
	void HandleWorldMarkerEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 마커 월드 위치와 표시 스타일 해석
	bool ResolveMarkerWorldLocation(const FPRWorldMarkerData& MarkerData, FVector& OutWorldLocation, FPRWorldMarkerVisualData& OutVisualData, bool& bOutVisible) const;

	// 월드 위치를 레이어 좌표로 투영
	bool ProjectMarkerToLayer(
		const FVector& WorldLocation,
		const FVector2D& LayerSize,
		FVector2D& OutScreenPosition,
		bool& bOutOnScreen,
		float& OutArrowAngle) const;

	// 방향 벡터를 화살표 각도로 변환
	float ConvertDirectionToArrowAngle(const FVector2D& Direction) const;

	// 만료 마커 정리
	void RetireExpiredMarkers();

protected:
	// 마커 배치 캔버스
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "WorldMarker")
	TObjectPtr<UCanvasPanel> MarkerCanvas;

	// 단일 마커 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WorldMarker")
	TSubclassOf<UPRWorldMarkerWidget> MarkerWidgetClass;

	// 화면 가장자리 안전 여백
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WorldMarker", meta = (ClampMin = "0.0"))
	float EdgePadding = 48.0f;

private:
	// 활성 마커 위젯 목록
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<UPRWorldMarkerWidget>> ActiveMarkerWidgets;

	// 이벤트 구독 핸들
	FDelegateHandle WorldMarkerEventHandle;
};
