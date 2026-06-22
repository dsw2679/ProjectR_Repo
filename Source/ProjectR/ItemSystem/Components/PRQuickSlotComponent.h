// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (퀵슬롯 슬롯 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Types/PRQuickSlotTypes.h"
#include "PRQuickSlotComponent.generated.h"

class UPRConsumableDataAsset;
class UPRItemInstance_Consumable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRQuickSlotChangedSignature, UPRQuickSlotComponent*, QuickSlotComponent, int32, SlotIndex);

// 소비 아이템 퀵슬롯 등록 상태와 입력 사용 요청을 관리한다
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRQuickSlotComponent();

	/*~ UActorComponent Interface ~*/
public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 인벤토리 컴포넌트를 연결하고 퀵슬롯 캐시를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void InitializeQuickSlots(UPRInventoryComponent* InInventoryComponent);

	// 지정 슬롯에 소비 아이템 종류 등록을 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void RequestRegisterQuickSlotItem(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData);

	// 지정 슬롯의 등록 해제를 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void RequestClearQuickSlotItem(int32 SlotIndex);

	// 지정 슬롯의 소비 아이템 사용을 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|QuickSlot")
	void RequestUseQuickSlot(int32 SlotIndex);

	// 지정 슬롯의 표시 데이터를 만든다
	UFUNCTION(BlueprintPure, Category = "ProjectR|QuickSlot")
	FPRQuickSlotViewData BuildQuickSlotViewData(int32 SlotIndex) const;

	// 전체 퀵슬롯 표시 데이터를 만든다
	UFUNCTION(BlueprintPure, Category = "ProjectR|QuickSlot")
	TArray<FPRQuickSlotViewData> BuildQuickSlotViewDataList() const;

	// 퀵슬롯 변경 델리게이트를 반환한다
	FPRQuickSlotChangedSignature& GetOnQuickSlotChanged() { return OnQuickSlotChanged; }

	// 지정 슬롯의 소비 아이템 인스턴스 캐시를 갱신한다
	void RefreshCachedConsumableItem(int32 SlotIndex);

	// 전체 슬롯의 소비 아이템 인스턴스 캐시를 갱신한다
	void RefreshAllCachedConsumableItems();
	
	int32 GetMaxQuickSlotCount() const {return MaxQuickSlotCount;}
	int32 GetUsingQuickSlotCount() const;
	bool IsRegisteredItem(UPRConsumableDataAsset* InConsumableData);

	// 현재 퀵슬롯 등록 상태 저장 데이터
	FPRQuickSlotSaveData MakeSaveData() const;

	// 저장 데이터 기반 퀵슬롯 등록 상태 복원
	void ApplySaveData(const FPRQuickSlotSaveData& InSaveData);

	// 리스폰 전 퀵슬롯 소비 아이템 캐시 재조회
	void ResetSystem();
protected:
	// 퀵슬롯 복제 결과를 로컬 UI에 알린다
	UFUNCTION()
	void OnRep_QuickSlots();

	// 인벤토리 변경 시 캐시를 다시 맞춘다
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData);

	// 지정 슬롯에 소비 아이템 종류를 등록한다
	bool RegisterQuickSlotItemInternal(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData);

	// 지정 슬롯 등록을 해제한다
	bool ClearQuickSlotItemInternal(int32 SlotIndex);

	// 지정 슬롯의 소비 아이템을 사용한다
	bool UseQuickSlotInternal(int32 SlotIndex);

	// 현재 소유자의 인벤토리 컴포넌트를 조회한다
	UPRInventoryComponent* ResolveInventoryComponent() const;

	// 슬롯 인덱스가 유효한지 확인한다
	bool IsValidSlotIndex(int32 SlotIndex) const;

	// 클라이언트 요청을 서버의 퀵슬롯 등록 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestRegisterQuickSlotItem(int32 SlotIndex, UPRConsumableDataAsset* ConsumableData);

	// 클라이언트 요청을 서버의 퀵슬롯 해제 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestClearQuickSlotItem(int32 SlotIndex);

	// 클라이언트 요청을 서버의 퀵슬롯 사용 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestUseQuickSlot(int32 SlotIndex);

public:
	// 퀵슬롯 칸 수
	static constexpr int32 MaxQuickSlotCount = 4;

protected:
	// 등록된 퀵슬롯 목록이다
	UPROPERTY(ReplicatedUsing=OnRep_QuickSlots, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TArray<FPRQuickSlotEntry> QuickSlots;
	
	// 초기 고정 퀵슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|QuickSlot")
	TArray<UPRConsumableDataAsset*> PrimaryItems;
	
private:
	// 현재 연결된 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryComponent> CachedInventoryComponent = nullptr;

	// 퀵슬롯 등록 또는 표시 상태가 변경되었을 때 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|QuickSlot", meta = (AllowPrivateAccess = "true"))
	FPRQuickSlotChangedSignature OnQuickSlotChanged;
};
