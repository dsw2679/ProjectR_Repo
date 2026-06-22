// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (픽업 Notification Entry UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/HUD/PRPickupNotificationTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPickupNotificationEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;
class UPRPickupNotificationEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRPickupNotificationEntryFinishedSignature, UPRPickupNotificationEntryWidget*, EntryWidget);

UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPickupNotificationEntryWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRPickupNotificationEntryWidget(const FObjectInitializer& ObjectInitializer);

	// 획득 알림 표시 데이터 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void SetNotificationData(const FPRPickupNotificationViewData& InViewData);

	// 획득 알림 등장 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void PlayFadeInAnimation();

	// 획득 알림 퇴장 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void PlayFadeOutAnimation();

	// 진행 중인 퇴장 애니메이션 취소
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void CancelFadeOutAnimation();

public:
	// FadeOut 종료 시 컨테이너에 제거를 알리는 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|HUD|Pickup")
	FPRPickupNotificationEntryFinishedSignature OnFadeOutFinished;

protected:
	/*~ UUserWidget Interface ~*/
	// FadeOut 종료 콜백 바인딩
	virtual void NativeConstruct() override;

	// FadeOut 종료 콜백 정리
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleFadeOutFinished();

protected:
	// UMG 트리에서 같은 이름으로 바인딩할 알림 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UTextBlock> InfoText;

	// UMG 트리에서 같은 이름으로 바인딩할 알림 아이콘
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UImage> ItemIcon;

	// 알림 등장 애니메이션
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UWidgetAnimation> FadeIn;

	// 알림 퇴장 애니메이션
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UWidgetAnimation> FadeOut;

private:
	bool bWaitingFadeOutFinished = false;
};
