// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (획득 아이템 목록 동기화 구현)
// Author: 배유찬 (인벤토리 아이템 탭 전환 및 리스트 렌더링 구현)
// Author: 이건주 (캐릭터 3D 프리뷰 연동 리스트 UI 구현)
#include "PRInventoryItemListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"

UPRInventoryItemListWidget::UPRInventoryItemListWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRInventoryItemListWidget::SetItemList(EPRItemType InListType, const TArray<FPRInventoryItemSlotViewData>& InItems)
{
	ListType = InListType;	// 표시할 아이템 리스트 타입
	Items = InItems;		// 표시할 아이템들 뷰 데이터

	//ItemListTypeText->SetText()

	FString ItemTypeString;
	switch (ListType)
	{
		case EPRItemType::PrimaryWeapon:
		ItemTypeString = TEXT("주무기");
		break;
		case EPRItemType::SecondaryWeapon:
		ItemTypeString = TEXT("보조무기");
		break;
		case EPRItemType::Consumable:
		ItemTypeString = TEXT("소비");
		break;
		case EPRItemType::Mod:
		ItemTypeString = TEXT("모드");
		break;
		case EPRItemType::Material:
		ItemTypeString = TEXT("재료");
		break;
		case EPRItemType::Equipment:
		ItemTypeString = TEXT("장비");
		break;
		case EPRItemType::Weapon:
		ItemTypeString = TEXT("무기");
		break;
		default:
		ItemTypeString = TEXT("아이템 리스트");
		break;
	}

	if (IsValid(ItemListTypeText))
	{
		ItemListTypeText->SetText(FText::FromString(ItemTypeString));
	}

	// 생성한 슬롯에 현재 리스트 항목을 반영한다
	RebuildNativeItemList();
}

void UPRInventoryItemListWidget::SetListText(FText InText)
{
	if (IsValid(ItemListTypeText))
	{
		ItemListTypeText->SetText(InText);
	}
}

/*~ UUserWidget Interface ~*/

void UPRInventoryItemListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// RowCount와 ColumnCount에 맞춰 동적 슬롯을 다시 생성한다
	RebuildNativeItemSlots();
	// 생성한 슬롯에 현재 리스트 항목을 반영한다
	RebuildNativeItemList();
}

void UPRInventoryItemListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// RowCount와 ColumnCount에 맞춰 동적 슬롯을 다시 생성한다
	RebuildNativeItemSlots();
	// 생성한 슬롯에 현재 리스트 항목을 반영한다
	RebuildNativeItemList();
}

void UPRInventoryItemListWidget::RebuildNativeItemSlots()
{
	// 슬롯 리스트 생성 전 이미 생성된 리스트 정리
	ClearGeneratedItemSlots();

	if (!IsValid(ItemListPanel) || !IsValid(ItemSlotWidgetClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemListWidget] RebuildNativeItemSlots() | 조건이 유효하지 않습니다."));
		return;
	}

	const int32 ValidRowCount = FMath::Max(0, RowCount);
	const int32 ValidColumnCount = FMath::Max(1, ColumnCount);
	const int32 ValidSlotCount = ValidRowCount * ValidColumnCount;
	// 행열 수 만큼 슬롯 생성, 할당
	for (int32 SlotIndex = 0; SlotIndex < ValidSlotCount; ++SlotIndex)
	{
		UPRItemSlotWidget* ItemSlotWidget = nullptr;
		APlayerController* OwningPlayer = GetOwningPlayer();
		if (IsValid(OwningPlayer))
		{
			ItemSlotWidget = CreateWidget<UPRItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass);
		}
		else if (IsValid(WidgetTree))
		{
			ItemSlotWidget = WidgetTree->ConstructWidget<UPRItemSlotWidget>(ItemSlotWidgetClass);
		}

		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		// 슬롯 목록에 유효한 슬롯을 추가하고 클릭 이벤트를 바인딩한다
		AddGeneratedItemSlot(ItemSlotWidget);
		// 슬롯을 패널 종류에 맞게 추가한다
		AddItemSlotToPanel(ItemSlotWidget, SlotIndex);
	}
}

