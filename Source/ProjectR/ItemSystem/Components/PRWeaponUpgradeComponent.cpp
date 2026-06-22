// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재료 소모 기반 무기 업그레이드 및 데미지 스탯 강화 구현)
// Author: 배유찬 (업그레이드 정보 장비 인스턴스 동기화 구현)
#include "PRWeaponUpgradeComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/ItemSystem/PRWeaponStatStatics.h"

UPRWeaponUpgradeComponent::UPRWeaponUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

FPRWeaponUpgradeResult UPRWeaponUpgradeComponent::RequestUpgradeWeapon(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::InvalidAuthority);
	}

	if (!CanRequestUpgrade(RequestingController))
	{
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::InvalidInteractor);
	}

	UWorld* World = GetWorld();
	const double CurrentServerTime = IsValid(World) ? World->GetTimeSeconds() : 0.0;
	if (bUpgradeRequestInProgress || CurrentServerTime < NextAllowedUpgradeRequestServerTime)
	{
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::RequestThrottled);
	}

	bUpgradeRequestInProgress = true;
	NextAllowedUpgradeRequestServerTime = CurrentServerTime + UpgradeRequestMinInterval;

	APRPlayerState* PlayerState = RequestingController->GetPlayerState<APRPlayerState>();
	UPRInventoryComponent* InventoryComponent = IsValid(PlayerState) ? PlayerState->GetInventoryComponent() : nullptr;
	UPRCurrencyComponent* CurrencyComponent = IsValid(PlayerState) ? PlayerState->GetCurrencyComponent() : nullptr;
	if (!IsValid(InventoryComponent) || !IsValid(CurrencyComponent) || !IsValid(WeaponItem) || !InventoryComponent->OwnsItem(WeaponItem))
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::InvalidWeapon);
	}

	const int32 CurrentUpgradeLevel = WeaponItem->GetUpgradeLevel();
	const int32 TargetUpgradeLevel = CurrentUpgradeLevel + 1;
	if (CurrentUpgradeLevel >= MaxUpgradeLevel)
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::MaxLevel);
	}

	const FPRWeaponUpgradeRow* UpgradeRow = FindNextUpgradeRow(WeaponItem, CurrentUpgradeLevel);
	if (!UpgradeRow)
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::MissingUpgradeData);
	}

	TMap<UPRMaterialDataAsset*, int32> MaterialCosts;
	if (!ResolveMaterialCosts(UpgradeRow->Cost, MaterialCosts))
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::MissingItemData);
	}

	if (CurrencyComponent->GetScrap() < UpgradeRow->Cost.Scrap)
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::NotEnoughScrap);
	}

	for (const TPair<UPRMaterialDataAsset*, int32>& MaterialCost : MaterialCosts)
	{
		const UPRItemInstance_Material* MaterialItem = InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialCost.Key);
		if (!IsValid(MaterialItem) || MaterialItem->GetStackCount() < MaterialCost.Value)
		{
			bUpgradeRequestInProgress = false;
			return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::NotEnoughMaterial);
		}
	}

	if (UpgradeRow->Cost.Scrap > 0 && !CurrencyComponent->TrySpendScrap(UpgradeRow->Cost.Scrap))
	{
		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::ConsumeFailed);
	}

	TArray<TPair<UPRMaterialDataAsset*, int32>> ConsumedMaterials;
	for (const TPair<UPRMaterialDataAsset*, int32>& MaterialCost : MaterialCosts)
	{
		if (!InventoryComponent->RemoveItemByData(MaterialCost.Key, MaterialCost.Value))
		{
			for (const TPair<UPRMaterialDataAsset*, int32>& ConsumedMaterial : ConsumedMaterials)
			{
				InventoryComponent->AddItem<UPRItemInstance_Material>(ConsumedMaterial.Key, ConsumedMaterial.Value);
			}

			if (UpgradeRow->Cost.Scrap > 0)
			{
				CurrencyComponent->AddScrap(UpgradeRow->Cost.Scrap);
			}

			bUpgradeRequestInProgress = false;
			return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::ConsumeFailed);
		}

		ConsumedMaterials.Add(MaterialCost);
	}

	if (!WeaponItem->SetUpgradeLevel(TargetUpgradeLevel))
	{
		for (const TPair<UPRMaterialDataAsset*, int32>& ConsumedMaterial : ConsumedMaterials)
		{
			InventoryComponent->AddItem<UPRItemInstance_Material>(ConsumedMaterial.Key, ConsumedMaterial.Value);
		}

		if (UpgradeRow->Cost.Scrap > 0)
		{
			CurrencyComponent->AddScrap(UpgradeRow->Cost.Scrap);
		}

		bUpgradeRequestInProgress = false;
		return MakeFailureResult(RequestingController, WeaponItem, EPRWeaponUpgradeFailReason::ConsumeFailed);
	}

	if (UPRWeaponManagerComponent* WeaponManager = ResolveWeaponManager(RequestingController))
	{
		WeaponManager->RefreshCurrentWeaponUpgradeState(WeaponItem);
	}

	bUpgradeRequestInProgress = false;
	return MakeSuccessResult(RequestingController, WeaponItem);
}

