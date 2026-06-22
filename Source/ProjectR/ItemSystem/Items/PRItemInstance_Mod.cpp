// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 게이지 및 스택 상태 보존 구현)
// Author: 이건주 (무기 Mod 장착 시 특수 이펙트 및 HUD 상태 동기화 구현)
#include "PRItemInstance_Mod.h"

#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

void UPRItemInstance_Mod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPRItemInstance_Mod, EquippedWeaponItem);
}

bool UPRItemInstance_Mod::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	// Mod 선택 목록은 어떤 무기에 장착할지 LastFocusedItem으로 기억한 뒤 ContextObject로 넘김
	// 이 함수는 그 대상 무기를 꺼내어 InventoryComponent의 Mod 장착 처리로 연결함
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// InventoryComponent는 서버에서 요청 Item 소유권을 확인한 뒤 RuntimeContext에 다시 채워 넣음
	// Mod 장착은 같은 인벤토리 안의 Mod Item과 Weapon Item 사이에서만 허용됨
	if (!IsValid(ActivationContext.InventoryComponent))
	{
		return false;
	}

	// Mod Item 자체에는 이번 UI 선택에서 목표로 한 무기 슬롯 정보가 없음
	// 그래서 Mod 목록을 열 때 선택한 무기 ItemInstance를 ContextObject로 반드시 전달해야 함
	UPRItemInstance_Weapon* TargetWeaponItem = Cast<UPRItemInstance_Weapon>(ActivationContext.ContextObject);
	if (!IsValid(TargetWeaponItem))
	{
		return false;
	}

	return ActivationContext.InventoryComponent->EquipModItemToWeapon(this, TargetWeaponItem);
}

bool UPRItemInstance_Mod::DeactivateItem(const FPRItemActivationContext& ActivationContext)
{
	// Mod 해제 항목은 표시만 명령처럼 보이고 실제로는 장착된 Mod ItemInstance를 비활성화함
	// 장착 중 Mod 항목을 클릭한 경우와 해제 항목을 클릭한 경우 모두 이 함수로 모임
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// InventoryComponent는 서버에서 요청 Item 소유권을 확인한 뒤 RuntimeContext에 다시 채워 넣음
	// 실제 양방향 연결 해제와 무기 런타임 갱신은 UnequipModFromWeapon 안에서 처리됨
	if (!IsValid(ActivationContext.InventoryComponent))
	{
		return false;
	}

	// UI가 어떤 무기의 Mod 목록에서 해제했는지 알 수 있으면 그 무기를 우선 사용함
	// ContextObject가 비어 있으면 Mod Item이 복제 상태로 들고 있는 기존 장착 무기로 보정함
	UPRItemInstance_Weapon* TargetWeaponItem = Cast<UPRItemInstance_Weapon>(ActivationContext.ContextObject);
	if (!IsValid(TargetWeaponItem))
	{
		TargetWeaponItem = EquippedWeaponItem.Get();
	}

	if (!IsValid(TargetWeaponItem))
	{
		return false;
	}

	return ActivationContext.InventoryComponent->UnequipModFromWeapon(TargetWeaponItem);
}

void UPRItemInstance_Mod::InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount)
{
	Super::InitializeItem(InItemData, InitialStackCount);
	ClearEquippedWeaponItem();
}

UPRWeaponModDataAsset* UPRItemInstance_Mod::GetModData() const
{
	return Cast<UPRWeaponModDataAsset>(ItemData);
}

void UPRItemInstance_Mod::FillSaveEntry(FPRModItemSaveEntry& OutEntry) const
{
	// 저장 엔트리 기본값
	OutEntry = FPRModItemSaveEntry();
	OutEntry.ModData = GetModData();
}

void UPRItemInstance_Mod::ApplySaveEntry(const FPRModItemSaveEntry& InEntry)
{
}

bool UPRItemInstance_Mod::MatchesModData(const UPRWeaponModDataAsset* InModData) const
{
	return GetModData() == InModData;
}

bool UPRItemInstance_Mod::IsEquipped() const
{
	return IsValid(EquippedWeaponItem);
}

bool UPRItemInstance_Mod::CanEquipToWeaponItem(const UPRItemInstance_Weapon* WeaponItem) const
{
	if (!IsValid(WeaponItem))
	{
		return false;
	}

	return !IsEquipped() || EquippedWeaponItem == WeaponItem;
}

void UPRItemInstance_Mod::MarkEquippedToWeaponItem(UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(WeaponItem))
	{
		return;
	}

	EquippedWeaponItem = WeaponItem;
}

void UPRItemInstance_Mod::ClearEquippedWeaponItem()
{
	EquippedWeaponItem = nullptr;
}

void UPRItemInstance_Mod::OnRep_ModData()
{
	// 클라이언트에서 Mod 데이터 복제 결과를 Item 참조 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item data replicated. Item = %s | Mod = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetModData()));
}

void UPRItemInstance_Mod::OnRep_EquippedWeaponItem()
{
	// 클라이언트에서 Mod Item의 장착 대상 변경을 Item 참조 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item equipped weapon replicated. Item = %s | Mod = %s | EquippedWeaponItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetModData()),
		*GetNameSafe(EquippedWeaponItem.Get()));

	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}
