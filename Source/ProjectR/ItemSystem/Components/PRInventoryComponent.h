// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 강화 재료 및 재화 인벤토리 소모 연동 구현)
// Author: 배유찬 (인벤토리 기초 구조 설계 및 획득 아이템 인벤토리 보관 연동 구현)
// Author: 이건주 (아이템 퀵슬롯 HUD, 무기 Mod 장착 슬롯 및 3D 프리뷰 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "PRInventoryComponent.generated.h"

class UPRItemDataAsset;
class UPRConsumableDataAsset;
class UPRInventoryComponent;
class UPRWeaponManagerComponent;
class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;
class UPRMaterialDataAsset;
class AActor;
class UActorChannel;
class FOutBunch;
struct FReplicationFlags;

// 인벤토리 변경 신호의 원인
UENUM(BlueprintType)
enum class EPRInventoryChangeReason : uint8
{
	// 아이템 목록이 변경되었다
	ItemAdded,
	ItemRemoved,
	// Mod 장착 상태가 변경되었다
	ModEquipChanged,
	// 무기 강화 단계가 변경되었다
	WeaponUpgradeChanged,
	// 아이템 보유 개수가 변경되었다
	StackChanged,
	// 인벤토리 전체 갱신
	BulkRestored,
	// 장비 장착 상태 변경
	EquipmentChanged
};

USTRUCT(BlueprintType)
struct FPRInventoryChangeEventData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	EPRInventoryChangeReason ChangeReason;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UPRItemInstance> ItemInstance;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRInventoryChangedSignature, UPRInventoryComponent*, InventoryComponent, const FPRInventoryChangeEventData&, EventData);

// 인벤토리 Item 참조 목록
USTRUCT()
struct FPRInventoryItemList
{
	GENERATED_BODY()

	// 캐시된 Item 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRItemInstance>> Items;
};

// 인벤토리 정본 배열에서 파생된 조회용 캐시다
USTRUCT()
struct FPRInventoryItemCache
{
	GENERATED_BODY()

	// 모든 캐시 데이터를 비운다
	void Reset();

	// 정본 배열 기준으로 캐시를 다시 만든다
	void Rebuild(const TArray<UPRItemInstance*>& SourceItems);

	// 단일 Item을 캐시에 추가
	void AddItem(UPRItemInstance* Item);

	// 단일 Item을 캐시에서 제거
	void RemoveItem(UPRItemInstance* Item);

	// Item 데이터로 첫 번째 Item을 찾는다
	UPRItemInstance* FindItemByData(const UPRItemDataAsset* ItemData) const;

	// Item 타입으로 Item 목록을 반환
	const FPRInventoryItemList* FindItemsByType(EPRItemType ItemType) const;

private:
	// Item 타입별 조회 캐시
	UPROPERTY(Transient)
	TMap<EPRItemType, FPRInventoryItemList> ItemsByType;

	// Item 데이터별 조회 캐시
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPRItemDataAsset>, FPRInventoryItemList> ItemsByData;
};

// 플레이어가 소유한 Item 인스턴스의 정본 컨테이너다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInventoryComponent();

	/*~ UActorComponent Interface ~*/
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

