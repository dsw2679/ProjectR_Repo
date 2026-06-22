// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 등급 비주얼 및 마우스 오버 툴팁 호출 구현)
// Author: 배유찬 (슬롯 클릭 시 장비 장착/해제 어빌리티 연동 구현)
// Author: 이건주 (3D 모델 아이콘 렌더링 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRItemSlotWidget.generated.h"

class UButton;
class UImage;
class UPRUIControllerComponent;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRItemSlotClickedSignature, const FPRInventoryItemSlotViewData&, ViewData);

// 인벤토리에서 아이템 한 칸의 이름, 아이콘, 클릭 이벤트를 담당하는 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRItemSlotWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯 표시 데이터를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetSlotViewData(const FPRInventoryItemSlotViewData& InViewData);

	// 현재 슬롯 표시 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	FPRInventoryItemSlotViewData GetSlotViewData() const {return ViewData;}

protected:
	/*~ UUserWidget Interface ~*/
	// 초기화 시 버튼 이벤트를 바인딩한다
	virtual void NativeOnInitialized() override;

	// 마우스 호버 시작 시 툴팁 생성
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스 호버 종료 시 툴팁 제거
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// 위젯 파괴 시 표시 중인 툴팁을 숨긴다
	virtual void NativeDestruct() override;

	// 마우스 버튼 입력으로 우클릭을 처리한다
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// 클릭 버튼 좌클릭 이벤트를 슬롯 좌클릭으로 전달한다
	UFUNCTION()
	void HandleClickButtonClicked();

	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	void RefreshNativeDisplay();

	// 현재 표시 데이터로 툴팁 위젯을 갱신한다
	void ShowTooltip();

	// 활성 툴팁 위젯을 숨긴다
	void HideTooltip();

	// 소유 플레이어의 UI 컨트롤러를 조회한다
	UPRUIControllerComponent* ResolveUIController() const;

public:
	// 좌클릭 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Inventory")
	FPRItemSlotClickedSignature OnLeftClicked;

	// 우클릭 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Inventory")
	FPRItemSlotClickedSignature OnRightClicked;

protected:
	// UMG에서 바인딩할 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemNameText;

	// UMG에서 바인딩할 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UImage> ItemIconImage;

	// UMG에서 바인딩할 장착 중 표시 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UImage> EquippedIndicatorImage;

	// UMG에서 바인딩할 아이템 보유 개수 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> StackCountText;

	// UMG에서 바인딩할 클릭 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UButton> ClickButton;

private:
	// 현재 슬롯 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	FPRInventoryItemSlotViewData ViewData;

	bool bIsMouseHovering = false;
};
