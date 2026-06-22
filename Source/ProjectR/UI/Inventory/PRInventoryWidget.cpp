// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (보유 골드 실시간 수량 갱신 구현)
// Author: 배유찬 (플레이어 스탯 상세 정보 및 장비 장착 외형 동기화 구현)
// Author: 이건주 (캐릭터 3D 프리뷰 회전 제어 및 무기 상태 연동 구현)
#include "PRInventoryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/UI/Inventory/PRCurrencyDisplayWidget.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/UI/Inventory/PRInventoryItemListWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"
#include "ProjectR/UI/Growth/PRPlayerStatsPanelWidget.h"
#include "ProjectR/UI/Preview/PRCharacterPreviewWidget.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"

UPRInventoryWidget::UPRInventoryWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRInventoryWidget::SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent, UPREquipmentManagerComponent* InEquipmentManagerComponent)
{
	UnbindInventorySourceEvents();

	InventoryComponent = InInventoryComponent;
	WeaponManagerComponent = InWeaponManagerComponent;
	QuickSlotComponent = InQuickSlotComponent;
	EquipmentManagerComponent = InEquipmentManagerComponent;

	BindInventorySourceEvents();
	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
	RefreshPlayerStatsPanelWidget();
}

/*~ UUserWidget Interface ~*/

void UPRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// UMG BindWidgetOptional 포인터를 반복 처리 가능한 배열로 정리
	CacheChildWidgetLists();
	// 하위 슬롯 이벤트 바인드
	BindChildWidgetEvents();
	// 인벤토리 소스 이벤트 바인드
	BindInventorySourceEvents();
	// 아이템 리스트 숨기기
	CloseItemList();
	// 인벤토리 화면 갱신
	RefreshInventoryView();
	// 플레이어 성장 스탯 패널 갱신
	RefreshPlayerStatsPanelWidget();
}

void UPRInventoryWidget::NativeDestruct()
{
	// 아이템 리스트 숨기기
	CloseItemList();
	// 하위 위젯 이벤트 바인딩 정리
	UnbindChildWidgetEvents();
	// 인벤토리 소스 이벤트 바인딩 정리
	UnbindInventorySourceEvents();

	Super::NativeDestruct();
}

void UPRInventoryWidget::CacheChildWidgetLists()
{
	// UMG 슬롯 이름은 Blueprint 배치와 맞추기 위한 고정 지점
	// 실제 갱신과 이벤트 연결은 배열을 통해 같은 코드 경로로 처리
	QuickSlotWidgets.Reset();
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget0);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget1);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget2);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget3);

	// 장비 슬롯 위젯과 슬롯 타입은 같은 인덱스를 공유
	// 클릭 시 ViewData.ContextIndex에 슬롯 타입을 넣어 목록 처리 컨텍스트로 사용
	EquipmentSlotWidgets.Reset();
	EquipmentSlotTypes.Reset();

	EquipmentSlotWidgets.Add(HeadEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Head);

	EquipmentSlotWidgets.Add(BodyEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Body);

	EquipmentSlotWidgets.Add(HandsEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Hands);

	EquipmentSlotWidgets.Add(LegsEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Legs);

	EquipmentSlotWidgets.Add(AmuletEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Amulet);

	EquipmentSlotWidgets.Add(Ring1EquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Ring1);

	EquipmentSlotWidgets.Add(Ring2EquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Ring2);
}