const FPRWeaponUpgradeRow* UPRWeaponUpgradeComponent::FindNextUpgradeRow(const UPRItemInstance_Weapon* WeaponItem, int32 CurrentUpgradeLevel) const
{
	const UDataTable* TargetUpgradeTable = ResolveUpgradeTable(WeaponItem);
	if (!IsValid(TargetUpgradeTable))
	{
		return nullptr;
	}

	const FName RowName = MakeUpgradeRowName(CurrentUpgradeLevel + 1);
	static const FString ContextString(TEXT("UPRWeaponUpgradeComponent::FindNextUpgradeRow"));
	return TargetUpgradeTable->FindRow<FPRWeaponUpgradeRow>(RowName, ContextString, false);
}

FPRWeaponUpgradePreview UPRWeaponUpgradeComponent::BuildUpgradePreview(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem) const
{
	FPRWeaponUpgradePreview Preview;

	Preview.bValidWeapon = IsValid(WeaponItem);
	if (!Preview.bValidWeapon)
	{
		return Preview;
	}

	Preview.CurrentLevel = WeaponItem->GetUpgradeLevel();
	Preview.bMaxLevel = Preview.CurrentLevel >= MaxUpgradeLevel;
	Preview.TargetLevel = Preview.bMaxLevel ? Preview.CurrentLevel : Preview.CurrentLevel + 1;
	const UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	Preview.CurrentBaseDamage = UPRWeaponStatStatics::CalculateUpgradedBaseDamage(WeaponData, Preview.CurrentLevel);
	Preview.TargetBaseDamage = UPRWeaponStatStatics::CalculateUpgradedBaseDamage(WeaponData, Preview.TargetLevel);
	if (Preview.bMaxLevel)
	{
		return Preview;
	}

	APRPlayerState* PlayerState = IsValid(RequestingController)
		? RequestingController->GetPlayerState<APRPlayerState>()
		: nullptr;
	UPRInventoryComponent* InventoryComponent = IsValid(PlayerState) ? PlayerState->GetInventoryComponent() : nullptr;
	UPRCurrencyComponent* CurrencyComponent = IsValid(PlayerState) ? PlayerState->GetCurrencyComponent() : nullptr;

	Preview.bOwnsWeapon = IsValid(InventoryComponent) && InventoryComponent->OwnsItem(WeaponItem);
	Preview.OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;

	const FPRWeaponUpgradeRow* UpgradeRow = FindNextUpgradeRow(WeaponItem, Preview.CurrentLevel);
	if (!UpgradeRow)
	{
		return Preview;
	}

	Preview.bHasUpgradeData = true;
	Preview.ScrapCost = UpgradeRow->Cost.Scrap;
	Preview.bEnoughScrap = Preview.OwnedScrap >= Preview.ScrapCost;

	TMap<UPRMaterialDataAsset*, int32> MaterialCosts;
	if (!ResolveMaterialCosts(UpgradeRow->Cost, MaterialCosts))
	{
		Preview.bHasUpgradeData = false;
		return Preview;
	}

	bool bEnoughMaterials = true;
	for (const TPair<UPRMaterialDataAsset*, int32>& MaterialCost : MaterialCosts)
	{
		FPRWeaponUpgradeMaterialCostViewData MaterialCostViewData;
		MaterialCostViewData.MaterialData = MaterialCost.Key;
		MaterialCostViewData.RequiredQuantity = MaterialCost.Value;
		MaterialCostViewData.DisplayName = IsValid(MaterialCost.Key) ? MaterialCost.Key->GetDisplayName() : FText::GetEmpty();
		MaterialCostViewData.Icon = IsValid(MaterialCost.Key) ? MaterialCost.Key->GetIcon() : nullptr;

		const UPRItemInstance_Material* MaterialItem = IsValid(InventoryComponent)
			? InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialCost.Key)
			: nullptr;
		MaterialCostViewData.OwnedQuantity = IsValid(MaterialItem) ? MaterialItem->GetStackCount() : 0;
		MaterialCostViewData.bEnough = MaterialCostViewData.OwnedQuantity >= MaterialCostViewData.RequiredQuantity;
		bEnoughMaterials &= MaterialCostViewData.bEnough;

		Preview.MaterialCosts.Add(MaterialCostViewData);
	}

	Preview.bCanUpgrade = Preview.bOwnsWeapon
		&& Preview.bHasUpgradeData
		&& Preview.bEnoughScrap
		&& bEnoughMaterials;
	return Preview;
}

