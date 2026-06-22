// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 재화 유효성 검사 및 구매 비용 레이아웃 구현)
// Author: 배유찬 (상점 UI 스택 관리 및 인벤토리 판매 목록 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopWidget.generated.h"

class UButton;
class UImage;
class UPRCurrencyDisplayWidget;
class UPRItemDataAsset;
class UPRShopComponent;
class UPRShopItemDetailWidget;
class UPRShopItemListWidget;
class UTextBlock;

// 상점 거래 UI의 네이티브 기반 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRShopWidget();
	
	// 상점 거래 요청 대상 Context를 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetShopContext(UPRShopComponent* InShopComponent);

	// 현재 상점 거래 요청 대상 Context를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	UPRShopComponent* GetShopComponent() const { return ShopComponent; }

	// 구매 탭을 선택하고 목록을 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void OpenBuyTab();

	// 판매 탭을 선택하고 목록을 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void OpenSellTab();

	// 상점 화면을 UI 스택에서 닫기
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void CloseShop();

protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 하위 위젯 이벤트를 바인딩
	virtual void NativeConstruct() override;

	// 위젯 표시가 끝날 때 이벤트 바인딩을 정리
	virtual void NativeDestruct() override;

private:
	// 하위 위젯 이벤트를 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩을 정리
	void UnbindChildWidgetEvents();

	// 인벤토리와 재화 변경 이벤트를 바인딩
	void BindSourceEvents();

	// 인벤토리와 재화 변경 이벤트 바인딩을 정리
	void UnbindSourceEvents();

	// 현재 로컬 플레이어의 인벤토리와 재화 컴포넌트를 조회
	void RefreshPlayerSources();

	// 현재 탭의 아이템 목록을 갱신
	void RefreshItemList();

	// 상점 이름과 보유 고철 표시를 갱신
	void RefreshHeader();

	// 선택 아이템 상세 위젯을 갱신
	void RefreshSelectedItemDetails();

	// 거래 버튼 표시와 활성 상태를 갱신
	void RefreshTransactionButtons();

	// 현재 탭 선택 표시를 갱신
	void RefreshTabVisuals();

	// 새 목록 기준으로 선택 슬롯 데이터를 갱신
	void SyncSelectedItemFromList(const TArray<FPRShopItemSlotViewData>& ListItems);

	// 구매 탭 표시 데이터를 만듬
	TArray<FPRShopItemSlotViewData> BuildBuyListItems() const;

	// 판매 탭 표시 데이터를 만듬
	TArray<FPRShopItemSlotViewData> BuildSellListItems() const;

	// 상점 Entry를 슬롯 표시 데이터로 변환
	FPRShopItemSlotViewData BuildShopSlotViewData(const FPRShopEntry& Entry, EPRShopTabType TabType) const;

	// 상점 Entry의 구매 재료 비용 표시 데이터를 만듬
	bool BuildMaterialCostViewData(const FPRShopEntry& Entry, int32 TransactionQuantity, TArray<FPRShopMaterialCostViewData>& OutMaterialCosts) const;

	// 지정 아이템 데이터의 현재 보유 수량을 반환
	int32 GetOwnedStackCount(const UPRItemDataAsset* ItemData) const;

	// 선택된 구매 재료 비용을 보유했는지 확인
	bool HasEnoughSelectedMaterialCosts() const;

	// 현재 선택으로 거래 요청이 가능한지 확인
	bool CanRequestSelectedTransaction() const;

	// 구매 탭 버튼 입력을 처리
	UFUNCTION()
	void HandleBuyTabButtonClicked();

	// 판매 탭 버튼 입력을 처리
	UFUNCTION()
	void HandleSellTabButtonClicked();

	// 구매 버튼 입력을 처리
	UFUNCTION()
	void HandleBuyButtonClicked();

	// 판매 버튼 입력을 처리
	UFUNCTION()
	void HandleSellButtonClicked();

	// 닫기 버튼 입력을 처리
	UFUNCTION()
	void HandleCloseButtonClicked();

	// 상점 목록 선택을 처리
	UFUNCTION()
	void HandleShopItemSelected(const FPRShopItemSlotViewData& ViewData);

	// 인벤토리 변경에 따라 현재 목록을 갱신
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData);

	// 고철 변경에 따라 보유 고철 표시와 버튼 상태를 갱신
	UFUNCTION()
	void HandleScrapChanged(int32 NewScrap);

	// 구매 결과에 따라 상점 UI를 갱신
	UFUNCTION()
	void HandleShopBuyResult(const FPRShopBuyResult& Result);

	// 판매 결과에 따라 상점 UI를 갱신
	UFUNCTION()
	void HandleShopSellResult(const FPRShopSellResult& Result);

protected:
	// UMG에서 바인딩할 상점 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> ShopNameText;

	// UMG에서 바인딩할 보유 고철 표시 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UPRCurrencyDisplayWidget> OwnedScrapText;

	// UMG에서 바인딩할 구매 탭 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> BuyTabButton;

	// UMG에서 바인딩할 판매 탭 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> SellTabButton;

	// UMG에서 바인딩할 구매 탭 선택 표시 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> BuyTabSelectedImage;

	// UMG에서 바인딩할 판매 탭 선택 표시 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> SellTabSelectedImage;

	// UMG에서 바인딩할 상점 아이템 목록 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UPRShopItemListWidget> ItemListWidget;

	// UMG에서 바인딩할 선택 아이템 상세 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UPRShopItemDetailWidget> SelectedItemDetailWidget;

	// UMG에서 바인딩할 구매 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> BuyButton;

	// UMG에서 바인딩할 판매 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> SellButton;

	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UButton> CloseButton;

private:
	// 현재 UI가 거래 요청에 사용할 상점 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRShopComponent> ShopComponent;

	// 현재 플레이어 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 현재 플레이어 재화 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;

	// 현재 선택된 상점 탭
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	EPRShopTabType CurrentTabType = EPRShopTabType::Buy;

	// 현재 선택된 상점 슬롯 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	FPRShopItemSlotViewData SelectedItem;
};
