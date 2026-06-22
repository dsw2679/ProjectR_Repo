// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI Interaction Progress Bar 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRInteractionProgressBar.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * Hold 상호작용 진행도를 표시하는 HUD 위젯.
 * BP UMG 트리에 동일 이름("ProgressBar") UProgressBar 가 있으면 자동 바인딩된다.
 * 이벤트 구독은 컨테이너인 UPRHUDWidget 이 담당하며, 본 위젯은 표시·진행도 갱신 인터페이스만 제공한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRInteractionProgressBar : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRInteractionProgressBar();

	// 진행도(0~1) 적용. ProgressBar 가 바인딩되어 있을 때만 동작
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetProgress(float InPercent);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetActionNameText(const FText& InText);
	
	// 위젯 가시성 토글. 비표시 시 진행도 0으로 초기화
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Interaction")
	void SetProgressBarVisible(bool bVisible);

protected:
	// BP UMG 트리에서 이름이 동일한 UProgressBar 가 있을 때 자동 바인딩
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UProgressBar> ProgressBar;
	
	// 상호작용 이름 (예: 소생 중)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "HUD")
	TObjectPtr<UTextBlock> ActionNameText; 
};
