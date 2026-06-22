// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 크로스헤어 인터페이스 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRCrosshairInterface.generated.h"

class UPRCrosshairConfig;
// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UPRCrosshairInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTR_API IPRCrosshairInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void InitCrosshair(const UPRCrosshairConfig* InConfig);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnHit();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnRecoil(float Speed, float Strength);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPreviewHit(bool bHit);
};
