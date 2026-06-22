// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Floating Text UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PRFloatingTextTypes.h"
#include "TimerManager.h"
#include "PRFloatingTextWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

// 플로팅 텍스트 위젯 베이스 클래스
// BP에서 상속하여 텍스트 표시 연출(애니메이션, 색상 등)을 구현한다
UCLASS(Abstract, Blueprintable, BlueprintType)
class PROJECTR_API UPRFloatingTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 매니저가 스타일을 반영한 초기화 파라미터를 전달한다
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void InitFloatingText(const FPRFloatingTextParams& Params);

	// 표시할 텍스트를 설정한다
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void SetText(const FText& InText);
	
	// 텍스트 색상을 설정한다. DeveloperSettings에서 지정된 색상이 적용된다
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void SetTextColor(FLinearColor InColor);

	// 풀 재사용 전 남은 애니메이션과 지연 액션 제거
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void ResetFloatingTextForReuse();

	// 바인딩된 PlayAnim 애니메이션 재생 및 자동 반납 예약
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void PlayFloatingText();
	
	// 텍스트 애니메이션 재생 등 월드 스폰 후 동작
	UFUNCTION(BlueprintNativeEvent, Category = "FloatingText")
	void OnPlay();
	
	// 연출이 끝났을 때 호출한다. Manager가 풀에 반환하도록 델리게이트를 트리거한다
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void FinishFloatingText();

protected:
	/*~ UUserWidget Interface ~*/
	// PlayAnim 완료 콜백 바인딩
	virtual void NativeConstruct() override;

	// PlayAnim 완료 콜백 및 fallback 타이머 정리
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePlayAnimationFinished();

	void HandleFinishFallbackElapsed();
	void ClearFinishFallbackTimer();

public:
	// 연출 완료 시 매니저에게 통보하는 델리게이트
	DECLARE_DELEGATE_OneParam(FOnFloatingTextFinishedSignature, UPRFloatingTextWidget* /*Widget*/);
	FOnFloatingTextFinishedSignature OnFloatingTextFinished;

protected:
	// BP 디자이너에서 바인딩할 텍스트 블록 (선택적)
	UPROPERTY(BlueprintReadOnly, Category = "FloatingText", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	// BP 디자이너에서 같은 이름으로 생성할 플로팅 텍스트 재생 애니메이션
	UPROPERTY(BlueprintReadOnly, Transient, Category = "FloatingText", meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PlayAnim;

	// AnimationFinished 이벤트 누락 대비 추가 대기 시간
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText", meta = (ClampMin = "0.0"))
	float FinishFallbackPaddingSeconds = 0.25f;

	// PlayAnim이 없는 위젯의 풀 반납 fallback 시간
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText", meta = (ClampMin = "0.0"))
	float NoAnimationFallbackSeconds = 1.0f;

private:
	FTimerHandle FinishFallbackTimerHandle;

	bool bWaitingPlayAnimationFinished = false;
	bool bFinishNotified = true;
};
