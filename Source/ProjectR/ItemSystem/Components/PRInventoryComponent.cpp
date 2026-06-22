// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 강화 재료 및 재화 인벤토리 소모 연동 구현)
// Author: 배유찬 (인벤토리 기초 구조 설계 및 획득 아이템 인벤토리 보관 연동 구현)
// Author: 이건주 (아이템 퀵슬롯 HUD, 무기 Mod 장착 슬롯 및 3D 프리뷰 연동 구현)
#include "PRInventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

namespace
{
	// 무기와 Mod의 태그 호환 여부 판정
	bool IsWeaponModCompatible(const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
	{
		// 무기 데이터나 Mod 데이터가 없는 경우
		if (!IsValid(WeaponData) || !IsValid(ModData))
		{
			return false;
		}

		// Mod 태그가 비어 있으면 범용 Mod로 간주
		if (ModData->ModTags.IsEmpty())
		{
			return true;
		}

		// 무기가 지원 태그를 하나도 선언하지 않았으면 태그가 있는 Mod 수용 불가
		if (WeaponData->SupportedModTags.IsEmpty())
		{
			return false;
		}

		return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
	}
}

void FPRInventoryItemCache::Reset()
{
	ItemsByType.Reset();
	ItemsByData.Reset();
}

void FPRInventoryItemCache::Rebuild(const TArray<UPRItemInstance*>& SourceItems)
{
	Reset();

	for (UPRItemInstance* Item : SourceItems)
	{
		AddItem(Item);
	}
}

void FPRInventoryItemCache::AddItem(UPRItemInstance* Item)
{
	// 캐시 등록 대상 검증
	if (!IsValid(Item) || !IsValid(Item->GetItemData()))
	{
		return;
	}

	UPRItemDataAsset* ItemData = Item->GetItemData();
	ItemsByType.FindOrAdd(ItemData->GetItemType()).Items.AddUnique(Item);
	TObjectPtr<UPRItemDataAsset> ItemDataKey = ItemData;
	ItemsByData.FindOrAdd(ItemDataKey).Items.AddUnique(Item);
}

void FPRInventoryItemCache::RemoveItem(UPRItemInstance* Item)
{
	// 캐시 제거 대상 검증
	if (!IsValid(Item) || !IsValid(Item->GetItemData()))
	{
		return;
	}

	UPRItemDataAsset* ItemData = Item->GetItemData();
	if (FPRInventoryItemList* ItemsForType = ItemsByType.Find(ItemData->GetItemType()))
	{
		ItemsForType->Items.Remove(Item);
		if (ItemsForType->Items.IsEmpty())
		{
			ItemsByType.Remove(ItemData->GetItemType());
		}
	}

	TObjectPtr<UPRItemDataAsset> ItemDataKey = ItemData;
	if (FPRInventoryItemList* ItemsForData = ItemsByData.Find(ItemDataKey))
	{
		ItemsForData->Items.Remove(Item);
		if (ItemsForData->Items.IsEmpty())
		{
			ItemsByData.Remove(ItemDataKey);
		}
	}
}

UPRItemInstance* FPRInventoryItemCache::FindItemByData(const UPRItemDataAsset* ItemData) const
{
	// 데이터 조회 대상 검증
	if (!IsValid(ItemData))
	{
		return nullptr;
	}

	TObjectPtr<UPRItemDataAsset> ItemDataKey = const_cast<UPRItemDataAsset*>(ItemData);
	const FPRInventoryItemList* ItemsForData = ItemsByData.Find(ItemDataKey);
	if (!ItemsForData)
	{
		return nullptr;
	}

	for (UPRItemInstance* Item : ItemsForData->Items)
	{
		if (IsValid(Item))
		{
			return Item;
		}
	}

	return nullptr;
}

const FPRInventoryItemList* FPRInventoryItemCache::FindItemsByType(EPRItemType ItemType) const
{
	return ItemsByType.Find(ItemType);
}

UPRInventoryComponent::UPRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRInventoryComponent, InventoryItems, COND_OwnerOnly);
}