public:
	
	// Item 데이터 타입에 맞는 추가 요청을 서버 권위 경로로 전달
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestAddItem(UPRItemDataAsset* InItemData, int32 Amount = 1);
	
	// Item 추가
	UPRItemInstance* AddItem(UPRItemDataAsset* InItemData, int32 Amount = 1);
	
	// Item 데이터 타입 기반 추가
	template<typename InstanceType>
	InstanceType* AddItem(UPRItemDataAsset* InItemData, int32 Amount = 1);

	// BP Item 클래스 기반 추가
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory", meta = (DeterminesOutputType = "ItemInstanceClass"))
	UPRItemInstance* BP_AddItem(UPRItemDataAsset* InItemData, TSubclassOf<UPRItemInstance> ItemInstanceClass, int32 Amount = 1);

	// Item 제거를 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestRemoveItem(UPRItemInstance* ItemInstance, int32 RemoveCount);

	// Item 데이터 기반 제거를 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestRemoveItemByData(UPRItemDataAsset* ItemData, int32 RemoveCount);

	// Item 데이터 기반 보유 개수 감소
	bool RemoveItemByData(UPRItemDataAsset* ItemData, int32 RemoveCount);

	// Item 활성화를 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestActivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// Item 비활성화를 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestDeactivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// 소비 Item 사용을 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestUseConsumableItem(UPRItemInstance_Consumable* ConsumableItem);

	// Mod Item을 지정 무기 Item에 장착하도록 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 지정 무기 Item의 Mod Item 장착을 해제하도록 서버 권위 경로로 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

	// Mod Item을 지정 무기 Item에 장착
	bool EquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 지정 무기 Item의 Mod Item 장착을 해제
	bool UnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

	// Item 소유 여부 확인
	bool OwnsItem(const UPRItemInstance* ItemInstance) const;

	// Item 데이터 기반 조회
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	UPRItemInstance* FindItemByData(const UPRItemDataAsset* InItemData) const;
	
	// Item 데이터 기반 지정 타입 조회
	template<typename InstanceType>
	InstanceType* FindItemByData(const UPRItemDataAsset* InItemData) const;

	// Item 타입 기반 목록 조회
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	TArray<UPRItemInstance*> GetItemsByType(EPRItemType ItemType) const;

	// Item 타입 기반 지정 타입 목록 조회
	template<typename InstanceType>
	TArray<InstanceType*> GetItemsByType(EPRItemType ItemType) const;

	// Item 타입 기반 인덱스 조회
	int32 GetItemIndexByType(const UPRItemInstance* ItemInstance, EPRItemType ItemType) const;

	// Item 타입 기반 지정 타입 인덱스 조회
	template<typename InstanceType>
	InstanceType* GetItemAtIndexByType(EPRItemType ItemType, int32 ItemIndex) const;

	// 인벤토리 변경 이벤트를 발행
	void OnInventoryChanged(const FPRInventoryChangeEventData& EventData);

	// 인벤토리 변경 델리게이트를 반환
	FPRInventoryChangedSignature& GetOnInventoryChanged() { return OnInventoryChangedDelegate; }

	// 현재 인벤토리 상태 저장 데이터
	FPRInventorySaveData MakeSaveData() const;

	// 저장 데이터 기반 인벤토리 복원
	void ApplySaveData(const FPRInventorySaveData& InSaveData);

	// 리스폰 전 인벤토리 조회 캐시 재구성
	void ResetSystem();
	
