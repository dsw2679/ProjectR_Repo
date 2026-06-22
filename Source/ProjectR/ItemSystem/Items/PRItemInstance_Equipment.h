// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Item 인스턴스 Equipment 구현)
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRItemInstance_Equipment.generated.h"

class UPREquipmentDataAsset;
class AActor;

// 인벤토리가 소유하는 장비 아이템 1종의 인스턴스를 관리
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Equipment : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UPRItemInstance Interface ~*/
	virtual bool ActivateItem(const FPRItemActivationContext& ActivationContext) override;
	virtual bool DeactivateItem(const FPRItemActivationContext& ActivationContext) override;

public:
	// 현재 연결된 장비 아이템 데이터 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Equipment")
	UPREquipmentDataAsset* GetEquipmentData() const;

	// 장비가 장착되는 슬롯 종류 반환
	EPREquipmentSlotType GetSlotType() const;

	// 장착 시 호출 (현재는 디버그 메시지, 추후 ChildMesh 변경)
	virtual void OnEquipped(AActor* OwnerActor);

	// 해제 시 호출 (현재는 디버그 메시지, 추후 ChildMesh 복원)
	virtual void OnUnequipped(AActor* OwnerActor);

	// 세이브 데이터에 현재 상태 기록
	void FillSaveEntry(FPREquipmentSlotSaveEntry& OutEntry) const;

	// 세이브 데이터에서 상태 복원 (현재는 특별히 복원할 내부 상태 없음)
	void ApplySaveEntry(const FPREquipmentSlotSaveEntry& InEntry);

protected:
	// 장비 장착 시 공용 수치 GE와 추가 GA/GE를 부여
	void GrantEquippedEffects(AActor* OwnerActor);

	// 장비 해제 시 장착으로 부여한 공용 수치 GE와 추가 GA/GE를 회수
	void ClearEquippedEffects(AActor* OwnerActor);

	// 공용 장비 수치 GE를 적용
	void ApplyCommonStatEffect(AActor* OwnerActor);

	// 공용 장비 수치 GE를 회수
	void RemoveCommonStatEffect(AActor* OwnerActor);

public:
	// 장비 추가 GA/GE로 부여한 핸들
	UPROPERTY(Transient)
	FPRAbilitySetHandles EquipmentAbilityHandles;

	// 장비 공용 수치 GE 핸들
	UPROPERTY(Transient)
	FActiveGameplayEffectHandle CommonStatEffectHandle;

	// 장비 슬롯 장착 여부
	UPROPERTY(Transient)
	bool bIsEquippedEquipmentSlot = false;
};
