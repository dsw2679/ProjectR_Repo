// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel Node UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelNodeWidget.generated.h"

class UImage;
class APRPlayerController;
class UButton;
class UTextBlock;


// 웨이포인트 Travel UI의 단일 목적지 버튼
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelNodeWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 지도 Blueprint에서 지정한 Waypoint ID 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|WaypointTravel")
	FGameplayTag GetWaypointId() const { return WaypointId; }

	// 버튼 표시와 요청에 사용할 목적지 노드 지정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void SetNodeOption(const FPRWaypointTravelNodeOption& InNodeOption);

	// 현재 WorldData와 매칭되지 않는 Waypoint 버튼 상태 초기화
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void ClearNodeOption();

	// 현재 버튼 목적지 노드 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|WaypointTravel")
	const FPRWaypointTravelNodeOption& GetNodeOption() const { return NodeOption; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 하위 위젯 이벤트 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩 해제
	void UnbindChildWidgetEvents();

	// 목적지 이름 표시 갱신
	void RefreshNodeText();

	// 버튼 클릭 처리
	UFUNCTION()
	void HandleNodeButtonClicked();

protected:
	// 지도 Blueprint에서 직접 지정하는 Waypoint 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "SpawnPoint"))
	FGameplayTag WaypointId;

	// UMG에서 바인딩할 목적지 버튼
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> NodeButton;

	// UMG에서 바인딩할 목적지 썸네일
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UImage> NodeThumbnail;

	// UMG에서 바인딩할 맵 이름 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> WorldNameText;

	// UMG에서 바인딩할 웨이포인트 이름 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> WaypointNameText;

	// UMG에서 바인딩할 잠금 또는 개발 모드 상태 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> StateText;

private:
	// 현재 버튼이 요청할 목적지 노드
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	FPRWaypointTravelNodeOption NodeOption;

	// WorldData와 매칭된 Waypoint 상태 보유 여부
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	bool bHasNodeOption = false;
};
