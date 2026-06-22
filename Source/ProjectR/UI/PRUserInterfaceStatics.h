// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (크로스헤어 및 사격 UI 정적 호출 구현)
// Author: 이건주 (무기 상태 및 드론 전용 UI 정적 호출 구현)
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRUserInterfaceStatics.generated.h"

enum class EPRCrosshairType : uint8;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRUserInterfaceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static TSubclassOf<UUserWidget> GetCrosshairWidgetClass(const EPRCrosshairType CrosshairType);

	// 소수 값을 UI 표시용 정수 또는 지정 소수점 텍스트로 변환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	static FText ConvertFloatToText(float Value, int32 MaximumFractionalDigits = 0);

	// Current가 Min과 Max 사이에서 차지하는 비율을 0.0부터 1.0 사이로 변환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	static float ConvertMinMaxToPercent(float CurrentValue, float MaxValue, float MinValue = 0.0f);
};
