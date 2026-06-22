// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopItemListWidget.generated.h"

class UPanelWidget;
class UPRShopItemSlotWidget;
class UScrollBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopItemSelectedSignature, const FPRShopItemSlotViewData&, ViewData);

// 상점 아이템 목록을 동적 슬롯으로 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopItemListWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 표시할 상점 슬롯 목록을 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetShopItemList(EPRShopTabType InTabType, const TArray<FPRShopItemSlotViewData>& InItems);

	// 현재 표시 중인 탭 유형을 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	EPRShopTabType GetTabType() const { return TabType; }

	// 현재 표시 항목 목록을 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	TArray<FPRShopItemSlotViewData> GetItems() const { return Items; }

protected:
	/*~ UUserWidget Interface ~*/
	// 디자이너와 런타임에서 슬롯 수 변경을 반영
	virtual void NativePreConstruct() override;

	// 초기화 시 동적 슬롯을 생성하고 클릭 이벤트를 바인딩
	virtual void NativeOnInitialized() override;

private:
	// 기본 표시 칸과 아이템 수에 맞춰 동적 슬롯을 다시 생성
	void RebuildNativeItemSlots();

	// 기본 표시 칸과 아이템 수를 반영한 슬롯 수 계산
	int32 GetDesiredSlotCount() const;

	// 생성한 동적 슬롯과 이벤트 바인딩을 정리
	void ClearGeneratedItemSlots();

	// 생성한 슬롯에 현재 리스트 항목을 반영
	void RebuildNativeItemList();

	// 리스트 스크롤 위치 초기화
	void ResetItemListScroll();

	// 슬롯을 패널 종류에 맞게 추가
	void AddItemSlotToPanel(UPRShopItemSlotWidget* ItemSlotWidget, int32 SlotIndex);

	// 슬롯 목록에 유효한 슬롯을 추가하고 클릭 이벤트를 바인딩
	void AddGeneratedItemSlot(UPRShopItemSlotWidget* ItemSlotWidget);

	// 리스트 슬롯 선택을 상위 위젯으로 전달
	UFUNCTION()
	void HandleItemSlotClicked(const FPRShopItemSlotViewData& ViewData);

public:
	// 아이템 선택 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopItemSelectedSignature OnItemSelected;

protected:
	// 동적 슬롯이 추가될 UMG 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UPanelWidget> ItemListPanel;

	// 아이템 목록 그리드를 감싸는 스크롤 박스
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UScrollBox> ItemListScrollBox;

	// 현재 탭 이름을 표시할 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> ItemListTypeText;

	// 생성할 슬롯 행 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 RowCount = 3;

	// 생성할 슬롯 열 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "1", UIMin = "1"))
	int32 ColumnCount = 4;

	// 동적으로 생성할 상점 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	TSubclassOf<UPRShopItemSlotWidget> ItemSlotWidgetClass;

private:
	// 동적으로 생성한 슬롯 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRShopItemSlotWidget>> ItemSlotWidgets;

	// 현재 표시 중인 탭 유형
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	EPRShopTabType TabType = EPRShopTabType::Buy;

	// 현재 표시 항목 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	TArray<FPRShopItemSlotViewData> Items;
};
