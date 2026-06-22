// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 재화 유효성 검사 및 구매 비용 레이아웃 구현)
// Author: 배유찬 (상점 UI 스택 관리 및 인벤토리 판매 목록 연동 구현)
#include "PRShopWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/Inventory/PRCurrencyDisplayWidget.h"
#include "ProjectR/UI/Shop/PRShopItemDetailWidget.h"
#include "ProjectR/UI/Shop/PRShopItemListWidget.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"

UPRShopWidget::UPRShopWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRShopWidget::SetShopContext(UPRShopComponent* InShopComponent)
{
	ShopComponent = InShopComponent;
	RefreshPlayerSources();
	OpenBuyTab();
	RefreshHeader();
}

void UPRShopWidget::OpenBuyTab()
{
	CurrentTabType = EPRShopTabType::Buy;
	SelectedItem = FPRShopItemSlotViewData();
	RefreshTabVisuals();
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::OpenSellTab()
{
	CurrentTabType = EPRShopTabType::Sell;
	SelectedItem = FPRShopItemSlotViewData();
	RefreshTabVisuals();
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::CloseShop()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	UPRUIControllerComponent* UIController = PlayerController->GetUIController();
	if (!IsValid(UIController))
	{
		return;
	}

	UIController->CloseShop();
}

/*~ UUserWidget Interface ~*/

void UPRShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindChildWidgetEvents();
	RefreshPlayerSources();
	RefreshHeader();
	RefreshTabVisuals();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::NativeDestruct()
{
	UnbindSourceEvents();
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRShopWidget::BindChildWidgetEvents()
{
	if (IsValid(BuyTabButton))
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
		BuyTabButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
	}

	if (IsValid(SellTabButton))
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
		SellTabButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
	}

	if (IsValid(BuyButton))
	{
		BuyButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
		BuyButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
	}

	if (IsValid(SellButton))
	{
		SellButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
		SellButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
	}

	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRShopWidget::HandleShopItemSelected);
		ItemListWidget->OnItemSelected.AddDynamic(this, &UPRShopWidget::HandleShopItemSelected);
	}
}

void UPRShopWidget::UnbindChildWidgetEvents()
{
	if (IsValid(BuyTabButton))
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
	}

	if (IsValid(SellTabButton))
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
	}

	if (IsValid(BuyButton))
	{
		BuyButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
	}

	if (IsValid(SellButton))
	{
		SellButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
	}

	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRShopWidget::HandleShopItemSelected);
	}
}

void UPRShopWidget::BindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRShopWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRShopWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRShopWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRShopWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (IsValid(PlayerController))
	{
		PlayerController->OnShopBuyResult.RemoveDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopBuyResult.AddDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopSellResult.RemoveDynamic(this, &UPRShopWidget::HandleShopSellResult);
		PlayerController->OnShopSellResult.AddDynamic(this, &UPRShopWidget::HandleShopSellResult);
	}
}

void UPRShopWidget::UnbindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRShopWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRShopWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (IsValid(PlayerController))
	{
		PlayerController->OnShopBuyResult.RemoveDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopSellResult.RemoveDynamic(this, &UPRShopWidget::HandleShopSellResult);
	}
}

void UPRShopWidget::RefreshPlayerSources()
{
	UnbindSourceEvents();

	InventoryComponent = nullptr;
	CurrencyComponent = nullptr;

	const APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	APRPlayerState* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<APRPlayerState>() : nullptr;
	if (IsValid(PlayerState))
	{
		InventoryComponent = PlayerState->GetInventoryComponent();
		CurrencyComponent = PlayerState->GetCurrencyComponent();
	}

	BindSourceEvents();
}

void UPRShopWidget::RefreshItemList()
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}

	const TArray<FPRShopItemSlotViewData> ListItems = CurrentTabType == EPRShopTabType::Buy
		? BuildBuyListItems()
		: BuildSellListItems();
	SyncSelectedItemFromList(ListItems);
	ItemListWidget->SetShopItemList(CurrentTabType, ListItems);
}

