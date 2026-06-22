// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (장비 슬롯 제어 및 장착 장비 기반 캐릭터 외형 변경 구현)
// Author: 이건주 (인벤토리 연동 장비 동기화 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "PREquipmentManagerComponent.generated.h"

class UPRItemInstance_Equipment;
class UPREquipmentManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FPREquipmentChangedSignature,
	EPREquipmentSlotType, SlotType,
	UPRItemInstance_Equipment*, EquipmentItem);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPREquipmentVisualInfosChangedSignature,
	UPREquipmentManagerComponent*, EquipmentManagerComponent);

// 단일 장비 슬롯 정보
USTRUCT(BlueprintType)
struct FPREquippedItemEntry
{
	GENERATED_BODY()

	UPROPERTY()
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	UPROPERTY()
	TObjectPtr<UPRItemInstance_Equipment> EquipmentItem;
};

// 장착 아이템 배열 관리 래퍼 구조체
USTRUCT(BlueprintType)
struct FPREquipmentList
{
	GENERATED_BODY()

public:
	// 슬롯에 아이템 장착 (기존에 있으면 덮어씀)
	void EquipItem(EPREquipmentSlotType SlotType, UPRItemInstance_Equipment* Item);

	// 슬롯 장비 해제 (해제된 인스턴스 반환)
	UPRItemInstance_Equipment* UnequipSlot(EPREquipmentSlotType SlotType);

	// 슬롯별 장착 아이템 조회
	UPRItemInstance_Equipment* GetEquippedItem(EPREquipmentSlotType SlotType) const;

	// 슬롯 점유 여부 확인
	bool IsSlotOccupied(EPREquipmentSlotType SlotType) const;

	// 전체 아이템 엔트리 반환
	const TArray<FPREquippedItemEntry>& GetEntries() const { return Entries; }

	// 현재 활성화된 슬롯 목록 반환
	TArray<EPREquipmentSlotType> GetActiveSlots() const;

	// 전체 장착 엔트리 초기화
	void Clear();

private:
	UPROPERTY()
	TArray<FPREquippedItemEntry> Entries;
};

// 방어구와 악세서리 같은 비무기 장착 요소의 책임 경계를 잡는 컴포넌트다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREquipmentManagerComponent();

	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	// 현재 공개 비주얼 정보 배열 반환
	const TArray<FPRReplicatedEquipmentInfo>& GetEquippedVisualInfos() const { return ReplicatedEquipmentInfos; }

public:
	// 슬롯에 장비 아이템 장착 (서버 전용)
	bool EquipItem(UPRItemInstance_Equipment* EquipmentItem);

	// 슬롯 장비 해제 (서버 전용)
	bool UnequipSlot(EPREquipmentSlotType SlotType);

	// 슬롯별 장착 아이템 조회
	UFUNCTION(BlueprintPure, Category = "ProjectR|Equipment")
	UPRItemInstance_Equipment* GetEquippedItem(EPREquipmentSlotType SlotType) const;

	// 슬롯 점유 여부 확인
	UFUNCTION(BlueprintPure, Category = "ProjectR|Equipment")
	bool IsSlotOccupied(EPREquipmentSlotType SlotType) const;

	// 현재 비무기 장비 상태 저장 데이터 생성
	FPREquipmentSaveData MakeSaveData() const;

	// 저장 데이터 기반 비무기 장비 상태 복원
	void ApplySaveData(const FPREquipmentSaveData& InSaveData);

	// 리스폰 전 장비 외형 알림 재동기화
	void ResetSystem();

public:
	// 장비 슬롯 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Equipment")
	FPREquipmentChangedSignature OnEquipmentChanged;

	// 장비 외형 정보 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Equipment")
	FPREquipmentVisualInfosChangedSignature OnEquipmentVisualInfosChanged;

protected:
	// 장착 시각 정보 복제 알림
	UFUNCTION()
	void OnRep_EquipmentInfos();

private:
	// 장착 아이템 관리 구조체 (Owner 전용)
	UPROPERTY(Replicated, Transient)
	FPREquipmentList EquippedList;

	// 장착 장비의 시각적 공개 정보 배열 (모두에게 복제)
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentInfos, Transient)
	TArray<FPRReplicatedEquipmentInfo> ReplicatedEquipmentInfos;
};
