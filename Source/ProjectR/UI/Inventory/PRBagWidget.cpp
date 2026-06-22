// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (가방 위젯 목록 및 퀵슬롯 등록 구현)

#include "PRBagWidget.h"

#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Types/PRQuickSlotTypes.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/Inventory/PRCurrencyDisplayWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryItemListWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"

/*~ 소스 설정 ~*/

void UPRBagWidget::SetBagSources(UPRInventoryComponent* InInventoryComponent, UPRQuickSlotComponent* InQuickSlotComponent)
{
	if (IsValid(InventoryComponent))
	{
		// 이전 인벤토리 변경 알림 정리
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRBagWidget::HandleInventoryChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		// 이전 퀵슬롯 변경 알림 정리
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		// 이전 고철 변경 알림 정리
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRBagWidget::HandleScrapChanged);
	}

	// 새 데이터 소스 보관
	InventoryComponent = InInventoryComponent;
	QuickSlotComponent = InQuickSlotComponent;
	CurrencyComponent = ResolveCurrencyComponent();

	if (IsValid(InventoryComponent))
	{
		// 새 인벤토리 변경 알림 연결
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRBagWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRBagWidget::HandleInventoryChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		// 새 퀵슬롯 변경 알림 연결
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
		QuickSlotComponent->GetOnQuickSlotChanged().AddDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		// 새 고철 변경 알림 연결
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRBagWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRBagWidget::HandleScrapChanged);
	}

	// 소스 교체 후 표시 동기화
	RefreshBagLists();
	RefreshCurrencyText();
}

/*~ UUserWidget Interface ~*/

void UPRBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(ConsumableItemListWidget))
	{
		// 소비 아이템 선택 알림 연결
		ConsumableItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRBagWidget::HandleConsumableItemSelected);
		ConsumableItemListWidget->OnItemSelected.AddDynamic(this, &UPRBagWidget::HandleConsumableItemSelected);
		// 소비 아이템 사용 알림 연결
		ConsumableItemListWidget->OnItemRightClicked.RemoveDynamic(this, &UPRBagWidget::HandleConsumableItemUseRequested);
		ConsumableItemListWidget->OnItemRightClicked.AddDynamic(this, &UPRBagWidget::HandleConsumableItemUseRequested);
	}

	if (IsValid(QuickSlotItemListWidget))
	{
		// 퀵슬롯 선택 알림 연결
		QuickSlotItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotSelected);
		QuickSlotItemListWidget->OnItemSelected.AddDynamic(this, &UPRBagWidget::HandleQuickSlotSelected);
	}

	if (IsValid(InventoryComponent))
	{
		// 인벤토리 변경 알림 연결
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRBagWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRBagWidget::HandleInventoryChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		// 퀵슬롯 변경 알림 연결
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
		QuickSlotComponent->GetOnQuickSlotChanged().AddDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
	}

	CurrencyComponent = ResolveCurrencyComponent();
	if (IsValid(CurrencyComponent))
	{
		// 고철 변경 알림 연결
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRBagWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRBagWidget::HandleScrapChanged);
	}

	// 위젯 생성 후 표시 동기화
	RefreshBagLists();
	RefreshCurrencyText();
}

void UPRBagWidget::NativeDestruct()
{
	if (IsValid(ConsumableItemListWidget))
	{
		// 소비 아이템 선택 알림 해제
		ConsumableItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRBagWidget::HandleConsumableItemSelected);
		// 소비 아이템 사용 알림 해제
		ConsumableItemListWidget->OnItemRightClicked.RemoveDynamic(this, &UPRBagWidget::HandleConsumableItemUseRequested);
	}

	if (IsValid(QuickSlotItemListWidget))
	{
		// 퀵슬롯 선택 알림 해제
		QuickSlotItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotSelected);
	}

	if (IsValid(InventoryComponent))
	{
		// 인벤토리 변경 알림 해제
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRBagWidget::HandleInventoryChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		// 퀵슬롯 변경 알림 해제
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRBagWidget::HandleQuickSlotChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		// 고철 변경 알림 해제
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRBagWidget::HandleScrapChanged);
	}
	CurrencyComponent = nullptr;

	// 퀵슬롯 등록 대기 상태 정리
	ClearPendingQuickSlotRegistration();

	Super::NativeDestruct();
}

