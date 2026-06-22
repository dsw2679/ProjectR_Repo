// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 강화 연출 및 재화/재료 소모 연동 구현)
// Author: 배유찬 (강화 UI 스택 제어 및 장착 무기 정보 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/ItemSystem/Types/PRWeaponUpgradeTypes.h"
#include "PRWeaponUpgradeWidget.generated.h"

class UButton;
class UPanelWidget;
class UPRCurrencyComponent;
class UPRInventoryItemListWidget;
class UPRItemInstance_Weapon;
class UPRItemSlotWidget;
class UPRWeaponUpgradeMaterialCostWidget;
class UPRWeaponUpgradeComponent;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRWeaponUpgradePreviewChangedSignature, const FPRWeaponUpgradePreview&, Preview);

// 무기 강화 UI의 C++ 기반 Context 보관 위젯이다
UCLASS()
class PROJECTR_API UPRWeaponUpgradeWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRWeaponUpgradeWidget();
	
	// 강화 UI가 사용할 강화 컴포넌트 Context를 지정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void SetUpgradeContext(UPRWeaponUpgradeComponent* InUpgradeComponent);

	// 현재 강화 컴포넌트 Context를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	UPRWeaponUpgradeComponent* GetUpgradeContext() const { return UpgradeComponent; }

	// 선택한 무기의 강화를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void RequestUpgradeWeapon(UPRItemInstance_Weapon* WeaponItem);

	// 현재 선택된 무기를 지정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void SelectWeapon(UPRItemInstance_Weapon* WeaponItem);

	// 현재 선택된 무기를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	UPRItemInstance_Weapon* GetSelectedWeapon() const { return SelectedWeaponItem; }

	// 현재 강화 미리보기 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradePreview GetCurrentPreview() const { return CurrentPreview; }

	// 무기 목록과 선택 무기 표시를 다시 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void RefreshWeaponUpgradeView();

	// 강화 화면을 UI 스택에서 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void CloseWeaponUpgrade();

protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 하위 위젯 이벤트와 소스 이벤트를 바인딩한다
	virtual void NativeConstruct() override;

	// 화면에서 제거될 때 이벤트 바인딩을 정리한다
	virtual void NativeDestruct() override;

private:
	// 하위 위젯 이벤트를 바인딩한다
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩을 정리한다
	void UnbindChildWidgetEvents();

	// 인벤토리와 재화 변경 이벤트를 바인딩한다
	void BindSourceEvents();

	// 인벤토리와 재화 변경 이벤트 바인딩을 정리한다
	void UnbindSourceEvents();

	// 플레이어 소유 인벤토리와 재화 컴포넌트를 다시 찾는다
	void ResolvePlayerSources();

	// 보유 무기 리스트를 갱신한다
	void RefreshWeaponList();

	// 선택 무기 슬롯을 갱신한다
	void RefreshSelectedWeaponSlot();

	// 선택 무기의 다음 강화 비용 표시를 갱신한다
	void RefreshUpgradePreview();

	// 강화 버튼 활성 상태를 갱신한다
	void RefreshUpgradeButton();

	// 재료 비용 아이콘 목록을 갱신한다
	void RefreshMaterialCostWidgets();

	// 생성한 재료 비용 위젯을 정리한다
	void ClearMaterialCostWidgets();

	// 비용 표시 위젯을 하나 생성해 패널에 추가한다
	void AddCostWidget(const FPRWeaponUpgradeMaterialCostViewData& CostViewData);

	// 무기 아이템 슬롯 표시 데이터를 만든다
	FPRInventoryItemSlotViewData BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bSelected) const;

	// 무기 목록에서 선택한 아이템을 처리한다
	UFUNCTION()
	void HandleWeaponListSelection(const FPRInventoryItemSlotViewData& ViewData);

	// 강화 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleUpgradeButtonClicked();

	// 닫기 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCloseButtonClicked();

	// 인벤토리 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData);

	// 고철 수량 변경 알림을 받아 비용 표시를 갱신한다
	UFUNCTION()
	void HandleScrapChanged(int32 NewScrap);

	// 강화 결과 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleWeaponUpgradeResult(const FPRWeaponUpgradeResult& Result);

public:
	// 강화 미리보기 데이터가 갱신되었을 때 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradePreviewChangedSignature OnUpgradePreviewChanged;

protected:
	// UMG에서 바인딩할 보유 무기 목록 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPRInventoryItemListWidget> WeaponListWidget;

	// UMG에서 바인딩할 선택 무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPRItemSlotWidget> SelectedWeaponSlotWidget;

	// UMG에서 바인딩할 강화 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UButton> UpgradeButton;

	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UButton> CloseButton;

	// UMG에서 바인딩할 강화 단계 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTextBlock> UpgradeLevelText;

	// UMG에서 바인딩할 강화 전후 데미지 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTextBlock> UpgradeDamageText;

	// UMG에서 바인딩할 재료 비용 목록 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPanelWidget> MaterialCostPanel;

	// 재료 비용 목록에 동적으로 생성할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TSubclassOf<UPRWeaponUpgradeMaterialCostWidget> MaterialCostWidgetClass;

	// 고철 비용 표시용 아이콘 텍스처
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTexture2D> ScrapIconTexture;

	// UMG에서 바인딩할 강화 결과 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTextBlock> UpgradeResultText;

private:
	// 현재 UI가 요청을 보낼 강화 컴포넌트
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRWeaponUpgradeComponent> UpgradeComponent;

	// 현재 선택한 강화 대상 무기
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> SelectedWeaponItem;

	// 현재 플레이어 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 현재 플레이어 재화 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;

	// 동적으로 생성한 재료 비용 위젯 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWeaponUpgradeMaterialCostWidget>> MaterialCostWidgets;

	// 현재 강화 미리보기 데이터
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (AllowPrivateAccess = "true"))
	FPRWeaponUpgradePreview CurrentPreview;
};
