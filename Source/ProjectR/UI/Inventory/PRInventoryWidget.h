// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (보유 골드 실시간 수량 갱신 구현)
// Author: 배유찬 (플레이어 스탯 상세 정보 및 장비 장착 외형 동기화 구현)
// Author: 이건주 (캐릭터 3D 프리뷰 회전 제어 및 무기 상태 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRInventoryWidget.generated.h"

class UPREquipmentManagerComponent;
class UPRItemInstance_Mod;
class UPRItemInstance_Equipment;
class UPRConsumableDataAsset;
class UPRItemDataAsset;
class UPRInventoryComponent;
class UPRInventoryItemListWidget;
class UPRUIManagerSubsystem;
class UPRItemInstance_Consumable;
class UPRItemInstance_Material;
class UPRItemInstance_Weapon;
class UPRItemSlotWidget;
class UPRQuickSlotComponent;
class UPRCharacterPreviewWidget;
class UPRCurrencyDisplayWidget;
class UPRPlayerStatsPanelWidget;
class UPRWeaponManagerComponent;
class UPRCurrencyComponent;
class UTextBlock;
class APRPlayerCharacter;

// 인벤토리 화면의 무기 슬롯과 아이템 선택 목록을 연결하는 최상위 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRInventoryWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRInventoryWidget();

	// 인벤토리 위젯에 표시할 Item 소스와 장착 상태 컴포넌트를 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent, UPREquipmentManagerComponent* InEquipmentManagerComponent);

	// ============ Getter =============
	// 현재 인벤토리 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRInventoryComponent* GetInventoryComponent() const {return InventoryComponent;}

	// 현재 무기 매니저 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const {return WeaponManagerComponent;}

	// 현재 퀵슬롯 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

	// 현재 장비 매니저 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPREquipmentManagerComponent* GetEquipmentManagerComponent() const { return EquipmentManagerComponent; }

	// 무기 리스트 대상 슬롯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	EPRWeaponSlotType GetPendingWeaponListSlot() const {return PendingWeaponListSlot;}
	// =========================
protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 하위 위젯 이벤트를 바인딩하고 표시 상태를 초기화한다
	virtual void NativeConstruct() override;

	// 화면에서 제거될 때 표시 상태와 하위 위젯 이벤트 바인딩을 정리한다
	virtual void NativeDestruct() override;

private:
	// BindWidgetOptional 슬롯들을 반복 처리용 배열로 캐싱한다
	void CacheChildWidgetLists();

	// 하위 슬롯과 리스트 이벤트를 바인딩한다
	void BindChildWidgetEvents();

	// 하위 슬롯과 리스트 이벤트 바인딩을 정리한다
	void UnbindChildWidgetEvents();

	// 인벤토리와 무기 매니저 변경 이벤트를 바인딩한다
	void BindInventorySourceEvents();

	// 인벤토리와 무기 매니저 변경 이벤트 바인딩을 정리한다
	void UnbindInventorySourceEvents();

	// ========= 이벤트 바인드 처리 함수 =========
	// 주무기 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandlePrimaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 주무기 슬롯 우클릭을 처리한다
	UFUNCTION()
	void HandlePrimaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 보조무기 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandleSecondaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 보조무기 슬롯 우클릭을 처리한다
	UFUNCTION()
	void HandleSecondaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 퀵슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandleQuickSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 장비 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandleEquipmentSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 재료 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandleMaterialSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 리스트에서 선택한 아이템을 장착 요청으로 변환한다
	UFUNCTION()
	void HandleItemListSelection(const FPRInventoryItemSlotViewData& ViewData);

	// 인벤토리 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent,const FPRInventoryChangeEventData& EventData);

	// 무기 장착 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot);

	// 퀵슬롯 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex);

	// 장비 슬롯 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleEquipmentChanged(EPREquipmentSlotType ChangedSlot, UPRItemInstance_Equipment* EquipmentItem);

	// 장비 외형 정보 변경 알림을 받아 캐릭터 프리뷰를 갱신
	UFUNCTION()
	void HandleEquipmentVisualInfosChanged(UPREquipmentManagerComponent* ChangedEquipmentManagerComponent);

	// 고철 수량 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleScrapChanged(int32 NewScrap);
	// =======================


	// 지정 슬롯의 무기 목록을 연다
	void OpenWeaponList(EPRWeaponSlotType TargetSlot);

	// 지정 무기의 Mod 목록을 연다
	void OpenModList(UPRItemInstance* TargetItem);

	// 지정 퀵슬롯에 등록할 소비 아이템 목록을 연다
	void OpenConsumableListForQuickSlot(int32 SlotIndex);

	// 지정 장비 슬롯에 장착할 장비 아이템 목록을 연다
	void OpenEquipmentListForSlot(EPREquipmentSlotType SlotType);

	// 보유 재료 아이템 목록을 연다
	void OpenMaterialList();

	// 지정 리스트 타입이 현재 열려 있는지 확인한다
	bool IsItemListOpenAs(EPRItemType ListType) const;

	// 아이템 리스트를 숨긴다
	void CloseItemList();

	// 아이템 리스트에 표시 데이터를 설정하고 UI 스택에 Push한다
	void PushItemList(EPRItemType ListType, const TArray<FPRInventoryItemSlotViewData>& ListItems);

	// 현재 장착 슬롯 위젯을 갱신한다
	void RefreshEquippedSlotWidgets();

	// 현재 퀵슬롯 위젯을 갱신한다
	void RefreshQuickSlotWidgets();

	// 현재 장비 슬롯 위젯을 갱신한다
	void RefreshEquipmentSlotWidgets();

	// 재료 목록 진입 슬롯을 갱신한다
	void RefreshMaterialSlotWidget();

	// 고철 보유량 텍스트를 갱신한다
	void RefreshCurrencyText();

	// 장착 슬롯과 열린 리스트를 현재 데이터 기준으로 갱신한다
	void RefreshInventoryView();

	// 캐릭터 프리뷰 위젯에 현재 플레이어 외형과 무기 상태 소스를 전달한다
	void RefreshCharacterPreviewWidget();

	// 플레이어 스탯 패널 PlayerState 소스 전달
	void RefreshPlayerStatsPanelWidget();

	// 프리뷰 기준이 되는 플레이어 캐릭터를 조회한다
	APRPlayerCharacter* GetPreviewSourceCharacter() const;

	// 무기 슬롯 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildWeaponSlotViewData(EPRWeaponSlotType SlotType) const;

	// 무기 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped) const;

	// Mod 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped) const;

	// 소비 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildConsumableItemViewData(UPRItemInstance_Consumable* ConsumableItem) const;

	// 재료 목록 진입 슬롯 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildMaterialSlotViewData() const;

	// 재료 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildMaterialItemViewData(UPRItemInstance_Material* MaterialItem) const;

	// 퀵슬롯 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildQuickSlotViewData(int32 SlotIndex) const;

	// 장비 슬롯 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildEquipmentSlotViewData(EPREquipmentSlotType SlotType) const;

	// 장비 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildEquipmentItemViewData(UPRItemInstance_Equipment* EquipmentItem, bool bEquipped) const;

	// 비활성화 명령 항목 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildDeactivateActionViewData(EPRItemType ListType, UPRItemInstance* TargetItem) const;

	// 장비 슬롯 표시 이름을 반환한다
	FText GetEquipmentSlotDisplayName(EPREquipmentSlotType SlotType) const;

	// 지정 무기에 Mod가 장착 가능한지 확인한다
	bool IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem) const;

	// 현재 인벤토리 소유자의 재화 컴포넌트를 조회한다
	UPRCurrencyComponent* ResolveCurrencyComponent() const;

	// 로컬 플레이어 UI 매니저를 조회한다
	UPRUIManagerSubsystem* ResolveUIManager() const;