bool UPRInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (!RepFlags->bNetOwner)
	{
		return bWroteSomething;
	}

	for (UPRItemInstance* Item : InventoryItems)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void UPRInventoryComponent::RequestAddItem(UPRItemDataAsset* InItemData, int32 Amount)
{
	// 데이터 없는 요청 제외
	if (!IsValid(InItemData))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		AddItem<UPRItemInstance>(InItemData, Amount);
		return;
	}

	Server_RequestAddItem(InItemData, Amount);
}

UPRItemInstance* UPRInventoryComponent::AddItem(UPRItemDataAsset* InItemData, int32 Amount)
{
	return AddItemInternal(InItemData, Amount);
}

UPRItemInstance* UPRInventoryComponent::BP_AddItem(UPRItemDataAsset* InItemData, TSubclassOf<UPRItemInstance> ItemInstanceClass, int32 Amount)
{
	return AddItemInternal(InItemData, Amount);
}

UPRItemInstance* UPRInventoryComponent::AddItemInternal(UPRItemDataAsset* InItemData, int32 Amount)
{
	// 데이터 없는 생성 제외
	if (!IsValid(InItemData) || Amount <= 0)
	{
		return nullptr;
	}

	TSubclassOf<UPRItemInstance> ItemInstanceClass = InItemData->GetItemInstanceClass();
	if (!ensureMsgf(IsValid(ItemInstanceClass),TEXT("%s: Invalid Item Instance Type"), *InItemData->GetName()))
	{
		return nullptr;
	}

	if (InItemData->MaxStackCount > 1)
	{
		if (UPRItemInstance* ExistingItem = FindItemByData(InItemData))
		{
			ExistingItem->AddStack(Amount);

			if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[Inventory][Server] AddItemStack. Owner = %s | Item = %s | Data = %s | StackCount = %d"),
					*GetNameSafe(GetOwner()),
					*GetNameSafe(ExistingItem),
					*GetNameSafe(InItemData),
					ExistingItem->GetStackCount());
			}

			return ExistingItem;
		}
	}

	UPRItemInstance* NewItem = NewObject<UPRItemInstance>(this, ItemInstanceClass);
	if (!IsValid(NewItem))
	{
		return nullptr;
	}

	NewItem->InitializeItem(InItemData, Amount);
	RegisterInventoryItem(NewItem);

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Server] AddItem. Owner = %s | Item = %s | Data = %s | StackCount = %d | Count = %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NewItem),
			*GetNameSafe(InItemData),
			NewItem->GetStackCount(),
			InventoryItems.Num());
	}

	return NewItem;
}

void UPRInventoryComponent::RequestRemoveItem(UPRItemInstance* ItemInstance, int32 RemoveCount)
{
	// 잘못된 Item 제거 요청 제외
	if (!IsValid(ItemInstance) || RemoveCount <= 0)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		RemoveItemInternal(ItemInstance, RemoveCount);
		return;
	}

	Server_RequestRemoveItem(ItemInstance, RemoveCount);
}

void UPRInventoryComponent::RequestRemoveItemByData(UPRItemDataAsset* ItemData, int32 RemoveCount)
{
	// 잘못된 Item 데이터 제거 요청 제외
	if (!IsValid(ItemData) || RemoveCount <= 0)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		RemoveItemByDataInternal(ItemData, RemoveCount);
		return;
	}

	Server_RequestRemoveItemByData(ItemData, RemoveCount);
}

bool UPRInventoryComponent::RemoveItemByData(UPRItemDataAsset* ItemData, int32 RemoveCount)
{
	return RemoveItemByDataInternal(ItemData, RemoveCount);
}

bool UPRInventoryComponent::RemoveItemInternal(UPRItemInstance* ItemInstance, int32 RemoveCount)
{
	// Item 제거 대상 검증
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ItemInstance) || !OwnsItem(ItemInstance) || RemoveCount <= 0)
	{
		return false;
	}

	const int32 PreviousStackCount = ItemInstance->GetStackCount();
	if (!ItemInstance->RemoveStack(RemoveCount))
	{
		return false;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] RemoveItem. Owner = %s | Item = %s | Data = %s | BeforeStackCount = %d | AfterStackCount = %d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ItemInstance),
		*GetNameSafe(ItemInstance->GetItemData()),
		PreviousStackCount,
		ItemInstance->GetStackCount());

	if (ItemInstance->GetStackCount() <= 0)
	{
		UnregisterInventoryItem(ItemInstance);
	}

	return true;
}