/*~ 목록 갱신 ~*/

void UPRBagWidget::RefreshItemList(EPRItemType ItemType, UPRInventoryItemListWidget* TargetListWidget)
{
	if (!IsValid(TargetListWidget))
	{
		// 표시 대상 리스트 부재
		return;
	}

	TArray<FPRInventoryItemSlotViewData> ListItems;
	if (IsValid(InventoryComponent))
	{
		if (ItemType == EPRItemType::Consumable)
		{
			for (UPRItemInstance_Consumable* ConsumableItem : InventoryComponent->GetItemsByType<UPRItemInstance_Consumable>(EPRItemType::Consumable))
			{
				if (!IsValid(ConsumableItem))
				{
					continue;
				}

				// 소비 아이템 표시 데이터 추가
				ListItems.Add(UPRInventoryItemSlotViewDataBuilder::BuildConsumableItemViewData(ConsumableItem));
			}
		}
		else if (ItemType == EPRItemType::Material)
		{
			for (UPRItemInstance_Material* MaterialItem : InventoryComponent->GetItemsByType<UPRItemInstance_Material>(EPRItemType::Material))
			{
				if (!IsValid(MaterialItem))
				{
					continue;
				}

				// 소재 아이템 표시 데이터 추가
				ListItems.Add(UPRInventoryItemSlotViewDataBuilder::BuildMaterialItemViewData(MaterialItem));
			}
		}
	}

	// 대상 리스트 표시 갱신
	TargetListWidget->SetItemList(ItemType, ListItems);
}

void UPRBagWidget::RefreshCurrencyText()
{
	if (!IsValid(ScrapDisplayWidget))
	{
		return;
	}

	if (!IsValid(CurrencyComponent))
	{
		CurrencyComponent = ResolveCurrencyComponent();
	}

	const int32 ScrapAmount = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
	ScrapDisplayWidget->SetScrapAmount(ScrapAmount);
}

void UPRBagWidget::RefreshQuickSlotItemList()
{
	if (!IsValid(QuickSlotItemListWidget))
	{
		// 퀵슬롯 선택 리스트 부재
		return;
	}

	TArray<FPRInventoryItemSlotViewData> QuickSlotItems;
	if (IsValid(PendingQuickSlotConsumableData) && IsValid(QuickSlotComponent))
	{
		for (const FPRQuickSlotViewData& QuickSlotViewData : QuickSlotComponent->BuildQuickSlotViewDataList())
		{
			FPRInventoryItemSlotViewData SlotViewData = UPRInventoryItemSlotViewDataBuilder::BuildQuickSlotViewData(QuickSlotViewData);
			// 빈 퀵슬롯 선택 허용
			SlotViewData.InventoryAction = EPRInventoryAction::Activate;
			QuickSlotItems.Add(SlotViewData);
		}
	}

	// 퀵슬롯 선택 목록 표시 갱신
	QuickSlotItemListWidget->SetItemList(EPRItemType::Consumable, QuickSlotItems);
	QuickSlotItemListWidget->SetListText(FText::FromString(TEXT("퀵슬롯 등록")));

	const ESlateVisibility QuickSlotListVisibility = IsValid(PendingQuickSlotConsumableData)
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed;
	QuickSlotItemListWidget->SetVisibility(QuickSlotListVisibility);
}

void UPRBagWidget::RefreshBagLists()
{
	// 보유 아이템 목록 갱신
	RefreshItemList(EPRItemType::Consumable, ConsumableItemListWidget);
	RefreshItemList(EPRItemType::Material, MaterialItemListWidget);

	// 퀵슬롯 선택 목록 갱신
	RefreshQuickSlotItemList();
}