FName UPRWeaponUpgradeComponent::MakeUpgradeRowName(int32 UpgradeLevel) const
{
	return FName(*FString::Printf(TEXT("Level_%d"), UpgradeLevel));
}

bool UPRWeaponUpgradeComponent::CanRequestUpgrade(APRPlayerController* RequestingController) const
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

bool UPRWeaponUpgradeComponent::ResolveMaterialCosts(const FPREconomyCost& Cost, TMap<UPRMaterialDataAsset*, int32>& OutMaterialCosts) const
{
	OutMaterialCosts.Reset();

	for (const FPREconomyItemCost& ItemCost : Cost.Items)
	{
		if (!ItemCost.ItemAssetId.IsValid() || ItemCost.Quantity <= 0)
		{
			return false;
		}

		UPRItemDataAsset* ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(ItemCost.ItemAssetId);
		UPRMaterialDataAsset* MaterialData = Cast<UPRMaterialDataAsset>(ItemData);
		if (!IsValid(MaterialData))
		{
			return false;
		}

		int32& Quantity = OutMaterialCosts.FindOrAdd(MaterialData);
		Quantity += ItemCost.Quantity;
	}

	return true;
}

UPRWeaponManagerComponent* UPRWeaponUpgradeComponent::ResolveWeaponManager(APRPlayerController* RequestingController) const
{
	if (!IsValid(RequestingController))
	{
		return nullptr;
	}

	APawn* RequestingPawn = RequestingController->GetPawn();
	if (!IsValid(RequestingPawn))
	{
		return nullptr;
	}

	return RequestingPawn->FindComponentByClass<UPRWeaponManagerComponent>();
}

UDataTable* UPRWeaponUpgradeComponent::ResolveUpgradeTable(const UPRItemInstance_Weapon* WeaponItem) const
{
	const UPRWeaponDataAsset* WeaponData = IsValid(WeaponItem) ? WeaponItem->GetWeaponData() : nullptr;
	return ResolveUpgradeTableByWeaponData(WeaponData);
}

UDataTable* UPRWeaponUpgradeComponent::ResolveUpgradeTableByWeaponData(const UPRWeaponDataAsset* WeaponData) const
{
	if (IsValid(WeaponData) && IsValid(WeaponData->WeaponUpgradeTable))
	{
		return WeaponData->WeaponUpgradeTable;
	}

	return UpgradeTable;
}

FPRWeaponUpgradeResult UPRWeaponUpgradeComponent::MakeFailureResult(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem, EPRWeaponUpgradeFailReason FailReason)
{
	FPRWeaponUpgradeResult Result;
	Result.bSuccess = false;
	Result.FailReason = FailReason;
	Result.UpgradeComponent = this;
	Result.WeaponItem = WeaponItem;
	Result.UpgradeLevel = IsValid(WeaponItem) ? WeaponItem->GetUpgradeLevel() : 0;

	NotifyResult(RequestingController, Result);
	return Result;
}

FPRWeaponUpgradeResult UPRWeaponUpgradeComponent::MakeSuccessResult(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem)
{
	FPRWeaponUpgradeResult Result;
	Result.bSuccess = true;
	Result.FailReason = EPRWeaponUpgradeFailReason::None;
	Result.UpgradeComponent = this;
	Result.WeaponItem = WeaponItem;
	Result.UpgradeLevel = IsValid(WeaponItem) ? WeaponItem->GetUpgradeLevel() : 0;

	NotifyResult(RequestingController, Result);
	return Result;
}

void UPRWeaponUpgradeComponent::NotifyResult(APRPlayerController* RequestingController, const FPRWeaponUpgradeResult& Result) const
{
	if (IsValid(RequestingController))
	{
		RequestingController->ClientNotifyWeaponUpgradeResult(Result);
	}
}
