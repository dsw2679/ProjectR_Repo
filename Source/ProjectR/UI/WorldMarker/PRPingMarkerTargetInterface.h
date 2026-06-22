// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 핑 핑/마커 타겟 인터페이스 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRWorldMarkerTypes.h"
#include "PRPingMarkerTargetInterface.generated.h"

class APlayerController;

UINTERFACE(BlueprintType)
class PROJECTR_API UPRPingMarkerTargetInterface : public UInterface
{
	GENERATED_BODY()
};

// 핑 마커 대상 인터페이스
class PROJECTR_API IPRPingMarkerTargetInterface
{
	GENERATED_BODY()

public:
	// 마커 기준 월드 위치 반환
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|WorldMarker")
	FVector GetPingMarkerWorldLocation() const;
	virtual FVector GetPingMarkerWorldLocation_Implementation() const = 0;
	
	// 마커 표시 스타일 반환
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|WorldMarker")
	FPRWorldMarkerVisualData GetPingMarkerVisualData() const;
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const = 0;

	// 마커 표시 여부 반환
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|WorldMarker")
	bool ShouldPingMarkerVisible() const;
	virtual bool ShouldPingMarkerVisible_Implementation() const { return true; }
	
	// 요청자 기준 핑 허용 여부 반환
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|WorldMarker")
	bool CanCreatePingMarker(APlayerController* RequestingController) const;
	virtual bool CanCreatePingMarker_Implementation(APlayerController* RequestingController) const {return true;}
};
