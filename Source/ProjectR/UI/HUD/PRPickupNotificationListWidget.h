// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (픽업 Notification List UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/UI/HUD/PRPickupNotificationTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPickupNotificationListWidget.generated.h"

class UPRPickupNotificationEntryWidget;
class UTexture2D;
class UVerticalBox;

USTRUCT()
struct FPRPickupNotificationEntryState
{
	GENERATED_BODY()

	// 현재 VerticalBox에 등록된 알림 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRPickupNotificationEntryWidget> EntryWidget = nullptr;

	// 알림 병합 여부를 판단할 보상 종류
	UPROPERTY(Transient)
	EPRRewardType RewardType = EPRRewardType::None;

	// 아이템 또는 탄약 알림 병합 여부를 판단할 Primary Asset Id
	UPROPERTY(Transient)
	FPrimaryAssetId ItemAssetId;

	// 현재 알림에 누적된 수량
	UPROPERTY(Transient)
	int32 Quantity = 0;

	FTimerHandle DisplayTimerHandle;
};

UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPickupNotificationListWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRPickupNotificationListWidget(const FObjectInitializer& ObjectInitializer);

	// 드롭 보상 획득 알림 추가
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void AddNotification(const FPRPickupNotificationPayload& Payload);

protected:
	/*~ UUserWidget Interface ~*/
	// 등록된 제거 타이머 정리
	virtual void NativeDestruct() override;

private:
	FPRPickupNotificationViewData MakeViewData(const FPRPickupNotificationPayload& Payload) const;
	FPRPickupNotificationEntryState* FindMergeTarget(const FPRPickupNotificationPayload& Payload);
	bool IsSameNotification(const FPRPickupNotificationEntryState& EntryState, const FPRPickupNotificationPayload& Payload) const;
	void RestartDisplayTimer(FPRPickupNotificationEntryState& EntryState);
	void RemoveOldestNotificationImmediately();
	void RemoveNotificationImmediately(UPRPickupNotificationEntryWidget* EntryWidget);
	void ClearDisplayTimer(UPRPickupNotificationEntryWidget* EntryWidget);

	UFUNCTION()
	void HandleDisplayDurationExpired(UPRPickupNotificationEntryWidget* EntryWidget);

	UFUNCTION()
	void HandleEntryFadeOutFinished(UPRPickupNotificationEntryWidget* EntryWidget);

protected:
	// UMG 트리에서 같은 이름으로 바인딩할 알림 목록 VerticalBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UVerticalBox> NotificationListBox;

	// 생성할 개별 획득 알림 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|Pickup")
	TSubclassOf<UPRPickupNotificationEntryWidget> EntryWidgetClass;

	// 동시에 표시할 최대 알림 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|Pickup", meta = (ClampMin = "1"))
	int32 MaxVisibleCount = 4;

	// FadeOut 시작 전까지 유지할 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|Pickup", meta = (ClampMin = "0.0"))
	float DisplayDuration = 5.0f;

	// 고철 획득 알림에 사용할 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|Pickup")
	TObjectPtr<UTexture2D> ScrapIconTexture = nullptr;

private:
	UPROPERTY(Transient)
	TArray<FPRPickupNotificationEntryState> EntryStates;
};