void UPRInventoryWidget::BindChildWidgetEvents()
{
	// 이벤트 바인드 전 하위 위젯 이벤트 초기화
	UnbindChildWidgetEvents();

	// 주무기 슬롯 위젯 마우스 클릭 이벤트 바인드
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		PrimaryWeaponSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked);
		PrimaryWeaponSlotWidget->OnRightClicked.AddDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked);
	}

	// 보조무기 슬롯 위젯 마우스 클릭 이벤트 바인드
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		SecondaryWeaponSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked);
		SecondaryWeaponSlotWidget->OnRightClicked.AddDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked);
	}

	// 퀵슬롯은 슬롯 번호만 다르고 열리는 목록과 선택 처리는 동일한 구조
	for (UPRItemSlotWidget* QuickSlotWidget : QuickSlotWidgets)
	{
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlotLeftClicked);
		}
	}

	// 장비 슬롯은 슬롯 타입만 다르고 목록 구성과 장착 요청 흐름이 동일한 구조
	for (UPRItemSlotWidget* EquipmentSlotWidget : EquipmentSlotWidgets)
	{
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleEquipmentSlotLeftClicked);
		}
	}

	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleMaterialSlotLeftClicked);
	}

	// 아이템 리스트 위젯 이벤트 바인드
	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.AddDynamic(this, &UPRInventoryWidget::HandleItemListSelection);
	}
}

void UPRInventoryWidget::UnbindChildWidgetEvents()
{
	// 주무기 슬롯 이벤트 언바인드
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		PrimaryWeaponSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked);
		PrimaryWeaponSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked);
	}

	// 보조무기 슬롯 이벤트 언바인드
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		SecondaryWeaponSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked);
		SecondaryWeaponSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked);
	}

	// 배열 캐시 기준으로 연결한 슬롯 이벤트 정리
	for (UPRItemSlotWidget* QuickSlotWidget : QuickSlotWidgets)
	{
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotLeftClicked);
		}
	}

	for (UPRItemSlotWidget* EquipmentSlotWidget : EquipmentSlotWidgets)
	{
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentSlotLeftClicked);
		}
	}

	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleMaterialSlotLeftClicked);
	}

	// 아이템 리스트 이벤트 언바인드
	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRInventoryWidget::HandleItemListSelection);
	}
}

void UPRInventoryWidget::BindInventorySourceEvents()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController) || !OwningPlayerController->IsLocalController())
	{
		return;
	}
	
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
	}

	if (IsValid(WeaponManagerComponent))
	{
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().AddDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
		QuickSlotComponent->GetOnQuickSlotChanged().AddDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
	}

	if (IsValid(EquipmentManagerComponent))
	{
		EquipmentManagerComponent->OnEquipmentChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
		EquipmentManagerComponent->OnEquipmentChanged.AddDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
		EquipmentManagerComponent->OnEquipmentVisualInfosChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentVisualInfosChanged);
		EquipmentManagerComponent->OnEquipmentVisualInfosChanged.AddDynamic(this, &UPRInventoryWidget::HandleEquipmentVisualInfosChanged);
	}

	CurrencyComponent = ResolveCurrencyComponent();
	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
	}
}

void UPRInventoryWidget::UnbindInventorySourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
	}

	if (IsValid(WeaponManagerComponent))
	{
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
	}

	if (IsValid(EquipmentManagerComponent))
	{
		EquipmentManagerComponent->OnEquipmentChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
		EquipmentManagerComponent->OnEquipmentVisualInfosChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentVisualInfosChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
	}

	CurrencyComponent = nullptr;
}

//
void UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::PrimaryWeapon) && PendingWeaponListSlot == EPRWeaponSlotType::Primary)
	{
		CloseItemList();
		return;
	}

	// 주무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Primary);
}

void UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Mod) && LastFocusedItem == ViewData.ItemInstance)
	{
		CloseItemList();
		return;
	}
	
	OpenModList( ViewData.ItemInstance);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::SecondaryWeapon) && PendingWeaponListSlot == EPRWeaponSlotType::Secondary)
	{
		CloseItemList();
		return;
	}

	// 보조무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Secondary);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 보조무기 슬롯 위젯 우클릭 시 장착 가능한 모드 리스트 열기
	UPRItemInstance_Weapon* WeaponItem = IsValid(WeaponManagerComponent)
		? WeaponManagerComponent->GetWeaponInstanceBySlotType(EPRWeaponSlotType::Secondary)
		: nullptr;

	if (IsItemListOpenAs(EPRItemType::Mod) && LastFocusedItem == WeaponItem)
	{
		CloseItemList();
		return;
	}

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleQuickSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// QuickSlotWidgets 배열 갱신에서 넣은 슬롯 번호
	const int32 SlotIndex = ViewData.ContextIndex;
	if (SlotIndex == INDEX_NONE)
	{
		return;
	}

	if (IsItemListOpenAs(EPRItemType::Consumable) && LastFocusedIndex == SlotIndex)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(SlotIndex);
}