bool UPRInventoryComponent::RemoveItemByDataInternal(UPRItemDataAsset* ItemData, int32 RemoveCount)
{
	// 데이터 기반 제거 대상 검증
	if (!IsValid(ItemData))
	{
		return false;
	}

	return RemoveItemInternal(FindItemByData(ItemData), RemoveCount);
}

void UPRInventoryComponent::RequestActivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	// 잘못된 Item 활성화 요청 제외
	if (!IsValid(ItemInstance))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		ActivateItemInternal(ItemInstance, ActivationContext);
		return;
	}

	Server_RequestActivateItem(ItemInstance, ActivationContext);
}

void UPRInventoryComponent::RequestDeactivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	// 잘못된 Item 비활성화 요청 제외
	if (!IsValid(ItemInstance))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		DeactivateItemInternal(ItemInstance, ActivationContext);
		return;
	}

	Server_RequestDeactivateItem(ItemInstance, ActivationContext);
}

void UPRInventoryComponent::RequestUseConsumableItem(UPRItemInstance_Consumable* ConsumableItem)
{
	// 잘못된 소비 Item 사용 요청 제외
	if (!IsValid(ConsumableItem))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UseConsumableItemInternal(ConsumableItem);
		return;
	}

	Server_RequestUseConsumableItem(ConsumableItem);
}

bool UPRInventoryComponent::ActivateItemInternal(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	// Item 활성화 소유권 검증
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ItemInstance) || !InventoryItems.Contains(ItemInstance))
	{
		return false;
	}

	FPRItemActivationContext RuntimeContext = ActivationContext;
	RuntimeContext.InventoryComponent = this;

	if (!ItemInstance->ActivateItem(RuntimeContext))
	{
		return false;
	}

	if (ItemInstance->GetStackCount() <= 0)
	{
		UnregisterInventoryItem(ItemInstance);
	}

	return true;
}

bool UPRInventoryComponent::DeactivateItemInternal(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	// Item 비활성화 소유권 검증
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ItemInstance) || !InventoryItems.Contains(ItemInstance))
	{
		return false;
	}

	FPRItemActivationContext RuntimeContext = ActivationContext;
	RuntimeContext.InventoryComponent = this;

	return ItemInstance->DeactivateItem(RuntimeContext);
}

bool UPRInventoryComponent::UseConsumableItemInternal(UPRItemInstance_Consumable* ConsumableItem)
{
	// 소비 Item 사용 소유권 검증
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ConsumableItem) || !OwnsItem(ConsumableItem))
	{
		return false;
	}

	return ConsumableItem->UseItem(GetOwner());
}

void UPRInventoryComponent::RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 잘못된 Item 참조 요청은 서버 RPC를 보내기 전에 중단
	if (!IsValid(ModItem) || !IsValid(TargetWeaponItem))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		EquipModItemToWeapon(ModItem, TargetWeaponItem);
		return;
	}

	Server_RequestEquipModItemToWeapon(ModItem, TargetWeaponItem);
}

void UPRInventoryComponent::RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 잘못된 Item 참조 요청은 서버 RPC를 보내기 전에 중단
	if (!IsValid(TargetWeaponItem))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UnequipModFromWeapon(TargetWeaponItem);
		return;
	}

	Server_RequestUnequipModFromWeapon(TargetWeaponItem);
}

