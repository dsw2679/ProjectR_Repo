// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelWidget.generated.h"

class UButton;
class UPanelWidget;
class UPRUIManagerSubsystem;
class UPRWaypointTravelMapWidget;
class UPRWaypointTravelNodeWidget;
class UPRWaypointTravelWorldWidget;
class UPRWorldRegistry;

// Waypoint Travel 화면 전환을 담당하는 호스트 전용 루트 UI
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRWaypointTravelWidget();

	// 마지막 방문 Waypoint와 월드 선택 항목을 단일 Overview 목록으로 재생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void RebuildNodeList();

	// 마지막 방문 Waypoint와 월드 목록을 표시할 Overview 데이터 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void RebuildOverview();

	// 선택한 월드의 지도 Waypoint 목록 표시
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void OpenWorldMap(FGameplayTag WorldId);

	// 월드 진행도 리셋 버튼 표시 상태 설정
	void SetWorldResetButtonVisible(bool bShouldShow);

	// 마지막 방문 Waypoint 노드 옵션 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	bool BuildLastVisitedWaypointOption(FPRWaypointTravelNodeOption& OutOption) const;

	// Registry와 진행 상태 기준 월드 목록 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	TArray<FPRWaypointTravelWorldOption> BuildWorldOptions() const;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 하위 위젯 이벤트 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩 해제
	void UnbindChildWidgetEvents();

	// 선택 월드의 지도 위젯을 생성하여 UIManager에 Push
	bool RebuildMapWidget(FGameplayTag WorldId);

	// 현재 지도 위젯을 UIManager에서 Pop하여 제거
	void ClearMapWidget();

	// WaypointKey 기준 목적지 노드 옵션 생성
	bool BuildNodeOptionFromKey(const FPRWaypointKey& WaypointKey, bool bAllowLockedNode, FPRWaypointTravelNodeOption& OutOption) const;

	// 프로젝트 월드 Registry 조회
	const UPRWorldRegistry* GetWorldRegistry() const;

	// 로컬 플레이어 UI 매니저 서브시스템 조회
	UPRUIManagerSubsystem* GetUIManager() const;

	// Travel UI 닫기 처리
	void CloseTravelWidget(bool bNotifyServerCancel);

	// 월드 진행도 리셋 버튼 표시 갱신
	void RefreshWorldResetButtonVisibility();

	// 닫기 버튼 클릭 처리
	UFUNCTION()
	void HandleCloseButtonClicked();

	// 뒤로가기 버튼 클릭 처리
	UFUNCTION()
	void HandleBackButtonClicked();

	// 월드 진행도 리셋 버튼 클릭 처리
	UFUNCTION()
	void HandleResetWorldButtonClicked();

	// 월드 선택 처리
	UFUNCTION()
	void HandleWorldSelected(FGameplayTag WorldId);

protected:
	// 목적지 노드 버튼으로 생성할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSubclassOf<UPRWaypointTravelNodeWidget> NodeWidgetClass;

	// Overview 월드 선택 항목으로 생성할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSubclassOf<UPRWaypointTravelWorldWidget> WorldWidgetClass;

	// UMG에서 바인딩할 Overview 항목 목록 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UPanelWidget> NodeListPanel;

	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> CloseButton;

	// UMG에서 바인딩할 월드 지도 뒤로가기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> BackButton;

	// UMG에서 바인딩할 월드 진행도 리셋 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> ResetWorldButton;

private:
	// 현재 지도 화면에서 선택된 월드 ID
	UPROPERTY(Transient)
	FGameplayTag CurrentWorldMapId;

	// 동적으로 생성한 목적지 노드 버튼 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWaypointTravelNodeWidget>> NodeWidgets;

	// 동적으로 생성한 월드 선택 버튼 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWaypointTravelWorldWidget>> WorldWidgets;

	// 현재 표시 중인 월드 지도 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRWaypointTravelMapWidget> CurrentMapWidget;

	// 현재 Travel UI의 월드 진행도 리셋 버튼 표시 여부
	bool bShowWorldResetButton = false;
};