void UPRInventoryWidget::HandleEquipmentSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 장비 슬롯은 enum 값을 숫자 컨텍스트로 보관
	const EPREquipmentSlotType SlotType = static_cast<EPREquipmentSlotType>(ViewData.ContextIndex);
	if (SlotType == EPREquipmentSlotType::None)
	{
		return;
	}

	if (IsItemListOpenAs(EPRItemType::Equipment) && LastFocusedIndex == ViewData.ContextIndex)
	{
		CloseItemList();
		return;
	}

	OpenEquipmentListForSlot(SlotType);
}

void UPRInventoryWidget::HandleMaterialSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Material))
	{
		CloseItemList();
		return;
	}

	OpenMaterialList();
}

void UPRInventoryWidget::HandleItemListSelection(const FPRInventoryItemSlotViewData& ViewData)
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}
	
	FPRItemActivationContext ActivationContext;
	ActivationContext.InventoryComponent = InventoryComponent;
	ActivationContext.UserActor = GetOwningPlayer();
	ActivationContext.ContextObject = LastFocusedItem;
	ActivationContext.ContextIndex = LastFocusedIndex;
	
	// 리스트 선택은 여기서 ActivateItem/DeactivateItem 요청으로만 변환함
	// 무기 장착, Mod 장착, 퀵슬롯 등록의 실제 처리는 각 ItemInstance 구현에서 나뉨
	if (ViewData.InventoryAction == EPRInventoryAction::Activate)
	{
		if (IsValid(InventoryComponent) && IsValid(ViewData.ItemInstance.Get()))
		{
			// Widget 외부 공통 활성화 경로
			InventoryComponent->RequestActivateItem(ViewData.ItemInstance.Get(), ActivationContext);
		}
	}
	else if (ViewData.InventoryAction == EPRInventoryAction::Deactivate)
	{
		if (IsValid(InventoryComponent) && IsValid(ViewData.ItemInstance.Get()))
		{
			// Widget 외부 공통 비활성화 경로
			InventoryComponent->RequestDeactivateItem(ViewData.ItemInstance.Get(), ActivationContext);
		}
	}

	CloseItemList();
}

void UPRInventoryWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent,
	const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != InventoryComponent)
	{
		return;
	}

	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	static_cast<void>(ChangedSlot);

	if (ChangedWeaponManagerComponent != WeaponManagerComponent)
	{
		return;
	}

	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex)
{
	if (ChangedQuickSlotComponent != QuickSlotComponent)
	{
		return;
	}

	RefreshQuickSlotWidgets();
}

void UPRInventoryWidget::HandleEquipmentChanged(EPREquipmentSlotType ChangedSlot, UPRItemInstance_Equipment* EquipmentItem)
{
	// 장비 변경은 슬롯 표시, 열린 장비 목록의 장착 표시, 캐릭터 프리뷰가 함께 변하는 외부 상태
	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::HandleEquipmentVisualInfosChanged(UPREquipmentManagerComponent* ChangedEquipmentManagerComponent)
{
	if (ChangedEquipmentManagerComponent != EquipmentManagerComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 같은 외형 이벤트를 플레이어 캐릭터와 인벤토리 위젯이 함께 수신하는 구조
	// 델리게이트 실행 순서와 무관하게 캐릭터 파츠 메시 갱신 이후 프리뷰를 복사하기 위한 다음 틱 예약
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::RefreshCharacterPreviewWidget));
}

