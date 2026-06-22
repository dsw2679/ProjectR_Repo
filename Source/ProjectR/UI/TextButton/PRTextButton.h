// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "PRTextButton.generated.h"

class UTextBlock;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRTextButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	// 버튼 표시 텍스트 설정
	UFUNCTION(BlueprintCallable)
	void SetText(FText InText);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TextButton")
	TObjectPtr<UTextBlock> ButtonText;
};