protected:
	// 클라이언트에서 Item 목록 복제 결과를 확인
	UFUNCTION()
	void OnRep_InventoryItems(const TArray<UPRItemInstance*>& OldInventoryItems);

	// Item 보유 개수 감소
	bool RemoveItemInternal(UPRItemInstance* ItemInstance, int32 RemoveCount);

	// Item 데이터 기반 보유 개수 감소
	bool RemoveItemByDataInternal(UPRItemDataAsset* ItemData, int32 RemoveCount);

	// Item 활성화
	bool ActivateItemInternal(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// Item 비활성화
	bool DeactivateItemInternal(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// 소비 Item 사용
	bool UseConsumableItemInternal(UPRItemInstance_Consumable* ConsumableItem);

	// 클라이언트 요청을 서버의 Item 추가 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestAddItem(UPRItemDataAsset* InItemData, int32 Amount);

	// 클라이언트 요청을 서버의 Item 제거 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestRemoveItem(UPRItemInstance* ItemInstance, int32 RemoveCount);

	// 클라이언트 요청을 서버의 Item 데이터 기반 제거 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestRemoveItemByData(UPRItemDataAsset* ItemData, int32 RemoveCount);

	// 클라이언트 요청을 서버의 Item 활성화 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestActivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// 클라이언트 요청을 서버의 Item 비활성화 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestDeactivateItem(UPRItemInstance* ItemInstance, const FPRItemActivationContext& ActivationContext);

	// 클라이언트 요청을 서버의 소비 Item 사용 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestUseConsumableItem(UPRItemInstance_Consumable* ConsumableItem);

	// 클라이언트 Mod 장착 요청을 서버의 인벤토리 장착 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 클라이언트 Mod 해제 요청을 서버의 인벤토리 해제 처리로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

private:
	// Item 데이터 타입 기반 추가
	UPRItemInstance* AddItemInternal(UPRItemDataAsset* InItemData, int32 Amount);

	// 현재 인벤토리에 Item 인스턴스를 등록
	void RegisterInventoryItem(UPRItemInstance* ItemInstance);

	// 현재 인벤토리에서 Item 인스턴스를 등록 해제
	void UnregisterInventoryItem(UPRItemInstance* ItemInstance);

	// 정본 배열 기준 조회 캐시 재생성
	void RebuildInventoryCaches();

	// Item 변경 이벤트를 발행
	void BroadcastInventoryItemChanged(EPRInventoryChangeReason ChangeReason, UPRItemInstance* ItemInstance);

	// 현재 인벤토리 소유자의 무기 매니저 컴포넌트를 조회
	UPRWeaponManagerComponent* ResolveOwnerWeaponManager() const;

	// 현재 장착 중인 무기라면 런타임 Mod 변경 반응을 전달
	void NotifyWeaponItemModChanged(UPRItemInstance_Weapon* WeaponItem);

public:
	// 현재 인벤토리가 소유한 모든 Item 목록
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TArray<UPRItemInstance*> InventoryItems;

private:
	// 정본 배열 기반 조회 캐시
	UPROPERTY(Transient)
	FPRInventoryItemCache InventoryItemCache;

	// 인벤토리 목록 또는 Item 장착 상태가 변경되었을 때 알린다
	FPRInventoryChangedSignature OnInventoryChangedDelegate;
};

template<typename InstanceType>
InstanceType* UPRInventoryComponent::AddItem(UPRItemDataAsset* InItemData, int32 Amount)
{
	static_assert(TIsDerivedFrom<InstanceType, UPRItemInstance>::IsDerived, "InstanceType must derive from UPRItemInstance");

	return Cast<InstanceType>(AddItem(InItemData, Amount));
}

template<typename InstanceType>
InstanceType* UPRInventoryComponent::FindItemByData(const UPRItemDataAsset* InItemData) const
{
	static_assert(TIsDerivedFrom<InstanceType, UPRItemInstance>::IsDerived, "InstanceType must derive from UPRItemInstance");

	return Cast<InstanceType>(FindItemByData(InItemData));
}

template<typename InstanceType>
TArray<InstanceType*> UPRInventoryComponent::GetItemsByType(EPRItemType ItemType) const
{
	static_assert(TIsDerivedFrom<InstanceType, UPRItemInstance>::IsDerived, "InstanceType must derive from UPRItemInstance");

	TArray<InstanceType*> TypedItems;
	const TArray<UPRItemInstance*> Items = GetItemsByType(ItemType);
	TypedItems.Reserve(Items.Num());

	for (UPRItemInstance* Item : Items)
	{
		if (InstanceType* TypedItem = Cast<InstanceType>(Item))
		{
			TypedItems.Add(TypedItem);
		}
	}

	return TypedItems;
}

template<typename InstanceType>
InstanceType* UPRInventoryComponent::GetItemAtIndexByType(EPRItemType ItemType, int32 ItemIndex) const
{
	static_assert(TIsDerivedFrom<InstanceType, UPRItemInstance>::IsDerived, "InstanceType must derive from UPRItemInstance");

	const TArray<InstanceType*> TypedItems = GetItemsByType<InstanceType>(ItemType);
	return TypedItems.IsValidIndex(ItemIndex) ? TypedItems[ItemIndex] : nullptr;
}
