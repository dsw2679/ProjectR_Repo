// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 거래 비용/재료 유효성 검사 구현)
// Author: 배유찬 (장비 및 소모품 거래 관련 상점 기초 로직 구현)
#include "PRShopComponent.h"

#include "Net/UnrealNetwork.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Shop/Data/PRShopDataAsset.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"

UPRShopComponent::UPRShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

/*~ UActorComponent Interface ~*/

void UPRShopComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		InitializeRuntimeStocks();
	}
}

void UPRShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRShopComponent, RuntimeStocks);
}

/*~ 거래 요청 ~*/

FPRShopBuyResult UPRShopComponent::RequestBuyItem(APRPlayerController* RequestingController, FName EntryId, int32 Quantity)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidAuthority);
	}

	if (!CanRequestShop(RequestingController))
	{
		return MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidBuyer);
	}

	APRPlayerState* PlayerState = ResolvePlayerState(RequestingController);
	if (!IsValid(PlayerState))
	{
		return MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidBuyer);
	}

	if (!BeginShopTransaction(PlayerState))
	{
		return MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::RequestThrottled);
	}

	FPRShopBuyResult Result;
	const FPRShopEntry* Entry = FindEntry(EntryId);
	UPRItemDataAsset* ItemData = Entry ? ResolveItemData(*Entry) : nullptr;
	UPRInventoryComponent* InventoryComponent = ResolveInventoryComponent(RequestingController);
	UPRCurrencyComponent* CurrencyComponent = ResolveCurrencyComponent(RequestingController);
	TMap<UPRMaterialDataAsset*, int32> MaterialCosts;

	if (!Entry)
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidEntry);
	}
	else if (!Entry->bCanShopSellToPlayer)
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::NotForSale);
	}
	else if (!IsValid(ItemData))
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::MissingItemData);
	}
	else if (Quantity <= 0 || Entry->Quantity <= 0)
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidEntry);
	}
	else if (!HasEnoughStock(*Entry, Quantity))
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::OutOfStock);
	}
	else if (!IsValid(InventoryComponent) || !IsValid(CurrencyComponent))
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InvalidBuyer);
	}
	else if (!CanGrantItem(InventoryComponent, ItemData, Entry->Quantity * Quantity))
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::InventoryFull);
	}
	else if (!ResolveBuyMaterialCosts(*Entry, Quantity, MaterialCosts))
	{
		Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::MissingItemData);
	}
	else
	{
		const int32 TotalScrapPrice = Entry->BaseScrapPrice * Quantity;
		const int32 TotalItemQuantity = Entry->Quantity * Quantity;
		if (CurrencyComponent->GetScrap() < TotalScrapPrice)
		{
			Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::NotEnoughScrap);
		}
		else if (!HasEnoughBuyMaterialCosts(InventoryComponent, MaterialCosts))
		{
			Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::NotEnoughMaterial);
		}
		else if (TotalScrapPrice > 0 && !CurrencyComponent->TrySpendScrap(TotalScrapPrice))
		{
			Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::ConsumeFailed);
		}
		else
		{
			TArray<TPair<UPRMaterialDataAsset*, int32>> ConsumedMaterials;
			if (!ConsumeBuyMaterialCosts(InventoryComponent, MaterialCosts, ConsumedMaterials))
			{
				if (TotalScrapPrice > 0)
				{
					CurrencyComponent->AddScrap(TotalScrapPrice);
				}

				Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::ConsumeFailed);
			}
			else if (!IsValid(InventoryComponent->AddItem(ItemData, TotalItemQuantity)))
			{
				if (TotalScrapPrice > 0)
				{
					CurrencyComponent->AddScrap(TotalScrapPrice);
				}

				RefundBuyMaterialCosts(InventoryComponent, ConsumedMaterials);
				Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::GrantFailed);
			}
			else if (!ConsumeStock(*Entry, Quantity))
			{
				if (TotalScrapPrice > 0)
				{
					CurrencyComponent->AddScrap(TotalScrapPrice);
				}

				RefundBuyMaterialCosts(InventoryComponent, ConsumedMaterials);
				Result = MakeBuyFailureResult(RequestingController, EntryId, Quantity, EPRShopBuyFailReason::OutOfStock);
			}
			else
			{
				Result = MakeBuySuccessResult(RequestingController, EntryId, Quantity);
			}
		}
	}

	EndShopTransaction(PlayerState);
	return Result;
}

