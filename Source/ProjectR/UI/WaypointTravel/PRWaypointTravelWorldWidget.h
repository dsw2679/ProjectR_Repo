// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel World UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelWorldWidget.generated.h"

class UImage;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRWaypointTravelWorldSelectedSignature, FGameplayTag, WorldId);

// Overview에서 월드를 선택하는 단일 항목 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelWorldWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 월드 선택 항목 표시와 선택 이벤트에 사용할 상태 지정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void SetWorldOption(const FPRWaypointTravelWorldOption& InWorldOption);

	// 현재 월드 선택 항목 상태 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|WaypointTravel")
	const FPRWaypointTravelWorldOption& GetWorldOption() const { return WorldOption; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 하위 위젯 이벤트 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩 해제
	void UnbindChildWidgetEvents();

	// 월드 이름과 상태 표시 갱신
	void RefreshWorldPresentation();

	// 월드 선택 버튼 클릭 처리
	UFUNCTION()
	void HandleWorldButtonClicked();

public:
	// 월드 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|WaypointTravel")
	FPRWaypointTravelWorldSelectedSignature OnWorldSelected;

protected:
	// UMG에서 바인딩할 월드 선택 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> WorldButton;
	
	// 월드 썸네일 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UImage> WorldThumbnail;

	// UMG에서 바인딩할 월드 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> WorldNameText;

	// UMG에서 바인딩할 월드 상태 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> StateText;

private:
	// 현재 월드 선택 항목 상태
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	FPRWaypointTravelWorldOption WorldOption;

	// 월드 선택 항목 상태 보유 여부
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	bool bHasWorldOption = false;
};
