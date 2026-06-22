// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (퀵슬롯 슬롯 컴포넌트 구현)
#include "PRQuickSlotComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"

UPRQuickSlotComponent::UPRQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	QuickSlots.SetNum(MaxQuickSlotCount);
}

void UPRQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots(ResolveInventoryComponent());
}

void UPRQuickSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRQuickSlotComponent, QuickSlots, COND_OwnerOnly);
}

void UPRQuickSlotComponent::InitializeQuickSlots(UPRInventoryComponent* InInventoryComponent)
{
	int32 PrimaryIndex = 0;
	for (UPRConsumableDataAsset* PrimaryItem : PrimaryItems)
	{
		if (IsValid(PrimaryItem))
		{
			RegisterQuickSlotItemInternal(PrimaryIndex, PrimaryItem);
			PrimaryIndex++;
		}
	}
	
	CachedInventoryComponent = InInventoryComponent;

	if (QuickSlots.Num() != MaxQuickSlotCount)
	{
		QuickSlots.SetNum(MaxQuickSlotCount);
	}

	if (IsValid(CachedInventoryComponent))
	{
		CachedInventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRQuickSlotComponent::HandleInventoryChanged);
		CachedInventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRQuickSlotComponent::HandleInventoryChanged);
	}

	RefreshAllCachedConsumableItems();
	OnQuickSlotChanged.Broadcast(this, INDEX_NONE);
}

void UPRQuickSlotComponent::RequestRegisterQuickSlotItem(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData)
{
	// 잘못된 슬롯 또는 데이터 요청은 서버 RPC 전송 전에 중단
	if (!IsValidSlotIndex(SlotIndex) || !IsValid(ConsumableData))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		RegisterQuickSlotItemInternal(SlotIndex, ConsumableData);
		return;
	}

	Server_RequestRegisterQuickSlotItem(SlotIndex, ConsumableData);
}

void UPRQuickSlotComponent::RequestClearQuickSlotItem(int32 SlotIndex)
{
	// 잘못된 슬롯 요청은 서버 RPC 전송 전에 중단
	if (!IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		ClearQuickSlotItemInternal(SlotIndex);
		return;
	}

	Server_RequestClearQuickSlotItem(SlotIndex);
}

void UPRQuickSlotComponent::RequestUseQuickSlot(int32 SlotIndex)
{
	// 잘못된 슬롯 요청은 서버 RPC 전송 전에 중단
	if (!IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UseQuickSlotInternal(SlotIndex);
		return;
	}

	Server_RequestUseQuickSlot(SlotIndex);
}

FPRQuickSlotViewData UPRQuickSlotComponent::BuildQuickSlotViewData(int32 SlotIndex) const
{
	FPRQuickSlotViewData ViewData;
	ViewData.SlotIndex = SlotIndex;

	if (!QuickSlots.IsValidIndex(SlotIndex))
	{
		return ViewData;
	}

	const FPRQuickSlotEntry& Entry = QuickSlots[SlotIndex];
	ViewData.ConsumableData = Entry.ConsumableData;
	ViewData.ConsumableItem = Entry.CachedConsumableItem;
	ViewData.bHasRegisteredItem = IsValid(Entry.ConsumableData);

	if (IsValid(Entry.ConsumableData))
	{
		ViewData.Icon = Entry.ConsumableData->GetIcon();
	}

	if (IsValid(Entry.CachedConsumableItem))
	{
		ViewData.StackCount = Entry.CachedConsumableItem->GetStackCount();
		ViewData.bUsable = Entry.CachedConsumableItem->HasAnyStack();
	}

	return ViewData;
}

TArray<FPRQuickSlotViewData> UPRQuickSlotComponent::BuildQuickSlotViewDataList() const
{
	TArray<FPRQuickSlotViewData> ViewDataList;
	ViewDataList.Reserve(MaxQuickSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlotCount; ++SlotIndex)
	{
		ViewDataList.Add(BuildQuickSlotViewData(SlotIndex));
	}

	return ViewDataList;
}

int32 UPRQuickSlotComponent::GetUsingQuickSlotCount() const
{
	int32 Ret = 0;
	
	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlotCount; ++SlotIndex)
	{
		if (IsValid(QuickSlots[SlotIndex].ConsumableData))
		{
			Ret++;
		}
	}
	
	return Ret;
}

bool UPRQuickSlotComponent::IsRegisteredItem(UPRConsumableDataAsset* InConsumableData)
{
	for (auto& QuickSlotItem : QuickSlots)
	{
		if (QuickSlotItem.ConsumableData == InConsumableData)
		{
			return true;
		}
	}
	return false;
}