bool UPRInventoryComponent::EquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 서버 내부 장착에 필요한 Item 참조와 소유권이 유효하지 않은 경우
	if (!IsValid(ModItem) || !OwnsItem(ModItem) || !IsValid(ModItem->GetModData()))
	{
		// Mod Item 소유권이나 데이터 누락 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | ModItem = %s | Reason = InvalidModItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ModItem));

		// 장착 실패. Mod Item 조회나 데이터 검증 실패
		return false;
	}

	// 장착 대상 Weapon Item은 같은 인벤토리 소유여야 서버 정본 변경이 가능하다
	if (!IsValid(TargetWeaponItem) || !OwnsItem(TargetWeaponItem))
	{
		// Weapon Item 소유권 검증 실패 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = InvalidWeaponItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 장착 실패. Weapon Item 소유권 검증 실패
		return false;
	}

	// Mod 장착은 인벤토리 정본 변경 전 무기 데이터와 Mod 태그 호환성을 먼저 검증
	if (!IsWeaponModCompatible(TargetWeaponItem->GetWeaponData(), ModItem->GetModData()))
	{
		// 호환되지 않는 무기와 Mod 조합을 Owner, Weapon Item, Mod Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Reason = IncompatibleMod"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem),
			*GetNameSafe(ModItem));

		// 장착 실패. 무기와 Mod 태그 호환성 검증 실패
		return false;
	}

	// 이미 같은 무기에 같은 Mod Item이 연결된 경우 중복 요청으로 처리
	if (TargetWeaponItem->GetEquippedModItem() == ModItem
		&& ModItem->GetEquippedWeaponItem() == TargetWeaponItem)
	{
		// 중복 장착 요청을 Owner, Weapon Item, Mod Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Reason = AlreadyEquipped"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem),
			*GetNameSafe(ModItem));

		// 장착 실패. 이미 동일한 연결 상태다
		return false;
	}

	// 이동 장착을 위해 Mod Item이 기억하던 기존 Weapon Item 연결을 먼저 해제
	UPRItemInstance_Weapon* PreviousWeaponItem = ModItem->GetEquippedWeaponItem();
	if (IsValid(PreviousWeaponItem) && OwnsItem(PreviousWeaponItem) && PreviousWeaponItem != TargetWeaponItem)
	{
		// 기존 장착 무기에서 Mod Item 연결과 적용 데이터를 제거
		PreviousWeaponItem->ClearEquippedModItem();
		PreviousWeaponItem->SetModData(nullptr);

		// 기존 무기가 현재 장착 중이면 어빌리티와 공개 상태를 해제 상태로 갱신
		NotifyWeaponItemModChanged(PreviousWeaponItem);
	}
	else if (!IsValid(PreviousWeaponItem) || !OwnsItem(PreviousWeaponItem))
	{
		// 인벤토리에 없는 Weapon Item을 가리키는 오래된 연결은 새 장착 전에 정리
		ModItem->ClearEquippedWeaponItem();
	}

	// 타겟 무기에 다른 Mod Item이 있었다면 해당 Mod Item의 역참조를 먼저 비운다
	UPRItemInstance_Mod* PreviousModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(PreviousModItem) && PreviousModItem != ModItem)
	{
		PreviousModItem->ClearEquippedWeaponItem();
	}

	// 인벤토리 정본 상태를 새 Mod Item과 타겟 Weapon Item의 양방향 연결로 확정
	TargetWeaponItem->SetEquippedModItem(ModItem);
	TargetWeaponItem->SetModData(ModItem->GetModData());
	ModItem->MarkEquippedToWeaponItem(TargetWeaponItem);

	// 타겟 무기가 현재 장착 중이면 어빌리티와 공개 상태를 새 Mod 기준으로 갱신
	NotifyWeaponItemModChanged(TargetWeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	// 서버 장착 처리가 확정된 뒤 Owner, Weapon Item, Mod Item, Mod 데이터를 남겨 UI 요청 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] Mod 장착 완료. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Mod = %s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetWeaponItem),
		*GetNameSafe(ModItem),
		*GetNameSafe(ModItem->GetModData()));

	// 장착 성공. 기존 연결 해제, 새 연결 확정, 장착 중 무기 런타임 갱신 마침
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ModEquipChanged;
	EventData.ItemInstance = TargetWeaponItem->GetEquippedModItem();
	OnInventoryChanged(EventData);
	return true;
}