FPRShopSellResult UPRShopComponent::RequestSellItem(APRPlayerController* RequestingController, FName EntryId, int32 Quantity)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidAuthority);
	}

	if (!CanRequestShop(RequestingController))
	{
		return MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidSeller);
	}

	APRPlayerState* PlayerState = ResolvePlayerState(RequestingController);
	if (!IsValid(PlayerState))
	{
		return MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidSeller);
	}

	if (!BeginShopTransaction(PlayerState))
	{
		return MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::RequestThrottled);
	}

	FPRShopSellResult Result;
	const FPRShopEntry* Entry = FindEntry(EntryId);
	UPRItemDataAsset* ItemData = Entry ? ResolveItemData(*Entry) : nullptr;
	UPRInventoryComponent* InventoryComponent = ResolveInventoryComponent(RequestingController);
	UPRCurrencyComponent* CurrencyComponent = ResolveCurrencyComponent(RequestingController);

	if (!Entry)
	{
		Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidEntry);
	}
	else if (!Entry->bCanPlayerSellToShop)
	{
		Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::NotBuyableByShop);
	}
	else if (!IsValid(ItemData))
	{
		Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::MissingItemData);
	}
	else if (Quantity <= 0 || Entry->Quantity <= 0)
	{
		Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidQuantity);
	}
	else if (!IsValid(InventoryComponent) || !IsValid(CurrencyComponent))
	{
		Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::InvalidSeller);
	}
	else
	{
		const int32 TotalItemQuantity = Entry->Quantity * Quantity;
		const int32 TotalScrapReward = CalculateSellScrapPrice(*Entry) * Quantity;
		if (TotalScrapReward <= 0)
		{
			Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::MissingPriceData);
		}
		else if (UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(ItemData))
		{
			UPRItemInstance_Consumable* ConsumableItem = InventoryComponent->FindItemByData<UPRItemInstance_Consumable>(ConsumableData);
			if (!IsValid(ConsumableItem) || ConsumableItem->GetStackCount() < TotalItemQuantity)
			{
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::NotEnoughItem);
			}
			else if (!InventoryComponent->RemoveItemByData(ConsumableData, TotalItemQuantity))
			{
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::RemoveFailed);
			}
			else if (!CurrencyComponent->AddScrap(TotalScrapReward))
			{
				InventoryComponent->AddItem(ConsumableData, TotalItemQuantity);
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::GrantScrapFailed);
			}
			else
			{
				Result = MakeSellSuccessResult(RequestingController, EntryId, Quantity);
			}
		}
		else if (UPRMaterialDataAsset* MaterialData = Cast<UPRMaterialDataAsset>(ItemData))
		{
			UPRItemInstance_Material* MaterialItem = InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialData);
			if (!IsValid(MaterialItem) || MaterialItem->GetStackCount() < TotalItemQuantity)
			{
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::NotEnoughItem);
			}
			else if (!InventoryComponent->RemoveItemByData(MaterialData, TotalItemQuantity))
			{
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::RemoveFailed);
			}
			else if (!CurrencyComponent->AddScrap(TotalScrapReward))
			{
				InventoryComponent->AddItem(MaterialData, TotalItemQuantity);
				Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::GrantScrapFailed);
			}
			else
			{
				Result = MakeSellSuccessResult(RequestingController, EntryId, Quantity);
			}
		}
		else
		{
			Result = MakeSellFailureResult(RequestingController, EntryId, Quantity, EPRShopSellFailReason::UnsupportedItemType);
		}
	}

	EndShopTransaction(PlayerState);
	return Result;
}