FPRQuickSlotSaveData UPRQuickSlotComponent::MakeSaveData() const
{
	FPRQuickSlotSaveData SaveData;
	SaveData.RegisteredConsumables.SetNum(MaxQuickSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlotCount; ++SlotIndex)
	{
		if (!QuickSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		SaveData.RegisteredConsumables[SlotIndex] = QuickSlots[SlotIndex].ConsumableData;
	}

	return SaveData;
}

void UPRQuickSlotComponent::ApplySaveData(const FPRQuickSlotSaveData& InSaveData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	CachedInventoryComponent = ResolveInventoryComponent();
	QuickSlots.SetNum(MaxQuickSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlotCount; ++SlotIndex)
	{
		UPRConsumableDataAsset* ConsumableData = InSaveData.RegisteredConsumables.IsValidIndex(SlotIndex)
			? InSaveData.RegisteredConsumables[SlotIndex].LoadSynchronous()
			: nullptr;

		// 슬롯 등록 상태 복원
		QuickSlots[SlotIndex].ConsumableData = ConsumableData;
		QuickSlots[SlotIndex].CachedConsumableItem = nullptr;
		RefreshCachedConsumableItem(SlotIndex);
	}

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	OnQuickSlotChanged.Broadcast(this, INDEX_NONE);
}

void UPRQuickSlotComponent::ResetSystem()
{
	// 소비 아이템 인스턴스 캐시 재조회
	CachedInventoryComponent = ResolveInventoryComponent();
	RefreshAllCachedConsumableItems();
	OnQuickSlotChanged.Broadcast(this, INDEX_NONE);
}

void UPRQuickSlotComponent::OnRep_QuickSlots()
{
	InitializeQuickSlots(ResolveInventoryComponent());
}

void UPRQuickSlotComponent::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent,
	const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != CachedInventoryComponent)
	{
		return;
	}

	RefreshAllCachedConsumableItems();
	OnQuickSlotChanged.Broadcast(this, INDEX_NONE);
}
bool UPRQuickSlotComponent::RegisterQuickSlotItemInternal(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValidSlotIndex(SlotIndex) || !IsValid(ConsumableData))
	{
		return false;
	}

	QuickSlots[SlotIndex].ConsumableData = ConsumableData;
	RefreshCachedConsumableItem(SlotIndex);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	OnQuickSlotChanged.Broadcast(this, SlotIndex);
	return true;
}

bool UPRQuickSlotComponent::ClearQuickSlotItemInternal(int32 SlotIndex)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValidSlotIndex(SlotIndex))
	{
		return false;
	}

	QuickSlots[SlotIndex].ConsumableData = nullptr;
	QuickSlots[SlotIndex].CachedConsumableItem = nullptr;

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	OnQuickSlotChanged.Broadcast(this, SlotIndex);
	return true;
}

bool UPRQuickSlotComponent::UseQuickSlotInternal(int32 SlotIndex)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValidSlotIndex(SlotIndex))
	{
		return false;
	}

	RefreshCachedConsumableItem(SlotIndex);

	const FPRQuickSlotEntry& Entry = QuickSlots[SlotIndex];
	if (!IsValid(Entry.ConsumableData) || !IsValid(Entry.CachedConsumableItem) || !IsValid(CachedInventoryComponent))
	{
		return false;
	}

	AActor* UserActor = GetOwner();
	if (!IsValid(UserActor))
	{
		return false;
	}
	
	Entry.CachedConsumableItem->UseItem(UserActor);
	RefreshCachedConsumableItem(SlotIndex);
	OnQuickSlotChanged.Broadcast(this, SlotIndex);
	return true;
}

void UPRQuickSlotComponent::RefreshCachedConsumableItem(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	FPRQuickSlotEntry& Entry = QuickSlots[SlotIndex];
	Entry.CachedConsumableItem = nullptr;

	if (!IsValid(Entry.ConsumableData))
	{
		return;
	}

	if (!IsValid(CachedInventoryComponent))
	{
		CachedInventoryComponent = ResolveInventoryComponent();
	}

	if (IsValid(CachedInventoryComponent))
	{
		Entry.CachedConsumableItem = CachedInventoryComponent->FindItemByData<UPRItemInstance_Consumable>(Entry.ConsumableData);
	}
}

void UPRQuickSlotComponent::RefreshAllCachedConsumableItems()
{
	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlotCount; ++SlotIndex)
	{
		RefreshCachedConsumableItem(SlotIndex);
	}
}

UPRInventoryComponent* UPRQuickSlotComponent::ResolveInventoryComponent() const
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return nullptr;
	}

	return OwnerActor->FindComponentByClass<UPRInventoryComponent>();
}

bool UPRQuickSlotComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < MaxQuickSlotCount;
}

void UPRQuickSlotComponent::Server_RequestRegisterQuickSlotItem_Implementation(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData)
{
	RegisterQuickSlotItemInternal(SlotIndex, ConsumableData);
}

void UPRQuickSlotComponent::Server_RequestClearQuickSlotItem_Implementation(int32 SlotIndex)
{
	ClearQuickSlotItemInternal(SlotIndex);
}

void UPRQuickSlotComponent::Server_RequestUseQuickSlot_Implementation(int32 SlotIndex)
{
	UseQuickSlotInternal(SlotIndex);
}