bool UPRInventoryComponent::UnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 해제 대상 Weapon Item은 같은 인벤토리 소유여야 서버 정본 변경이 가능하다
	if (!IsValid(TargetWeaponItem) || !OwnsItem(TargetWeaponItem))
	{
		// Weapon Item 소유권 검증 실패 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 해제 실패. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = InvalidWeaponItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 해제 실패. Weapon Item 소유권 검증 실패
		return false;
	}

	// Weapon Item이 기억하던 Mod Item이 있으면 Mod Item의 역참조를 함께 정리
	UPRItemInstance_Mod* PreviousModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(PreviousModItem))
	{
		PreviousModItem->ClearEquippedWeaponItem();
	}

	// 장착된 Mod Item도 Mod 데이터도 없으면 해제할 상태 없음
	if (!IsValid(PreviousModItem) && !IsValid(TargetWeaponItem->GetModData()))
	{
		// 무변경 해제 요청을 Owner와 Weapon Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 해제 실패. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = EmptyMod"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 해제 실패. 이미 빈 Mod 상태다
		return false;
	}

	// 인벤토리 정본 상태에서 Weapon Item의 Mod 연결을 제거
	TargetWeaponItem->ClearEquippedModItem();
	TargetWeaponItem->SetModData(nullptr);

	// 타겟 무기가 현재 장착 중이면 어빌리티와 공개 상태를 빈 Mod 기준으로 갱신
	NotifyWeaponItemModChanged(TargetWeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	// 서버 해제 처리가 확정된 뒤 Owner, Weapon Item, 이전 Mod Item을 남겨 UI 요청 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] Mod 해제 완료. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | PreviousModItem = %s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetWeaponItem),
		*GetNameSafe(PreviousModItem));

	// 해제 성공. Mod Item 역참조와 Weapon Item Mod 상태 갱신 마침
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ModEquipChanged;
	EventData.ItemInstance = TargetWeaponItem->GetEquippedModItem();
	
	OnInventoryChanged(EventData);
	return true;
}

bool UPRInventoryComponent::OwnsItem(const UPRItemInstance* ItemInstance) const
{
	// 유효하지 않은 Item 소유권 검사 제외
	if (!IsValid(ItemInstance))
	{
		return false;
	}

	return InventoryItems.Contains(const_cast<UPRItemInstance*>(ItemInstance));
}

UPRItemInstance* UPRInventoryComponent::FindItemByData(const UPRItemDataAsset* InItemData) const
{
	// 데이터 없는 조회 제외
	if (!IsValid(InItemData))
	{
		return nullptr;
	}

	if (UPRItemInstance* CachedItem = InventoryItemCache.FindItemByData(InItemData))
	{
		return CachedItem;
	}

	for (UPRItemInstance* Item : InventoryItems)
	{
		if (IsValid(Item) && Item->GetItemData() == InItemData)
		{
			return Item;
		}
	}

	return nullptr;
}

TArray<UPRItemInstance*> UPRInventoryComponent::GetItemsByType(EPRItemType ItemType) const
{
	TArray<UPRItemInstance*> Items;
	const FPRInventoryItemList* CachedItems = InventoryItemCache.FindItemsByType(ItemType);
	if (!CachedItems)
	{
		return Items;
	}

	Items.Reserve(CachedItems->Items.Num());
	for (UPRItemInstance* Item : CachedItems->Items)
	{
		if (IsValid(Item))
		{
			Items.Add(Item);
		}
	}

	return Items;
}

int32 UPRInventoryComponent::GetItemIndexByType(const UPRItemInstance* ItemInstance, EPRItemType ItemType) const
{
	if (!IsValid(ItemInstance))
	{
		return INDEX_NONE;
	}

	const TArray<UPRItemInstance*> Items = GetItemsByType(ItemType);
	return Items.IndexOfByKey(const_cast<UPRItemInstance*>(ItemInstance));
}

void UPRInventoryComponent::OnInventoryChanged(const FPRInventoryChangeEventData& EventData)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// ItemInstance 내부 값 변경 복제 갱신
		GetOwner()->ForceNetUpdate();
	}

	OnInventoryChangedDelegate.Broadcast(this, EventData);
}

void UPRInventoryComponent::ResetSystem()
{
	// 정본 Item 배열 기준 캐시 복구
	RebuildInventoryCaches();
}

