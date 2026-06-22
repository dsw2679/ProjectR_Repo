// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (가방 위젯 구조 정의)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRBagWidget.generated.h"

class UPRConsumableDataAsset;
class UPRCurrencyComponent;
class UPRCurrencyDisplayWidget;
class UPRInventoryItemListWidget;
class UPRQuickSlotComponent;

// 소비 아이템, 소재 아이템, 퀵슬롯 선택 목록을 표시하는 가방 위젯
UCLASS(BlueprintType)
class PROJECTR_API UPRBagWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 가방 데이터 소스 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Bag")
	void SetBagSources(UPRInventoryComponent* InInventoryComponent, UPRQuickSlotComponent* InQuickSlotComponent);

protected:
	/*~ UUserWidget Interface ~*/
	// 위젯 표시 초기화
	virtual void NativeConstruct() override;

	// 위젯 제거 정리
	virtual void NativeDestruct() override;

private:
	// 지정 아이템 목록 갱신
	void RefreshItemList(EPRItemType ItemType, UPRInventoryItemListWidget* TargetListWidget);

	// 퀵슬롯 선택 목록 갱신
	void RefreshQuickSlotItemList();

	// 전체 목록 갱신
	void RefreshBagLists();

	// 고철 보유량 표시 갱신
	void RefreshCurrencyText();

	// 퀵슬롯 등록 대기 상태 초기화
	void ClearPendingQuickSlotRegistration();

	// 현재 인벤토리 소유자의 재화 컴포넌트 조회
	UPRCurrencyComponent* ResolveCurrencyComponent() const;

	// 소비 아이템 선택 처리
	UFUNCTION()
	void HandleConsumableItemSelected(const FPRInventoryItemSlotViewData& ViewData);

	// 소비 아이템 사용 처리
	UFUNCTION()
	void HandleConsumableItemUseRequested(const FPRInventoryItemSlotViewData& ViewData);

	// 퀵슬롯 선택 처리
	UFUNCTION()
	void HandleQuickSlotSelected(const FPRInventoryItemSlotViewData& ViewData);

	// 인벤토리 변경 처리
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData);

	// 퀵슬롯 변경 처리
	UFUNCTION()
	void HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex);

	// 고철 수량 변경 처리
	UFUNCTION()
	void HandleScrapChanged(int32 NewScrap);

protected:
	// UMG에서 바인딩할 소비 아이템 리스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Bag")
	TObjectPtr<UPRInventoryItemListWidget> ConsumableItemListWidget;

	// UMG에서 바인딩할 소재 아이템 리스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Bag")
	TObjectPtr<UPRInventoryItemListWidget> MaterialItemListWidget;

	// UMG에서 바인딩할 퀵슬롯 선택 리스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Bag")
	TObjectPtr<UPRInventoryItemListWidget> QuickSlotItemListWidget;

	// UMG에서 바인딩할 고철 표시 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Bag")
	TObjectPtr<UPRCurrencyDisplayWidget> ScrapDisplayWidget;

private:
	// 보유 아이템 조회 소스
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Bag", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 퀵슬롯 등록 상태 소스
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Bag", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRQuickSlotComponent> QuickSlotComponent;

	// 퀵슬롯 등록 대기 소비 아이템 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Bag", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRConsumableDataAsset> PendingQuickSlotConsumableData;

	// 고철 보유량 제공 소스
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Bag", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;
};
