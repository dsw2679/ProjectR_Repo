// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Main Title UI base implementation)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "PRMainTitleWidget.generated.h"

class UWidgetAnimation;

DECLARE_MULTICAST_DELEGATE(FPRMainTitleContinueRequestedSignature);

// 메인 메뉴 진입 전 표시되는 타이틀 화면 위젯
UCLASS(Blueprintable)
class PROJECTR_API UPRMainTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 타이틀 화면 위젯 기본 설정 초기화
	UPRMainTitleWidget(const FObjectInitializer& ObjectInitializer);

	// 계속 진행 요청 이벤트 반환
	FPRMainTitleContinueRequestedSignature& GetOnContinueRequested() { return OnContinueRequested; }

	// 타이틀 화면에서 시작 메뉴 진입을 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Menu")
	void RequestContinue();

protected:
	/*~ UUserWidget Interface ~*/
	// 위젯 구성 후 키보드 포커스와 안내 애니메이션을 시작
	virtual void NativeConstruct() override;

	// 위젯 제거 시 지연 타이머를 정리
	virtual void NativeDestruct() override;

	// 키 입력으로 시작 메뉴 진입 요청
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 마우스 클릭으로 시작 메뉴 진입 요청
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// FadeOut 완료 후 계속 진행 이벤트 발행
	void FinishContinueRequest();

protected:
	// 타이틀 화면 퇴장 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FadeOut;

	// Press Any Key 안내 반복 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Flickering;

private:
	// 계속 진행 요청 이벤트
	FPRMainTitleContinueRequestedSignature OnContinueRequested;

	// FadeOut 완료 후 전환 타이머
	FTimerHandle ContinueRequestTimerHandle;

	// 중복 진행 요청 방지
	bool bContinueRequested = false;
};