FPRInventorySaveData UPRInventoryComponent::MakeSaveData() const
{
	FPRInventorySaveData SaveData;
	const TArray<UPRItemInstance_Weapon*> WeaponItems = GetItemsByType<UPRItemInstance_Weapon>(EPRItemType::Weapon);
	const TArray<UPRItemInstance_Mod*> ModItems = GetItemsByType<UPRItemInstance_Mod>(EPRItemType::Mod);
	const TArray<UPRItemInstance_Consumable*> ConsumableItems = GetItemsByType<UPRItemInstance_Consumable>(EPRItemType::Consumable);
	const TArray<UPRItemInstance_Material*> MaterialItems = GetItemsByType<UPRItemInstance_Material>(EPRItemType::Material);
	const TArray<UPRItemInstance_Equipment*> EquipmentItems = GetItemsByType<UPRItemInstance_Equipment>(EPRItemType::Equipment);
	SaveData.Weapons.Reserve(WeaponItems.Num());
	SaveData.Mods.Reserve(ModItems.Num());
	SaveData.Consumables.Reserve(ConsumableItems.Num());
	SaveData.Materials.Reserve(MaterialItems.Num());
	SaveData.Equipments.Reserve(EquipmentItems.Num());

	for (const UPRItemInstance_Weapon* WeaponItem : WeaponItems)
	{
		if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
		{
			continue;
		}

		FPRWeaponItemSaveEntry Entry;
		WeaponItem->FillSaveEntry(Entry);
		Entry.EquippedModIndex = GetItemIndexByType(WeaponItem->GetEquippedModItem(), EPRItemType::Mod);
		SaveData.Weapons.Add(Entry);
	}

	for (const UPRItemInstance_Mod* ModItem : ModItems)
	{
		if (!IsValid(ModItem) || !IsValid(ModItem->GetModData()))
		{
			continue;
		}

		FPRModItemSaveEntry Entry;
		ModItem->FillSaveEntry(Entry);
		SaveData.Mods.Add(Entry);
	}

	for (const UPRItemInstance_Consumable* ConsumableItem : ConsumableItems)
	{
		if (!IsValid(ConsumableItem) || !IsValid(ConsumableItem->GetConsumableData()))
		{
			continue;
		}

		FPRConsumableSaveEntry Entry;
		Entry.ConsumableData = ConsumableItem->GetConsumableData();
		Entry.StackCount = ConsumableItem->GetStackCount();
		SaveData.Consumables.Add(Entry);
	}

	for (const UPRItemInstance_Material* MaterialItem : MaterialItems)
	{
		if (!IsValid(MaterialItem) || !IsValid(MaterialItem->GetMaterialData()))
		{
			continue;
		}

		FPRMaterialSaveEntry Entry;
		Entry.MaterialData = MaterialItem->GetMaterialData();
		Entry.StackCount = MaterialItem->GetStackCount();
		SaveData.Materials.Add(Entry);
	}

	for (const UPRItemInstance_Equipment* EquipmentItem : EquipmentItems)
	{
		if (!IsValid(EquipmentItem) || !IsValid(EquipmentItem->GetEquipmentData()))
		{
			continue;
		}

		FPREquipmentItemSaveEntry Entry;
		Entry.EquipmentData = EquipmentItem->GetEquipmentData();
		Entry.StackCount = FMath::Max(EquipmentItem->GetStackCount(), 1);
		SaveData.Equipments.Add(Entry);
	}

	return SaveData;
}

