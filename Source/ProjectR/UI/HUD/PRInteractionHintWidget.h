// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction Hint UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PRInteractionHintWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteractionHintWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetHintVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetInteractionHintText(const FText& InText);
	
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetCanInteract(const bool bCanInteractable);
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UPanelWidget> InteractionHintPanel;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UTextBlock> InteractionHintText;
};
