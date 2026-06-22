// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (HUD Message UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRHUDMessageWidget.generated.h"

class UPanelWidget;
class UTextBlock;
struct FInstancedStruct;
struct FGameplayTag;

// HUD 중앙 안내 메시지 표시 위젯
UCLASS(BlueprintType)
class PROJECTR_API UPRHUDMessageWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRHUDMessageWidget();

	// HUD 안내 메시지를 표시한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Message")
	void ShowMessage(const FText& InMessage);

	// HUD 안내 메시지를 숨긴다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Message")
	void HideMessage();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// EventManager 콜백으로 HUD 메시지 표시 상태를 갱신한다
	void HandleHUDMessage(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 등록된 EventManager 핸들을 정리한다
	void UnbindFromEventManager();

protected:
	// 메시지 전체 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UPanelWidget> MessagePanel;

	// 메시지 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UTextBlock> MessageText;

private:
	FDelegateHandle HUDMessageEventHandle;
};
