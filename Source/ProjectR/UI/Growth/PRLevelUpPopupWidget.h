// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (레벨 Up Popup UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRLevelUpPopupWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRLevelUpPopupWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRLevelUpPopupWidget(const FObjectInitializer& ObjectInitializer);

	// 레벨업 팝업 문구 갱신 및 표시
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void ShowLevelUp(int32 PreviousLevel, int32 CurrentLevel);

protected:
	/*~ UUserWidget Interface ~*/
	// 위젯 생성 시 기본 숨김 상태 적용
	virtual void NativeConstruct() override;

	// 위젯 해제 시 자동 숨김 타이머 정리
	virtual void NativeDestruct() override;

	// 레벨업 팝업 표시 애니메이션 재생
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Growth")
	void PlayLevelUpFadeAnimation();

	// 팝업 표시 직후 BP 연출 재생 지점
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|Growth")
	void OnLevelUpPopupShown(int32 PreviousLevel, int32 CurrentLevel);

private:
	void HideLevelUpPopup();
	void ClearAutoHideTimer();

protected:
	// UMG에서 바인딩할 레벨 변경 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UTextBlock> Text_Bottom;

	// 레벨업 팝업 표시 애니메이션
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "ProjectR|Growth")
	TObjectPtr<UWidgetAnimation> Fade;

	// 자동 숨김까지 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Growth", meta = (ClampMin = "0.0"))
	float AutoHideDelay = 8.0f;

private:
	FTimerHandle AutoHideTimerHandle;
};