/*~ 조회 ~*/

FText UPRShopComponent::GetShopDisplayName() const
{
	return IsValid(ShopData) ? ShopData->DisplayName : FText::GetEmpty();
}

const TArray<FPRShopEntry>& UPRShopComponent::GetShopEntries() const
{
	static const TArray<FPRShopEntry> EmptyEntries;
	return IsValid(ShopData) ? ShopData->Entries : EmptyEntries;
}

int32 UPRShopComponent::GetRemainingStock(FName EntryId) const
{
	const FPRShopEntry* Entry = FindEntry(EntryId);
	if (!Entry)
	{
		return 0;
	}

	if (Entry->bUnlimitedStock)
	{
		return INDEX_NONE;
	}

	const FPRShopRuntimeStock* RuntimeStock = FindRuntimeStock(EntryId);
	return RuntimeStock ? RuntimeStock->RemainingStock : 0;
}

bool UPRShopComponent::IsUnlimitedStock(FName EntryId) const
{
	const FPRShopEntry* Entry = FindEntry(EntryId);
	return Entry ? Entry->bUnlimitedStock : false;
}

int32 UPRShopComponent::CalculateSellScrapPrice(const FPRShopEntry& Entry) const
{
	if (!IsValid(ShopData) || Entry.BaseScrapPrice <= 0)
	{
		return 0;
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(Entry.BaseScrapPrice) * ShopData->PlayerSellPriceRate));
}

const FPRShopEntry* UPRShopComponent::FindEntry(FName EntryId) const
{
	if (!IsValid(ShopData) || EntryId.IsNone())
	{
		return nullptr;
	}

	return ShopData->Entries.FindByPredicate([EntryId](const FPRShopEntry& Entry)
	{
		return Entry.EntryId == EntryId;
	});
}

UPRItemDataAsset* UPRShopComponent::ResolveItemData(const FPRShopEntry& Entry) const
{
	if (!Entry.ItemAssetId.IsValid())
	{
		return nullptr;
	}

	return UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Entry.ItemAssetId);
}

bool UPRShopComponent::CanRequestShop(APRPlayerController* RequestingController) const
{
	if (!IsValid(RequestingController) || !IsValid(GetOwner()))
	{
		return false;
	}

	APawn* RequestingPawn = RequestingController->GetPawn();
	if (!IsValid(RequestingPawn))
	{
		return false;
	}

	const float DistanceSq = FVector::DistSquared(RequestingPawn->GetActorLocation(), GetOwner()->GetActorLocation());
	return DistanceSq <= FMath::Square(RequestInteractionDistance);
}

APRPlayerState* UPRShopComponent::ResolvePlayerState(APRPlayerController* RequestingController) const
{
	return IsValid(RequestingController) ? RequestingController->GetPlayerState<APRPlayerState>() : nullptr;
}

UPRInventoryComponent* UPRShopComponent::ResolveInventoryComponent(APRPlayerController* RequestingController) const
{
	APRPlayerState* PlayerState = ResolvePlayerState(RequestingController);
	return IsValid(PlayerState) ? PlayerState->GetInventoryComponent() : nullptr;
}

UPRCurrencyComponent* UPRShopComponent::ResolveCurrencyComponent(APRPlayerController* RequestingController) const
{
	APRPlayerState* PlayerState = ResolvePlayerState(RequestingController);
	return IsValid(PlayerState) ? PlayerState->GetCurrencyComponent() : nullptr;
}

/*~ 거래 방어 ~*/