void UPRInventoryWidget::HandleScrapChanged(int32 NewScrap)
{
	RefreshCurrencyText();
}

void UPRInventoryWidget::OpenWeaponList(EPRWeaponSlotType TargetSlot)
{
	// 유효성 확인. 아이템 리스트 위젯, 인벤토리 컴포넌트, 무기매니저 컴포넌트
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(WeaponManagerComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] OpenWeaponList() 실행 조건이 유효하지 않습니다."));
		return;
	}

	// 무기 리스트 팝업 위젯에 띄울 아이템의 무기 슬롯
	PendingWeaponListSlot = TargetSlot;

	EPRItemType ItemListType = EPRItemType::Weapon;
	if (PendingWeaponListSlot == EPRWeaponSlotType::Primary)
	{
		ItemListType = EPRItemType::PrimaryWeapon;
	}
	else if (PendingWeaponListSlot == EPRWeaponSlotType::Secondary)
	{
		ItemListType = EPRItemType::SecondaryWeapon;
	}

	// Mod 장착 타겟 무기 Item
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Weapon* EquippedWeaponItem = WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot);
	if (IsValid(EquippedWeaponItem))
	{
		// 실제 장착 ItemInstance를 유지하는 비활성화 명령 항목
		ListItems.Add(BuildDeactivateActionViewData(ItemListType, EquippedWeaponItem));
	}

	for (UPRItemInstance_Weapon* WeaponItem : InventoryComponent->GetItemsByType<UPRItemInstance_Weapon>(EPRItemType::Weapon))
	{
		if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
		{
			continue;
		}

		if (WeaponItem->GetWeaponData()->SlotType != TargetSlot)
		{
			continue;
		}

		const bool bEquipped = WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot) == WeaponItem;
		ListItems.Add(BuildWeaponItemViewData(WeaponItem, bEquipped));
	}

	PushItemList(ItemListType, ListItems);
}

void UPRInventoryWidget::OpenModList(UPRItemInstance* TargetItem)
{
	UPRItemInstance_Weapon* TargetWeaponItem = Cast<UPRItemInstance_Weapon>(TargetItem);
	
	// 아이템 리스트 위젯, 인벤토리 컴포넌트, 장착 타겟 무기 아이템 유효성 확인
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(TargetWeaponItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 모드 리스트 열기 실패. OpenModList()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = TargetWeaponItem;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Mod* EquippedModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(EquippedModItem))
	{
		// 실제 장착 Mod ItemInstance를 유지하는 비활성화 명령 항목
		ListItems.Add(BuildDeactivateActionViewData(EPRItemType::Mod, EquippedModItem));
	}

	for (UPRItemInstance_Mod* ModItem : InventoryComponent->GetItemsByType<UPRItemInstance_Mod>(EPRItemType::Mod))
	{
		if (!IsModCompatibleWithWeapon(ModItem, TargetWeaponItem))
		{
			continue;
		}

		const bool bEquipped = ModItem->GetEquippedWeaponItem() == TargetWeaponItem;
		ListItems.Add(BuildModItemViewData(ModItem, bEquipped));
	}

	PushItemList(EPRItemType::Mod, ListItems);
}

void UPRInventoryWidget::OpenConsumableListForQuickSlot(int32 SlotIndex)
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(QuickSlotComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 소비 아이템 리스트 열기 실패. OpenConsumableListForQuickSlot()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = nullptr;
	// 소비 아이템 선택 결과를 등록할 퀵슬롯 번호를 숫자 컨텍스트로 보관
	LastFocusedIndex = SlotIndex;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Consumable* ConsumableItem : InventoryComponent->GetItemsByType<UPRItemInstance_Consumable>(EPRItemType::Consumable))
	{
		if (!IsValid(ConsumableItem))
		{
			continue;
		}

		ListItems.Add(BuildConsumableItemViewData(ConsumableItem));
	}

	PushItemList(EPRItemType::Consumable, ListItems);
}

