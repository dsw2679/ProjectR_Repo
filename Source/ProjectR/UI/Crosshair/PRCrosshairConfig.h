// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 크로스헤어 설정 클래스 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRCrosshairTypes.h"
#include "Engine/DataAsset.h"
#include "PRCrosshairConfig.generated.h"

class UPRCrosshairInterface;

UCLASS(BlueprintType)
class PROJECTR_API UPRCrosshairConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPRCrosshairType CrosshairType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Size = FVector2D(60.f,60.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D ElementStretchXY = FVector2D(1.f,1.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Thickness = 3.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor ColorAndOpacity = FLinearColor::White;
};