void UPRInventoryComponent::ApplySaveData(const FPRInventorySaveData& InSaveData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	InventoryItems.Reset();
	InventoryItemCache.Reset();

	TArray<UPRItemInstance_Mod*> RestoredModItems;
	RestoredModItems.Reserve(InSaveData.Mods.Num());

	for (const FPRModItemSaveEntry& Entry : InSaveData.Mods)
	{
		UPRWeaponModDataAsset* ModData = Entry.ModData.LoadSynchronous();
		if (!IsValid(ModData))
		{
			continue;
		}

		UPRItemInstance_Mod* NewModItem = Cast<UPRItemInstance_Mod>(NewObject<UPRItemInstance>(this, ModData->GetItemInstanceClass()));
		if (!IsValid(NewModItem))
		{
			continue;
		}

		// Mod 아이템 복원
		NewModItem->InitializeItem(ModData, 1);
		NewModItem->ApplySaveEntry(Entry);
		InventoryItems.Add(NewModItem);
		RestoredModItems.Add(NewModItem);
	}

	for (const FPRWeaponItemSaveEntry& Entry : InSaveData.Weapons)
	{
		UPRWeaponDataAsset* WeaponData = Entry.WeaponData.LoadSynchronous();
		if (!IsValid(WeaponData))
		{
			continue;
		}

		UPRItemInstance_Weapon* NewWeaponItem = Cast<UPRItemInstance_Weapon>(NewObject<UPRItemInstance>(this, WeaponData->GetItemInstanceClass()));
		if (!IsValid(NewWeaponItem))
		{
			continue;
		}

		// 무기 아이템 복원
		NewWeaponItem->InitializeItem(WeaponData, FMath::Max(Entry.StackCount, 1));
		NewWeaponItem->ApplySaveEntry(Entry);

		if (RestoredModItems.IsValidIndex(Entry.EquippedModIndex))
		{
			UPRItemInstance_Mod* ModItem = RestoredModItems[Entry.EquippedModIndex];
			if (IsValid(ModItem) && IsValid(ModItem->GetModData()))
			{
				// Mod 장착 관계 복원
				NewWeaponItem->SetEquippedModItem(ModItem);
				NewWeaponItem->SetModData(ModItem->GetModData());
				ModItem->MarkEquippedToWeaponItem(NewWeaponItem);
			}
		}

		InventoryItems.Add(NewWeaponItem);
	}

	for (const FPREquipmentItemSaveEntry& Entry : InSaveData.Equipments)
	{
		UPREquipmentDataAsset* EquipmentData = Entry.EquipmentData.LoadSynchronous();
		if (!IsValid(EquipmentData))
		{
			continue;
		}

		UPRItemInstance_Equipment* NewEquipmentItem = Cast<UPRItemInstance_Equipment>(NewObject<UPRItemInstance>(this, EquipmentData->GetItemInstanceClass()));
		if (!IsValid(NewEquipmentItem))
		{
			continue;
		}

		// 장비 아이템 복원
		NewEquipmentItem->InitializeItem(EquipmentData, FMath::Max(Entry.StackCount, 1));
		InventoryItems.Add(NewEquipmentItem);
	}

	for (const FPRConsumableSaveEntry& Entry : InSaveData.Consumables)
	{
		UPRConsumableDataAsset* ConsumableData = Entry.ConsumableData.LoadSynchronous();
		if (!IsValid(ConsumableData) || Entry.StackCount <= 0)
		{
			continue;
		}

		UPRItemInstance_Consumable* NewConsumableItem = Cast<UPRItemInstance_Consumable>(NewObject<UPRItemInstance>(this, ConsumableData->GetItemInstanceClass()));
		if (!IsValid(NewConsumableItem))
		{
			continue;
		}

		// 소비 아이템 복원
		NewConsumableItem->InitializeItem(ConsumableData, Entry.StackCount);
		InventoryItems.Add(NewConsumableItem);
	}

	for (const FPRMaterialSaveEntry& Entry : InSaveData.Materials)
	{
		UPRMaterialDataAsset* MaterialData = Entry.MaterialData.LoadSynchronous();
		if (!IsValid(MaterialData) || Entry.StackCount <= 0)
		{
			continue;
		}

		UPRItemInstance_Material* NewMaterialItem = Cast<UPRItemInstance_Material>(NewObject<UPRItemInstance>(this, MaterialData->GetItemInstanceClass()));
		if (!IsValid(NewMaterialItem))
		{
			continue;
		}

		// 재료 아이템 복원
		NewMaterialItem->InitializeItem(MaterialData, Entry.StackCount);
		InventoryItems.Add(NewMaterialItem);
	}

	RebuildInventoryCaches();

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::BulkRestored;
	EventData.ItemInstance = nullptr;
	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::OnRep_InventoryItems(const TArray<UPRItemInstance*>& OldInventoryItems)
{
	RebuildInventoryCaches();

	for (UPRItemInstance* Item : InventoryItems)
	{
		if (!IsValid(Item) || OldInventoryItems.Contains(Item))
		{
			continue;
		}

		BroadcastInventoryItemChanged(EPRInventoryChangeReason::ItemAdded, Item);
	}

	for (UPRItemInstance* OldItem : OldInventoryItems)
	{
		if (!IsValid(OldItem) || InventoryItems.Contains(OldItem))
		{
			continue;
		}

		BroadcastInventoryItemChanged(EPRInventoryChangeReason::ItemRemoved, OldItem);
	}
}

void UPRInventoryComponent::RegisterInventoryItem(UPRItemInstance* ItemInstance)
{
	// 등록 대상 검증
	if (!IsValid(ItemInstance))
	{
		return;
	}

	if (InventoryItems.Contains(ItemInstance))
	{
		return;
	}

	InventoryItems.Add(ItemInstance);
	InventoryItemCache.AddItem(ItemInstance);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	BroadcastInventoryItemChanged(EPRInventoryChangeReason::ItemAdded, ItemInstance);
}

void UPRInventoryComponent::UnregisterInventoryItem(UPRItemInstance* ItemInstance)
{
	// 등록 해제 대상 검증
	if (!IsValid(ItemInstance))
	{
		return;
	}

	if (InventoryItems.Remove(ItemInstance) <= 0)
	{
		return;
	}

	InventoryItemCache.RemoveItem(ItemInstance);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	BroadcastInventoryItemChanged(EPRInventoryChangeReason::ItemRemoved, ItemInstance);
}

void UPRInventoryComponent::RebuildInventoryCaches()
{
	InventoryItemCache.Rebuild(InventoryItems);
}

void UPRInventoryComponent::BroadcastInventoryItemChanged(EPRInventoryChangeReason ChangeReason, UPRItemInstance* ItemInstance)
{
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = ChangeReason;
	EventData.ItemInstance = ItemInstance;

	OnInventoryChanged(EventData);
}

UPRWeaponManagerComponent* UPRInventoryComponent::ResolveOwnerWeaponManager() const
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return nullptr;
	}

	AController* OwnerController = Cast<AController>(OwnerActor->GetOwner());
	if (!IsValid(OwnerController))
	{
		return nullptr;
	}
	
	if (APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(OwnerController->GetPawn()))
	{
		return Player->GetWeaponManager();
	}
	
	return nullptr;
}