void UPRInventoryItemListWidget::ClearGeneratedItemSlots()
{
	// 아이템 슬롯 위젯의 이벤트 제거
	for (UPRItemSlotWidget* ItemSlotWidget : ItemSlotWidgets)
	{
		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		ItemSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotLeftClicked);
		ItemSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotRightClicked);
	}

	ItemSlotWidgets.Reset();

	if (IsValid(ItemListPanel))
	{
		ItemListPanel->ClearChildren();
	}
}

void UPRInventoryItemListWidget::RebuildNativeItemList()
{
	for (int32 SlotIndex = 0; SlotIndex < ItemSlotWidgets.Num(); ++SlotIndex)
	{
		UPRItemSlotWidget* ItemSlotWidget = ItemSlotWidgets[SlotIndex];
		if (!IsValid(ItemSlotWidget))
		{
			continue;
		}

		// 슬롯 뷰 데이터 갱신
		if (Items.IsValidIndex(SlotIndex))
		{
			ItemSlotWidget->SetSlotViewData(Items[SlotIndex]);
			ItemSlotWidget->SetVisibility(ESlateVisibility::Visible);
			continue;
		}
		// 빈 뷰데이터 설정
		FPRInventoryItemSlotViewData EmptyViewData;
		ItemSlotWidget->SetSlotViewData(EmptyViewData);
		ItemSlotWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPRInventoryItemListWidget::AddItemSlotToPanel(UPRItemSlotWidget* ItemSlotWidget, int32 SlotIndex)
{
	if (!IsValid(ItemListPanel) || !IsValid(ItemSlotWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemListWidget] AddItemSlotToPanel() | 조건이 유효하지 않습니다."));
		return;
	}

	// 그리드 패널용
	if (UGridPanel* GridPanel = Cast<UGridPanel>(ItemListPanel))
	{
		const int32 ValidColumnCount = FMath::Max(1, ColumnCount);
		const int32 Row = SlotIndex / ValidColumnCount;
		const int32 Column = SlotIndex % ValidColumnCount;
		GridPanel->AddChildToGrid(ItemSlotWidget, Row, Column);
		return;
	}

	// 그 외
	ItemListPanel->AddChild(ItemSlotWidget);
}

void UPRInventoryItemListWidget::AddGeneratedItemSlot(UPRItemSlotWidget* ItemSlotWidget)
{
	if (!IsValid(ItemSlotWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryItemListWidget] AddGeneratedItemSlot() | 조건이 유효하지 않습니다."));
		return;
	}

	// 기존 할당 이벤트 삭제 후 새로 바인드
	ItemSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotLeftClicked);
	ItemSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotRightClicked);
	ItemSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotLeftClicked);
	ItemSlotWidget->OnRightClicked.AddDynamic(this, &UPRInventoryItemListWidget::HandleItemSlotRightClicked);

	ItemSlotWidgets.Add(ItemSlotWidget);
}

void UPRInventoryItemListWidget::HandleItemSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 슬롯이 비어있으면
	if (!IsValid(ViewData.ItemInstance.Get()) && ViewData.InventoryAction == EPRInventoryAction::None)
	{
		// 아무 것도 하지 않음
		return;
	}

	// 아이템 선택 이벤트를 발생시킨다
	OnItemSelected.Broadcast(ViewData);
}

void UPRInventoryItemListWidget::HandleItemSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 슬롯이 비어있으면
	if (!IsValid(ViewData.ItemInstance.Get()) && ViewData.InventoryAction == EPRInventoryAction::None)
	{
		// 아무것도 하지 않음
		return;
	}

	// 아이템 우클릭 이벤트를 발생시킨다
	OnItemRightClicked.Broadcast(ViewData);
}