bool UPRShopComponent::BeginShopTransaction(APRPlayerState* Trader)
{
	if (!IsValid(Trader))
	{
		return false;
	}

	UWorld* World = GetWorld();
	const double CurrentServerTime = IsValid(World) ? World->GetTimeSeconds() : 0.0;
	const TWeakObjectPtr<APRPlayerState> TraderKey = Trader;
	const double* NextAllowedTime = NextAllowedShopRequestServerTimes.Find(TraderKey);
	if (TradersInTransaction.Contains(TraderKey) || (NextAllowedTime && CurrentServerTime < *NextAllowedTime))
	{
		return false;
	}

	TradersInTransaction.Add(TraderKey);
	NextAllowedShopRequestServerTimes.Add(TraderKey, CurrentServerTime + ShopRequestMinInterval);
	return true;
}

void UPRShopComponent::EndShopTransaction(APRPlayerState* Trader)
{
	if (!IsValid(Trader))
	{
		return;
	}

	TradersInTransaction.Remove(Trader);
}

/*~ 결과 생성 ~*/

FPRShopBuyResult UPRShopComponent::MakeBuyFailureResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity, EPRShopBuyFailReason FailReason)
{
	FPRShopBuyResult Result;
	Result.bSuccess = false;
	Result.FailReason = FailReason;
	Result.ShopComponent = this;
	Result.EntryId = EntryId;
	Result.Quantity = Quantity;

	NotifyBuyResult(RequestingController, Result);
	return Result;
}

FPRShopBuyResult UPRShopComponent::MakeBuySuccessResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity)
{
	FPRShopBuyResult Result;
	Result.bSuccess = true;
	Result.FailReason = EPRShopBuyFailReason::None;
	Result.ShopComponent = this;
	Result.EntryId = EntryId;
	Result.Quantity = Quantity;

	NotifyBuyResult(RequestingController, Result);
	return Result;
}

FPRShopSellResult UPRShopComponent::MakeSellFailureResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity, EPRShopSellFailReason FailReason)
{
	FPRShopSellResult Result;
	Result.bSuccess = false;
	Result.FailReason = FailReason;
	Result.ShopComponent = this;
	Result.EntryId = EntryId;
	Result.Quantity = Quantity;

	NotifySellResult(RequestingController, Result);
	return Result;
}

FPRShopSellResult UPRShopComponent::MakeSellSuccessResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity)
{
	FPRShopSellResult Result;
	Result.bSuccess = true;
	Result.FailReason = EPRShopSellFailReason::None;
	Result.ShopComponent = this;
	Result.EntryId = EntryId;
	Result.Quantity = Quantity;

	NotifySellResult(RequestingController, Result);
	return Result;
}

/*~ 재고 ~*/

void UPRShopComponent::InitializeRuntimeStocks()
{
	RuntimeStocks.Reset();

	if (!IsValid(ShopData))
	{
		return;
	}

	for (const FPRShopEntry& Entry : ShopData->Entries)
	{
		if (Entry.bUnlimitedStock || Entry.EntryId.IsNone())
		{
			continue;
		}

		FPRShopRuntimeStock RuntimeStock;
		RuntimeStock.EntryId = Entry.EntryId;
		RuntimeStock.RemainingStock = FMath::Max(Entry.MaxStock, 0);
		RuntimeStocks.Add(RuntimeStock);
	}
}

FPRShopRuntimeStock* UPRShopComponent::FindRuntimeStock(FName EntryId)
{
	return RuntimeStocks.FindByPredicate([EntryId](const FPRShopRuntimeStock& RuntimeStock)
	{
		return RuntimeStock.EntryId == EntryId;
	});
}

const FPRShopRuntimeStock* UPRShopComponent::FindRuntimeStock(FName EntryId) const
{
	return RuntimeStocks.FindByPredicate([EntryId](const FPRShopRuntimeStock& RuntimeStock)
	{
		return RuntimeStock.EntryId == EntryId;
	});
}

bool UPRShopComponent::HasEnoughStock(const FPRShopEntry& Entry, int32 Quantity) const
{
	if (Entry.bUnlimitedStock)
	{
		return true;
	}

	const FPRShopRuntimeStock* RuntimeStock = FindRuntimeStock(Entry.EntryId);
	return RuntimeStock && RuntimeStock->RemainingStock >= Quantity;
}