void UPRInventoryComponent::NotifyWeaponItemModChanged(UPRItemInstance_Weapon* WeaponItem)
{
	// 잘못된 Weapon Item 요청이면 런타임 반응을 전달하지 않는다
	if (!IsValid(WeaponItem))
	{
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = ResolveOwnerWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return;
	}

	// WeaponManager는 현재 장착 중인 무기만 반응하고 인벤토리 정본 변경은 관여하지 않는다
	WeaponManager->HandleInventoryWeaponModChanged(WeaponItem);
}

void UPRInventoryComponent::Server_RequestAddItem_Implementation(UPRItemDataAsset* InItemData, int32 Amount)
{
	AddItem<UPRItemInstance>(InItemData, Amount);
}

void UPRInventoryComponent::Server_RequestRemoveItem_Implementation(UPRItemInstance* ItemInstance, int32 RemoveCount)
{
	RemoveItemInternal(ItemInstance, RemoveCount);
}

void UPRInventoryComponent::Server_RequestRemoveItemByData_Implementation(UPRItemDataAsset* ItemData, int32 RemoveCount)
{
	RemoveItemByDataInternal(ItemData, RemoveCount);
}

void UPRInventoryComponent::Server_RequestActivateItem_Implementation(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	ActivateItemInternal(ItemInstance, ActivationContext);
}

void UPRInventoryComponent::Server_RequestDeactivateItem_Implementation(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext)
{
	DeactivateItemInternal(ItemInstance, ActivationContext);
}

void UPRInventoryComponent::Server_RequestUseConsumableItem_Implementation(UPRItemInstance_Consumable* ConsumableItem)
{
	UseConsumableItemInternal(ConsumableItem);
}

void UPRInventoryComponent::Server_RequestEquipModItemToWeapon_Implementation(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	EquipModItemToWeapon(ModItem, TargetWeaponItem);
}

void UPRInventoryComponent::Server_RequestUnequipModFromWeapon_Implementation(UPRItemInstance_Weapon* TargetWeaponItem)
{
	UnequipModFromWeapon(TargetWeaponItem);
}