void UPRInventoryWidget::OpenEquipmentListForSlot(EPREquipmentSlotType SlotType)
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(EquipmentManagerComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 장비 아이템 리스트 열기 실패. OpenEquipmentListForSlot()"));
		return;
	}

	if (SlotType == EPREquipmentSlotType::None)
	{
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = nullptr;
	// 장비 선택 결과는 ItemInstance의 SlotType으로 처리되지만 목록 재구성에는 열린 슬롯 타입이 필요
	LastFocusedIndex = static_cast<int32>(SlotType);

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Equipment* EquippedItem = EquipmentManagerComponent->GetEquippedItem(SlotType);
	if (IsValid(EquippedItem))
	{
		// 장비 해제 항목도 실제 장착 ItemInstance를 들고 공통 Deactivate 요청으로 이동
		ListItems.Add(BuildDeactivateActionViewData(EPRItemType::Equipment, EquippedItem));
	}

	for (UPRItemInstance_Equipment* EquipmentItem : InventoryComponent->GetItemsByType<UPRItemInstance_Equipment>(EPRItemType::Equipment))
	{
		if (!IsValid(EquipmentItem) || !IsValid(EquipmentItem->GetEquipmentData()))
		{
			continue;
		}

		if (EquipmentItem->GetSlotType() != SlotType)
		{
			continue;
		}

		const bool bEquipped = EquippedItem == EquipmentItem;
		ListItems.Add(BuildEquipmentItemViewData(EquipmentItem, bEquipped));
	}

	PushItemList(EPRItemType::Equipment, ListItems);
}

void UPRInventoryWidget::OpenMaterialList()
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 재료 아이템 리스트 열기 실패. OpenMaterialList()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Material* MaterialItem : InventoryComponent->GetItemsByType<UPRItemInstance_Material>(EPRItemType::Material))
	{
		if (!IsValid(MaterialItem))
		{
			continue;
		}

		ListItems.Add(BuildMaterialItemViewData(MaterialItem));
	}

	PushItemList(EPRItemType::Material, ListItems);
}

bool UPRInventoryWidget::IsItemListOpenAs(EPRItemType ListType) const
{
	return IsValid(ItemListWidget) && ItemListWidget->IsVisible() && ItemListWidget->GetListType() == ListType;
}

void UPRInventoryWidget::CloseItemList()
{
	if (IsValid(ItemListWidget))
	{
		TArray<FPRInventoryItemSlotViewData> EmptyItems;
		ItemListWidget->SetItemList(EPRItemType::None, EmptyItems);

		if (UPRUIManagerSubsystem* UIManager = ResolveUIManager())
		{
			// ItemListWidget이 스택 최상위가 아니어도 지정 인스턴스만 Pop
			UIManager->PopUI(ItemListWidget);
		}
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;
}

void UPRInventoryWidget::PushItemList(EPRItemType ListType, const TArray<FPRInventoryItemSlotViewData>& ListItems)
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}

	ItemListWidget->SetItemList(ListType, ListItems);

	if (UPRUIManagerSubsystem* UIManager = ResolveUIManager())
	{
		// 부모 위젯 내부에 배치된 리스트도 UI 스택에서 입력 포커스를 받도록 Push
		UIManager->PushUIInstance(ItemListWidget);
	}
}

void UPRInventoryWidget::RefreshEquippedSlotWidgets()
{
	// 주무기 슬롯 위젯이 유효하면
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		//주무기 슬롯 갱신
		PrimaryWeaponSlotWidget->SetSlotViewData(BuildWeaponSlotViewData(EPRWeaponSlotType::Primary));
	}

	// 보조무기 슬롯 위젯이 유효하면
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		// 보조무기 슬롯 갱신
		SecondaryWeaponSlotWidget->SetSlotViewData(BuildWeaponSlotViewData(EPRWeaponSlotType::Secondary));
	}
}