void UPRBagWidget::ClearPendingQuickSlotRegistration()
{
	// 등록 대기 소비 아이템 제거
	PendingQuickSlotConsumableData = nullptr;
}

UPRCurrencyComponent* UPRBagWidget::ResolveCurrencyComponent() const
{
	if (!IsValid(InventoryComponent))
	{
		// 인벤토리 소스 부재
		return nullptr;
	}

	const APRPlayerState* PlayerState = Cast<APRPlayerState>(InventoryComponent->GetOwner());
	if (!IsValid(PlayerState))
	{
		// 플레이어 상태 부재
		return nullptr;
	}

	return PlayerState->GetCurrencyComponent();
}

/*~ 입력 처리 ~*/

void UPRBagWidget::HandleConsumableItemSelected(const FPRInventoryItemSlotViewData& ViewData)
{
	UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(ViewData.ItemData.Get());
	if (!IsValid(ConsumableData))
	{
		// 소비 아이템 선택 검증 실패
		ClearPendingQuickSlotRegistration();
		RefreshQuickSlotItemList();
		return;
	}

	// 퀵슬롯 등록 대상 보관
	PendingQuickSlotConsumableData = ConsumableData;

	// 등록 대상 기준 퀵슬롯 선택 목록 표시
	RefreshQuickSlotItemList();
}

void UPRBagWidget::HandleConsumableItemUseRequested(const FPRInventoryItemSlotViewData& ViewData)
{
	if (!IsValid(InventoryComponent) || ViewData.ItemType != EPRItemType::Consumable)
	{
		// 소비 아이템 사용 조건 미충족
		return;
	}

	UPRItemInstance_Consumable* ConsumableItem = Cast<UPRItemInstance_Consumable>(ViewData.ItemInstance.Get());
	if (!IsValid(ConsumableItem) || !ConsumableItem->HasAnyStack())
	{
		// 소비 아이템 인스턴스 검증 실패
		return;
	}

	// 소비 아이템 사용 요청
	InventoryComponent->RequestUseConsumableItem(ConsumableItem);
	
	// 아이템 사용 후 위젯 닫기
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(PlayerController->GetUIController()))
	{
		// UI 컨트롤러 조회 실패
		return;
	}

	// 플레이어 메뉴 닫기
	PlayerController->GetUIController()->ClosePlayerMenu();
}

void UPRBagWidget::HandleQuickSlotSelected(const FPRInventoryItemSlotViewData& ViewData)
{
	if (!IsValid(QuickSlotComponent) || !IsValid(PendingQuickSlotConsumableData))
	{
		// 퀵슬롯 등록 요청 조건 미충족
		ClearPendingQuickSlotRegistration();
		RefreshQuickSlotItemList();
		return;
	}

	if (ViewData.ContextIndex == INDEX_NONE)
	{
		// 퀵슬롯 인덱스 검증 실패
		return;
	}

	// 선택 슬롯에 소비 아이템 등록 요청
	QuickSlotComponent->RequestRegisterQuickSlotItem(ViewData.ContextIndex, PendingQuickSlotConsumableData);

	// 등록 요청 후 선택 상태 정리
	ClearPendingQuickSlotRegistration();
	RefreshQuickSlotItemList();
}

/*~ 변경 반응 ~*/

void UPRBagWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData)
{
	static_cast<void>(EventData);

	if (ChangedInventoryComponent != InventoryComponent)
	{
		// 다른 인벤토리 변경 무시
		return;
	}

	// 보유 수량 변경 반영
	RefreshBagLists();
}

void UPRBagWidget::HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex)
{
	static_cast<void>(ChangedSlotIndex);

	if (ChangedQuickSlotComponent != QuickSlotComponent)
	{
		// 다른 퀵슬롯 변경 무시
		return;
	}

	// 퀵슬롯 표시 상태 반영
	RefreshQuickSlotItemList();
}

void UPRBagWidget::HandleScrapChanged(int32 NewScrap)
{
	static_cast<void>(NewScrap);

	// 고철 표시 상태 반영
	RefreshCurrencyText();
}
