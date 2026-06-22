// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 게이지 및 스택 상태 보존 구현)
// Author: 이건주 (무기 Mod 장착 시 특수 이펙트 및 HUD 상태 동기화 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "PRItemInstance_Mod.generated.h"

class UPRItemInstance_Weapon;
class UPRWeaponModDataAsset;
struct FPRModItemSaveEntry;

// 인벤토리가 소유하는 무기 Mod 1개의 지속 인스턴스다
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Mod : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UPRItemInstance Interface ~*/
	virtual bool ActivateItem(const FPRItemActivationContext& ActivationContext) override;
	virtual bool DeactivateItem(const FPRItemActivationContext& ActivationContext) override;
public:
	virtual void InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount) override;

	// 현재 연결된 Mod 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponModDataAsset* GetModData() const;

	// Mod 인스턴스 상태 저장 엔트리 작성
	void FillSaveEntry(FPRModItemSaveEntry& OutEntry) const;

	// 저장 엔트리 기반 인스턴스 상태 복원
	void ApplySaveEntry(const FPRModItemSaveEntry& InEntry);

	// 입력 데이터가 현재 Mod와 같은지 확인한다
	bool MatchesModData(const UPRWeaponModDataAsset* InModData) const;

	// 현재 다른 무기 Item에 장착되어 있는지 확인한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	bool IsEquipped() const;

	// 현재 이 Mod를 장착 중인 무기 Item을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRItemInstance_Weapon* GetEquippedWeaponItem() const { return EquippedWeaponItem; }

	// 지정 무기 Item에 장착 가능한 상태인지 확인한다
	bool CanEquipToWeaponItem(const UPRItemInstance_Weapon* WeaponItem) const;

	// 지정 무기 Item에 장착된 상태로 표시한다
	void MarkEquippedToWeaponItem(UPRItemInstance_Weapon* WeaponItem);

	// 장착 중인 무기 Item 연결을 해제한다
	void ClearEquippedWeaponItem();

private:
	// Mod 데이터 복제 완료 시 클라이언트 확인 로그를 남긴다
	UFUNCTION()
	void OnRep_ModData();

	// 장착 대상 복제 완료 시 클라이언트 확인 로그를 남긴다
	UFUNCTION()
	void OnRep_EquippedWeaponItem();

public:
	// 이 Mod를 장착 중인 무기 Item
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponItem, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRItemInstance_Weapon> EquippedWeaponItem = nullptr;
};