void UPRInventoryWidget::RefreshQuickSlotWidgets()
{
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotWidgets.Num(); ++SlotIndex)
	{
		UPRItemSlotWidget* QuickSlotWidget = QuickSlotWidgets[SlotIndex];
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->SetSlotViewData(BuildQuickSlotViewData(SlotIndex));
		}
	}
}

void UPRInventoryWidget::RefreshEquipmentSlotWidgets()
{
	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlotWidgets.Num(); ++SlotIndex)
	{
		if (!EquipmentSlotTypes.IsValidIndex(SlotIndex))
		{
			continue;
		}

		UPRItemSlotWidget* EquipmentSlotWidget = EquipmentSlotWidgets[SlotIndex];
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->SetSlotViewData(BuildEquipmentSlotViewData(EquipmentSlotTypes[SlotIndex]));
		}
	}
}

void UPRInventoryWidget::RefreshMaterialSlotWidget()
{
	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->SetSlotViewData(BuildMaterialSlotViewData());
	}
}

void UPRInventoryWidget::RefreshCurrencyText()
{
	if (!IsValid(ScrapDisplayWidget) && !IsValid(ScrapAmountText))
	{
		return;
	}

	if (!IsValid(CurrencyComponent))
	{
		CurrencyComponent = ResolveCurrencyComponent();
	}

	const int32 ScrapAmount = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
	if (IsValid(ScrapDisplayWidget))
	{
		ScrapDisplayWidget->SetScrapAmount(ScrapAmount);
	}
	else if (IsValid(ScrapAmountText))
	{
		ScrapAmountText->SetText(FText::AsNumber(ScrapAmount));
	}
}

void UPRInventoryWidget::RefreshInventoryView()
{
	// 장착 슬롯은 리스트 표시 여부와 관계없이 항상 최신 상태로 맞춘다
	RefreshEquippedSlotWidgets();
	RefreshQuickSlotWidgets();
	RefreshEquipmentSlotWidgets();
	RefreshMaterialSlotWidget();
	RefreshCurrencyText();

	if (!IsValid(ItemListWidget) || !ItemListWidget->IsVisible())
	{
		return;
	}

	const EPRItemType CurrentListType = ItemListWidget->GetListType();
	if (CurrentListType == EPRItemType::Weapon
		|| CurrentListType == EPRItemType::PrimaryWeapon
		|| CurrentListType == EPRItemType::SecondaryWeapon)
	{
		if (PendingWeaponListSlot != EPRWeaponSlotType::None)
		{
			OpenWeaponList(PendingWeaponListSlot);
		}
	}
	else if (CurrentListType == EPRItemType::Mod && IsValid(LastFocusedItem))
	{
		OpenModList(LastFocusedItem);
	}
	else if (CurrentListType == EPRItemType::Consumable && LastFocusedIndex != INDEX_NONE)
	{
		OpenConsumableListForQuickSlot(LastFocusedIndex);
	}
	else if (CurrentListType == EPRItemType::Equipment && LastFocusedIndex != INDEX_NONE)
	{
		OpenEquipmentListForSlot(static_cast<EPREquipmentSlotType>(LastFocusedIndex));
	}
	else if (CurrentListType == EPRItemType::Material)
	{
		OpenMaterialList();
	}
}

void UPRInventoryWidget::RefreshCharacterPreviewWidget()
{
	if (!IsValid(CharacterPreviewWidget))
	{
		return;
	}

	APRPlayerCharacter* SourceCharacter = GetPreviewSourceCharacter();
	CharacterPreviewWidget->SetPreviewSources(SourceCharacter, WeaponManagerComponent);
}

void UPRInventoryWidget::RefreshPlayerStatsPanelWidget()
{
	if (!IsValid(PlayerStatsPanelWidget))
	{
		return;
	}

	APRPlayerState* PlayerState = IsValid(InventoryComponent) ? Cast<APRPlayerState>(InventoryComponent->GetOwner()) : nullptr;
	PlayerStatsPanelWidget->SetPlayerStateSource(PlayerState);
}