bool UPRShopComponent::CanGrantItem(UPRInventoryComponent* InventoryComponent, UPRItemDataAsset* ItemData, int32 TotalItemQuantity) const
{
	if (!IsValid(InventoryComponent) || !IsValid(ItemData) || TotalItemQuantity <= 0)
	{
		return false;
	}
	
	const UPRItemInstance* Item = InventoryComponent->FindItemByData(ItemData);
	const int32 CurrentStackCount = IsValid(Item) ? Item->GetStackCount() : 0;
	return CurrentStackCount + TotalItemQuantity <= ItemData->MaxStackCount;
}

bool UPRShopComponent::ResolveBuyMaterialCosts(const FPRShopEntry& Entry, int32 TransactionQuantity, TMap<UPRMaterialDataAsset*, int32>& OutMaterialCosts) const
{
	OutMaterialCosts.Reset();

	if (TransactionQuantity <= 0)
	{
		return false;
	}

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

		int32& Quantity = OutMaterialCosts.FindOrAdd(MaterialData);
		Quantity += MaterialCost.Quantity * TransactionQuantity;
	}

	return true;
}

bool UPRShopComponent::HasEnoughBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TMap<UPRMaterialDataAsset*, int32>& MaterialCosts) const
{
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	for (const TPair<UPRMaterialDataAsset*, int32>& MaterialCost : MaterialCosts)
	{
		const UPRItemInstance_Material* MaterialItem = InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialCost.Key);
		if (!IsValid(MaterialItem) || MaterialItem->GetStackCount() < MaterialCost.Value)
		{
			return false;
		}
	}

	return true;
}

bool UPRShopComponent::ConsumeBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TMap<UPRMaterialDataAsset*, int32>& MaterialCosts, TArray<TPair<UPRMaterialDataAsset*, int32>>& OutConsumedMaterials) const
{
	OutConsumedMaterials.Reset();

	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	for (const TPair<UPRMaterialDataAsset*, int32>& MaterialCost : MaterialCosts)
	{
		if (!InventoryComponent->RemoveItemByData(MaterialCost.Key, MaterialCost.Value))
		{
			RefundBuyMaterialCosts(InventoryComponent, OutConsumedMaterials);
			OutConsumedMaterials.Reset();
			return false;
		}

		OutConsumedMaterials.Add(MaterialCost);
	}

	return true;
}

void UPRShopComponent::RefundBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TArray<TPair<UPRMaterialDataAsset*, int32>>& ConsumedMaterials) const
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	for (const TPair<UPRMaterialDataAsset*, int32>& ConsumedMaterial : ConsumedMaterials)
	{
		InventoryComponent->AddItem<UPRItemInstance_Material>(ConsumedMaterial.Key, ConsumedMaterial.Value);
	}
}

bool UPRShopComponent::ConsumeStock(const FPRShopEntry& Entry, int32 Quantity)
{
	if (Entry.bUnlimitedStock)
	{
		return true;
	}

	FPRShopRuntimeStock* RuntimeStock = FindRuntimeStock(Entry.EntryId);
	if (!RuntimeStock || RuntimeStock->RemainingStock < Quantity)
	{
		return false;
	}

	RuntimeStock->RemainingStock -= Quantity;
	return true;
}

/*~ 알림 ~*/

void UPRShopComponent::NotifyBuyResult(APRPlayerController* RequestingController, const FPRShopBuyResult& Result) const
{
	if (IsValid(RequestingController))
	{
		RequestingController->ClientNotifyShopBuyResult(Result);
	}
}

void UPRShopComponent::NotifySellResult(APRPlayerController* RequestingController, const FPRShopSellResult& Result) const
{
	if (IsValid(RequestingController))
	{
		RequestingController->ClientNotifyShopSellResult(Result);
	}
}