void UPRShopWidget::RefreshHeader()
{
	if (IsValid(ShopNameText))
	{
		ShopNameText->SetText(IsValid(ShopComponent) ? ShopComponent->GetShopDisplayName() : FText::GetEmpty());
	}

	if (IsValid(OwnedScrapText))
	{
		const int32 OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
		OwnedScrapText->SetScrapAmount(OwnedScrap);
	}
}

void UPRShopWidget::RefreshSelectedItemDetails()
{
	if (!IsValid(SelectedItemDetailWidget))
	{
		return;
	}

	const int32 OwnedStackCount = GetOwnedStackCount(SelectedItem.ItemViewData.ItemData.Get());
	SelectedItemDetailWidget->SetSelectedItemDetails(SelectedItem, CurrentTabType, OwnedStackCount);
}

void UPRShopWidget::RefreshTransactionButtons()
{
	const bool bCanRequest = CanRequestSelectedTransaction();

	if (IsValid(BuyButton))
	{
		BuyButton->SetVisibility(CurrentTabType == EPRShopTabType::Buy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		BuyButton->SetIsEnabled(bCanRequest);
	}

	if (IsValid(SellButton))
	{
		SellButton->SetVisibility(CurrentTabType == EPRShopTabType::Sell ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SellButton->SetIsEnabled(bCanRequest);
	}
}

void UPRShopWidget::RefreshTabVisuals()
{
	if (IsValid(BuyTabSelectedImage))
	{
		BuyTabSelectedImage->SetVisibility(CurrentTabType == EPRShopTabType::Buy ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(SellTabSelectedImage))
	{
		SellTabSelectedImage->SetVisibility(CurrentTabType == EPRShopTabType::Sell ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPRShopWidget::SyncSelectedItemFromList(const TArray<FPRShopItemSlotViewData>& ListItems)
{
	if (SelectedItem.EntryId.IsNone())
	{
		return;
	}

	const FName SelectedEntryId = SelectedItem.EntryId;
	const EPRShopTabType SelectedTabType = SelectedItem.TabType;
	const FPRShopItemSlotViewData* UpdatedSelectedItem = ListItems.FindByPredicate(
		[SelectedEntryId, SelectedTabType](const FPRShopItemSlotViewData& ViewData)
		{
			return ViewData.EntryId == SelectedEntryId && ViewData.TabType == SelectedTabType;
		});

	SelectedItem = UpdatedSelectedItem ? *UpdatedSelectedItem : FPRShopItemSlotViewData();
}

TArray<FPRShopItemSlotViewData> UPRShopWidget::BuildBuyListItems() const
{
	TArray<FPRShopItemSlotViewData> ListItems;
	if (!IsValid(ShopComponent))
	{
		return ListItems;
	}

	for (const FPRShopEntry& Entry : ShopComponent->GetShopEntries())
	{
		if (!Entry.bCanShopSellToPlayer)
		{
			continue;
		}

		FPRShopItemSlotViewData ViewData = BuildShopSlotViewData(Entry, EPRShopTabType::Buy);
		if (IsValid(ViewData.ItemViewData.ItemData.Get()))
		{
			ListItems.Add(ViewData);
		}
	}

	return ListItems;
}

TArray<FPRShopItemSlotViewData> UPRShopWidget::BuildSellListItems() const
{
	TArray<FPRShopItemSlotViewData> ListItems;
	if (!IsValid(ShopComponent) || !IsValid(InventoryComponent))
	{
		return ListItems;
	}

	for (const FPRShopEntry& Entry : ShopComponent->GetShopEntries())
	{
		if (!Entry.bCanPlayerSellToShop)
		{
			continue;
		}

		FPRShopItemSlotViewData ViewData = BuildShopSlotViewData(Entry, EPRShopTabType::Sell);
		UPRItemDataAsset* ItemData = ViewData.ItemViewData.ItemData.Get();
		if (!IsValid(ItemData) || GetOwnedStackCount(ItemData) <= 0)
		{
			continue;
		}

		if (!ItemData->IsA<UPRConsumableDataAsset>() && !ItemData->IsA<UPRMaterialDataAsset>())
		{
			continue;
		}

		ListItems.Add(ViewData);
	}

	return ListItems;
}

FPRShopItemSlotViewData UPRShopWidget::BuildShopSlotViewData(const FPRShopEntry& Entry, EPRShopTabType TabType) const
{
	FPRShopItemSlotViewData ViewData;
	ViewData.EntryId = Entry.EntryId;
	ViewData.TabType = TabType;
	ViewData.bUnlimitedStock = Entry.bUnlimitedStock;
	ViewData.RemainingStock = IsValid(ShopComponent) ? ShopComponent->GetRemainingStock(Entry.EntryId) : 0;

	UPRItemDataAsset* ItemData = Entry.ItemAssetId.IsValid()
		? UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Entry.ItemAssetId)
		: nullptr;
	if (!IsValid(ItemData))
	{
		return ViewData;
	}

	ViewData.ItemViewData.ItemData = ItemData;
	ViewData.ItemViewData.DisplayName = ItemData->GetDisplayName();
	ViewData.ItemViewData.Icon = ItemData->GetIcon();
	ViewData.ItemViewData.ItemType = ItemData->GetItemType();
	ViewData.ItemViewData.StackCount = GetOwnedStackCount(ItemData);
	ViewData.ItemViewData.bShowStackCount = true;
	ViewData.ItemViewData.OwnedStackCount = ViewData.ItemViewData.StackCount;
	ViewData.ItemViewData.bHasOwnedStackCount = true;
	ViewData.bSelected = SelectedItem.EntryId == Entry.EntryId && SelectedItem.TabType == TabType;
	ViewData.UnitScrapPrice = TabType == EPRShopTabType::Buy
		? Entry.BaseScrapPrice
		: IsValid(ShopComponent) ? ShopComponent->CalculateSellScrapPrice(Entry) : 0;
	ViewData.OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
	ViewData.bEnoughScrap = TabType != EPRShopTabType::Buy || ViewData.OwnedScrap >= ViewData.UnitScrapPrice;

	const bool bValidMaterialCosts = TabType != EPRShopTabType::Buy
		|| BuildMaterialCostViewData(Entry, 1, ViewData.MaterialCosts);
	if (!bValidMaterialCosts)
	{
		ViewData.MaterialCosts.Reset();
	}

	const bool bHasStock = TabType == EPRShopTabType::Sell || Entry.bUnlimitedStock || ViewData.RemainingStock > 0;
	const bool bHasBuyCost = ViewData.UnitScrapPrice > 0 || !ViewData.MaterialCosts.IsEmpty();
	ViewData.bCanRequestTransaction = bHasStock
		&& (TabType == EPRShopTabType::Sell
			? ViewData.UnitScrapPrice > 0
			: bValidMaterialCosts && bHasBuyCost);
	return ViewData;
}

bool UPRShopWidget::BuildMaterialCostViewData(const FPRShopEntry& Entry, int32 TransactionQuantity, TArray<FPRShopMaterialCostViewData>& OutMaterialCosts) const
{
	OutMaterialCosts.Reset();

	if (TransactionQuantity <= 0)
	{
		return false;
	}

	TMap<UPRMaterialDataAsset*, int32> RequiredQuantities;
	for (const FPRShopMaterialCost& MaterialCost : Entry.BuyMaterialCosts)
	{
		if (!MaterialCost.MaterialAssetId.IsValid() || MaterialCost.Quantity <= 0)
		{
			return false;
		}

		UPRItemDataAsset* ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(MaterialCost.MaterialAssetId);
		UPRMaterialDataAsset* MaterialData = Cast<UPRMaterialDataAsset>(ItemData);
		if (!IsValid(MaterialData))
		{
			return false;
		}

		int32& RequiredQuantity = RequiredQuantities.FindOrAdd(MaterialData);
		RequiredQuantity += MaterialCost.Quantity * TransactionQuantity;
	}

	for (const TPair<UPRMaterialDataAsset*, int32>& RequiredQuantity : RequiredQuantities)
	{
		FPRShopMaterialCostViewData MaterialCostViewData;
		MaterialCostViewData.MaterialData = RequiredQuantity.Key;
		MaterialCostViewData.RequiredQuantity = RequiredQuantity.Value;
		MaterialCostViewData.DisplayName = RequiredQuantity.Key->GetDisplayName();
		MaterialCostViewData.Icon = RequiredQuantity.Key->GetIcon();

		const UPRItemInstance_Material* MaterialItem = IsValid(InventoryComponent)
			? InventoryComponent->FindItemByData<UPRItemInstance_Material>(RequiredQuantity.Key)
			: nullptr;
		MaterialCostViewData.OwnedQuantity = IsValid(MaterialItem) ? MaterialItem->GetStackCount() : 0;
		MaterialCostViewData.bEnough = MaterialCostViewData.OwnedQuantity >= MaterialCostViewData.RequiredQuantity;

		OutMaterialCosts.Add(MaterialCostViewData);
	}

	return true;
}

int32 UPRShopWidget::GetOwnedStackCount(const UPRItemDataAsset* ItemData) const
{
	if (!IsValid(InventoryComponent) || !IsValid(ItemData))
	{
		return 0;
	}

	const UPRItemInstance* Item = InventoryComponent->FindItemByData(ItemData);
	return IsValid(Item) ? Item->GetStackCount() : 0;
}

bool UPRShopWidget::HasEnoughSelectedMaterialCosts() const
{
	if (!IsValid(InventoryComponent))
	{
		return SelectedItem.MaterialCosts.IsEmpty();
	}

	for (const FPRShopMaterialCostViewData& MaterialCost : SelectedItem.MaterialCosts)
	{
		if (!IsValid(MaterialCost.MaterialData.Get()) || MaterialCost.RequiredQuantity <= 0)
		{
			return false;
		}

		const UPRItemInstance_Material* MaterialItem = InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialCost.MaterialData.Get());
		const int32 OwnedQuantity = IsValid(MaterialItem) ? MaterialItem->GetStackCount() : 0;
		if (OwnedQuantity < MaterialCost.RequiredQuantity)
		{
			return false;
		}
	}

	return true;
}

bool UPRShopWidget::CanRequestSelectedTransaction() const
{
	if (!IsValid(ShopComponent) || SelectedItem.EntryId.IsNone() || !SelectedItem.bCanRequestTransaction)
	{
		return false;
	}

	if (CurrentTabType == EPRShopTabType::Buy)
	{
		const int32 OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
		return OwnedScrap >= SelectedItem.UnitScrapPrice && HasEnoughSelectedMaterialCosts();
	}

	return GetOwnedStackCount(SelectedItem.ItemViewData.ItemData.Get()) > 0;
}

void UPRShopWidget::HandleBuyTabButtonClicked()
{
	OpenBuyTab();
}

void UPRShopWidget::HandleSellTabButtonClicked()
{
	OpenSellTab();
}

void UPRShopWidget::HandleBuyButtonClicked()
{
	if (!CanRequestSelectedTransaction())
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestBuyShopItem(ShopComponent, SelectedItem.EntryId, 1);
}

void UPRShopWidget::HandleSellButtonClicked()
{
	if (!CanRequestSelectedTransaction())
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestSellShopItem(ShopComponent, SelectedItem.EntryId, 1);
}

void UPRShopWidget::HandleCloseButtonClicked()
{
	CloseShop();
}

void UPRShopWidget::HandleShopItemSelected(const FPRShopItemSlotViewData& ViewData)
{
	SelectedItem = ViewData;
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != InventoryComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleScrapChanged(int32 NewScrap)
{
	static_cast<void>(NewScrap);

	RefreshHeader();
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleShopBuyResult(const FPRShopBuyResult& Result)
{
	if (Result.ShopComponent != ShopComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshHeader();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleShopSellResult(const FPRShopSellResult& Result)
{
	if (Result.ShopComponent != ShopComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshHeader();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}
