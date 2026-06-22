// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Party Health List UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPartyHealthListWidget.generated.h"

class APRPlayerState;
class UPRPartyMemberHealthWidget;
class UPanelWidget;

// 파티원 체력 목록을 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPartyHealthListWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRPartyHealthListWidget(const FObjectInitializer& ObjectInitializer);

	// 현재 GameState PlayerArray 기반 파티원 체력 슬롯 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Party")
	void RefreshPartyMembers();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 파티원 체력 슬롯 패널 자식 캐싱
	void CachePartyMemberSlots();

	// GameState 플레이어 수 변경 이벤트 구독
	void BindPlayerCountChanged();

	// GameState 플레이어 수 변경 이벤트 구독 해제
	void UnbindPlayerCountChanged();

	// GameState 플레이어 수 변경 이벤트 수신 처리
	void HandlePlayerCountChanged();

	// 파티원 목록 갱신 처리
	void RefreshPartyMembersInternal(int32 RemainingRetryCount);

	// 초기 복제 지연 대응용 갱신 재시도 예약
	void ScheduleRefreshRetry(int32 RemainingRetryCount);

	APRPlayerState* GetOwningPRPlayerState() const;
	void ApplyPartyMembers(const TArray<APRPlayerState*>& PartyMembers);

private:
	// 파티원 체력 슬롯들을 배치하는 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UPanelWidget> PartyMemberListPanel;

	// 패널 자식에서 수집한 파티원 체력 슬롯 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRPartyMemberHealthWidget>> PartyMemberSlots;

	// GameState 플레이어 수 변경 이벤트 핸들
	FDelegateHandle PlayerCountChangedDelegateHandle;
};
