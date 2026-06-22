// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (퀵슬롯 슬롯 UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Types/PRQuickSlotTypes.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRQuickSlotWidget.generated.h"

class UPRItemSlotWidget;
class UPRQuickSlotComponent;

// 퀵슬롯 4칸의 아이콘과 보유 개수를 표시하는 HUD 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRQuickSlotWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 퀵슬롯 표시 소스 컴포넌트를 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void SetQuickSlotComponent(UPRQuickSlotComponent* InQuickSlotComponent);

	// 전체 퀵슬롯 표시를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void RefreshQuickSlots();
	
	// 소유 플레이어의 퀵슬롯 표시 소스를 초기화한다
	void InitializeQuickSlotHUD();
protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 퀵슬롯 변경 알림을 받아 표시를 갱신한다
	UFUNCTION()
	void HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex);

	// 지정 슬롯 표시를 갱신한다
	void RefreshQuickSlot(int32 SlotIndex);

	// 지정 슬롯 위젯을 반환한다
	UPRItemSlotWidget* GetQuickSlotItemWidget(int32 SlotIndex) const;

	// 퀵슬롯 표시 데이터를 아이템 슬롯 표시 데이터로 변환한다
	FPRInventoryItemSlotViewData BuildItemSlotViewData(const FPRQuickSlotViewData& QuickSlotViewData) const;

protected:
	// UMG에서 바인딩할 1번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemWidget0;

	// UMG에서 바인딩할 2번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemWidget1;

	// UMG에서 바인딩할 3번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemWidget2;

	// UMG에서 바인딩할 4번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemWidget3;

private:
	// 현재 퀵슬롯 표시 소스 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRQuickSlotComponent> QuickSlotComponent = nullptr;
};