APRPlayerCharacter* UPRInventoryWidget::GetPreviewSourceCharacter() const
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController) || !OwningPlayerController->IsLocalController())
	{
		return nullptr;
	}

	return Cast<APRPlayerCharacter>(OwningPlayerController->GetPawn());
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildWeaponSlotViewData(EPRWeaponSlotType SlotType) const
{
	if (IsValid(WeaponManagerComponent))
	{
		if (UPRItemInstance_Weapon* WeaponItem = WeaponManagerComponent->GetWeaponInstanceBySlotType(SlotType))
		{
			return BuildWeaponItemViewData(WeaponItem, true);
		}
	}

	return UPRInventoryItemSlotViewDataBuilder::BuildEmptyWeaponSlotViewData(SlotType);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildWeaponItemViewData(WeaponItem, bEquipped);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildModItemViewData(ModItem, bEquipped);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildConsumableItemViewData(UPRItemInstance_Consumable* ConsumableItem) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildConsumableItemViewData(ConsumableItem);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildMaterialSlotViewData() const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildMaterialSlotViewData();
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildMaterialItemViewData(UPRItemInstance_Material* MaterialItem) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildMaterialItemViewData(MaterialItem);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildQuickSlotViewData(int32 SlotIndex) const
{
	if (!IsValid(QuickSlotComponent))
	{
		return UPRInventoryItemSlotViewDataBuilder::BuildEmptyQuickSlotViewData(SlotIndex);
	}

	const FPRQuickSlotViewData QuickSlotViewData = QuickSlotComponent->BuildQuickSlotViewData(SlotIndex);
	return UPRInventoryItemSlotViewDataBuilder::BuildQuickSlotViewData(QuickSlotViewData);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildEquipmentSlotViewData(EPREquipmentSlotType SlotType) const
{
	if (IsValid(EquipmentManagerComponent))
	{
		if (UPRItemInstance_Equipment* EquipmentItem = EquipmentManagerComponent->GetEquippedItem(SlotType))
		{
			FPRInventoryItemSlotViewData EquippedViewData = BuildEquipmentItemViewData(EquipmentItem, true);
			EquippedViewData.ContextIndex = static_cast<int32>(SlotType);
			return EquippedViewData;
		}
	}

	return UPRInventoryItemSlotViewDataBuilder::BuildEmptyEquipmentSlotViewData(SlotType);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildEquipmentItemViewData(UPRItemInstance_Equipment* EquipmentItem, bool bEquipped) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildEquipmentItemViewData(EquipmentItem, bEquipped);
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildDeactivateActionViewData(EPRItemType ListType, UPRItemInstance* TargetItem) const
{
	return UPRInventoryItemSlotViewDataBuilder::BuildDeactivateActionViewData(ListType, TargetItem);
}

FText UPRInventoryWidget::GetEquipmentSlotDisplayName(EPREquipmentSlotType SlotType) const
{
	return UPRInventoryItemSlotViewDataBuilder::GetEquipmentSlotDisplayName(SlotType);
}

bool UPRInventoryWidget::IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem) const
{
	return UPRInventoryItemSlotViewDataBuilder::IsModCompatibleWithWeapon(ModItem, WeaponItem);
}

UPRCurrencyComponent* UPRInventoryWidget::ResolveCurrencyComponent() const
{
	if (!IsValid(InventoryComponent))
	{
		return nullptr;
	}

	const APRPlayerState* PlayerState = Cast<APRPlayerState>(InventoryComponent->GetOwner());
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	return PlayerState->GetCurrencyComponent();
}

UPRUIManagerSubsystem* UPRInventoryWidget::ResolveUIManager() const
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UPRUIManagerSubsystem>();
}