protected:
	// UMG에서 바인딩할 주무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> PrimaryWeaponSlotWidget;

	// UMG에서 바인딩할 보조무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> SecondaryWeaponSlotWidget;

	// UMG에서 바인딩할 1번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemSlotWidget0;

	// UMG에서 바인딩할 2번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemSlotWidget1;

	// UMG에서 바인딩할 3번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemSlotWidget2;

	// UMG에서 바인딩할 4번 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> QuickSlotItemSlotWidget3;

	// UMG에서 바인딩할 머리 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> HeadEquipmentSlotWidget;

	// UMG에서 바인딩할 몸통 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> BodyEquipmentSlotWidget;

	// UMG에서 바인딩할 손 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> HandsEquipmentSlotWidget;

	// UMG에서 바인딩할 다리 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> LegsEquipmentSlotWidget;

	// UMG에서 바인딩할 목걸이 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> AmuletEquipmentSlotWidget;

	// UMG에서 바인딩할 반지 1 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> Ring1EquipmentSlotWidget;

	// UMG에서 바인딩할 반지 2 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> Ring2EquipmentSlotWidget;

	// UMG에서 바인딩할 재료 목록 진입 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> MaterialSlotWidget;

	// UMG에서 바인딩할 아이템 리스트 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRInventoryItemListWidget> ItemListWidget;

	// UMG에서 바인딩할 고철 표시 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRCurrencyDisplayWidget> ScrapDisplayWidget;

	// UMG에서 바인딩할 고철 보유량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ScrapAmountText;

	// UMG에서 바인딩할 캐릭터 프리뷰 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRCharacterPreviewWidget> CharacterPreviewWidget;

	// UMG에서 바인딩할 플레이어 스탯 패널 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRPlayerStatsPanelWidget> PlayerStatsPanelWidget;

private:
	// 아이템 보유 인벤토리 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 장착 무기 보유 무기 매니저 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 소비 아이템 퀵슬롯 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRQuickSlotComponent> QuickSlotComponent;

	// 비무기 장비 장착 상태 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPREquipmentManagerComponent> EquipmentManagerComponent;

	// 고철 보유량을 제공하는 재화 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;

	// 무기 리스트 팝업 위젯에 띄울 아이템의 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	EPRWeaponSlotType PendingWeaponListSlot = EPRWeaponSlotType::None;

	// Mod 장착 타겟 무기 Item
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance> LastFocusedItem;

	// 현재 열린 리스트가 선택 결과 처리에 사용할 숫자 컨텍스트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	int32 LastFocusedIndex = INDEX_NONE;

	// 반복 처리용 퀵슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRItemSlotWidget>> QuickSlotWidgets;

	// 반복 처리용 장비 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRItemSlotWidget>> EquipmentSlotWidgets;

	// 장비 슬롯 위젯 캐시와 같은 인덱스를 사용하는 슬롯 타입 캐시
	UPROPERTY(Transient)
	TArray<EPREquipmentSlotType> EquipmentSlotTypes;
};
