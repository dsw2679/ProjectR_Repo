// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (획득 아이템 목록 동기화 구현)
// Author: 배유찬 (인벤토리 아이템 탭 전환 및 리스트 렌더링 구현)
// Author: 이건주 (캐릭터 3D 프리뷰 연동 리스트 UI 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRInventoryItemListWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UPRItemSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRInventoryItemSelectedSignature, const FPRInventoryItemSlotViewData&, ViewData);

// 무기 또는 Mod 선택 목록을 표시하는 인벤토리 리스트 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRInventoryItemListWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 인벤토리 리스트 위젯의 UI 스택 속성을 초기화한다
	UPRInventoryItemListWidget();

	// 리스트 타입과 표시 항목을 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetItemList(EPRItemType InListType, const TArray<FPRInventoryItemSlotViewData>& InItems);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetListText(FText InText);

	// 현재 리스트 타입을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	EPRItemType GetListType() const {return ListType;}

	// 현재 표시 항목 목록을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	TArray<FPRInventoryItemSlotViewData> GetItems() const {return Items;}

protected:
	/*~ UUserWidget Interface ~*/
	// 디자이너와 런타임에서 슬롯 수 변경을 반영한다
	virtual void NativePreConstruct() override;

	// 초기화 시 동적 슬롯을 생성하고 클릭 이벤트를 바인딩한다
	virtual void NativeOnInitialized() override;

private:
	// RowCount와 ColumnCount에 맞춰 동적 슬롯을 다시 생성한다
	void RebuildNativeItemSlots();

	// 생성한 동적 슬롯과 이벤트 바인딩을 정리한다
	void ClearGeneratedItemSlots();

	// 생성한 슬롯에 현재 리스트 항목을 반영한다
	void RebuildNativeItemList();

	// 슬롯을 패널 종류에 맞게 추가한다
	void AddItemSlotToPanel(UPRItemSlotWidget* ItemSlotWidget, int32 SlotIndex);

	// 슬롯 목록에 유효한 슬롯을 추가하고 클릭 이벤트를 바인딩한다
	void AddGeneratedItemSlot(UPRItemSlotWidget* ItemSlotWidget);

	// 리스트 슬롯 좌클릭을 선택 이벤트로 전달한다
	UFUNCTION()
	void HandleItemSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 리스트 슬롯 우클릭을 선택 이벤트로 전달한다
	UFUNCTION()
	void HandleItemSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData);

public:
	// 아이템 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Inventory")
	FPRInventoryItemSelectedSignature OnItemSelected;

	// 아이템 우클릭 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Inventory")
	FPRInventoryItemSelectedSignature OnItemRightClicked;

protected:
	// 동적 슬롯이 추가될 UMG 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPanelWidget> ItemListPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemListTypeText;

	// 생성할 슬롯 행 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (ClampMin = "0", UIMin = "0"))
	int32 RowCount = 2;

	// 생성할 슬롯 열 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (ClampMin = "1", UIMin = "1"))
	int32 ColumnCount = 5;

	// 동적으로 생성할 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TSubclassOf<UPRItemSlotWidget> ItemSlotWidgetClass;

private:
	// 동적으로 생성한 슬롯 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRItemSlotWidget>> ItemSlotWidgets;

	// 현재 리스트 타입
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	EPRItemType ListType = EPRItemType::None;

	// 현재 표시 항목 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FPRInventoryItemSlotViewData> Items;
};
