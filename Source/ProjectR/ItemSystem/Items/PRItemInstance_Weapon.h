// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (강화 등급별 무기 공격력 실시간 연산 구현)
// Author: 배유찬 (무기 런타임 탄약 상태 및 업그레이드 스탯 보존 구현)
// Author: 이건주 (무기 Mod 슬롯 상태 동기화 및 3D 모델 프리뷰 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRItemInstance_Weapon.generated.h"

class UPRAbilitySystemComponent;
class AActor;
class UPRItemInstance_Mod;
class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;
struct FPRWeaponItemSaveEntry;

// 인벤토리가 소유하는 무기 1개의 지속 인스턴스
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Weapon : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UPRItemInstance Interface ~*/
	virtual bool ActivateItem(const FPRItemActivationContext& ActivationContext) override;
	virtual bool DeactivateItem(const FPRItemActivationContext& ActivationContext) override;
public:
	// 무기와 초기 Mod 데이터를 연결한다
	virtual void InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount = 1) override;
	virtual void InitializeMod(UPRWeaponModDataAsset* InModData = nullptr);

	// 현재 연결된 무기 데이터를 반환한다
	UPRWeaponDataAsset* GetWeaponData() const;

	// 무기 인스턴스 상태 저장 엔트리 작성
	void FillSaveEntry(FPRWeaponItemSaveEntry& OutEntry) const;

	// 저장 엔트리 기반 인스턴스 상태 복원
	void ApplySaveEntry(const FPRWeaponItemSaveEntry& InEntry);

	// 현재 연결된 Mod 데이터를 반환한다
	UPRWeaponModDataAsset* GetModData() const { return ModData; }

	// 현재 장착된 Mod Item을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRItemInstance_Mod* GetEquippedModItem() const { return EquippedModItem; }

	// 현재 Mod Item이 장착되어 있는지 확인한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	bool HasEquippedModItem() const;

	// 무기 슬롯에 장착되어 있는지 확인한다
	bool IsEquippedWeaponSlot() const { return bIsEquippedWeaponSlot; }

	// 입력 데이터가 현재 무기와 같은지 확인한다
	bool MatchesWeaponData(const UPRWeaponDataAsset* InWeaponData) const;

	// 장착된 Mod 데이터를 교체한다
	void SetModData(UPRWeaponModDataAsset* NewModData);

	// 장착된 Mod Item을 교체한다
	void SetEquippedModItem(UPRItemInstance_Mod* NewModItem);

	// 장착된 Mod Item을 비운다
	void ClearEquippedModItem();

	// 무기 슬롯 장착 시점의 생명주기 훅
	void OnEquipped(AActor* OwnerActor);

	// 무기 슬롯 해제 시점의 생명주기 훅
	void OnUnequipped(AActor* OwnerActor);

	// Mod 교체 시점의 생명주기 훅
	void OnModChanged(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData);

	// 현재 시각 기준 남은 Mod 지속 시간을 계산한다
	float GetRemainingModDurationSeconds(float ServerWorldTimeSeconds) const;

	// 현재 강화 단계를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	// 서버 권위에서 강화 단계를 지정한다
	bool SetUpgradeLevel(int32 NewLevel);

	// 서버 권위에서 강화 단계를 1 증가시킨다
	bool IncreaseUpgradeLevel();

protected:
	// 무기 장착 시 기본 AbilitySet 부여 인터페이스
	void GrantEquippedAbilitySets(AActor* OwnerActor);

	// 무기 비활성화 시 기본 AbilitySet 회수 인터페이스
	void ClearEquippedAbilitySets(AActor* OwnerActor);

	// Mod 교체 시 Mod 어빌리티 재구성 인터페이스
	void RebuildModAbility(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData);

	// 무기 비활성화 시 임시 런타임 상태를 정리한다
	void ResetTransientRuntimeOnDeactivate();

	FGameplayTagContainer BuildCurrentSlotFireModeBlockTags(const UPRWeaponDataAsset* WeaponData, EPRWeaponFireModeState BlockedFireModeState);
	
	FGameplayTagContainer BuildOppositeSlotBlockTags(const UPRWeaponDataAsset* WeaponData);
private:
	// 장착 Mod 데이터 복제 완료 시 인벤토리 UI 갱신 신호를 발행한다
	UFUNCTION()
	void OnRep_ModData();

	// 장착 Mod Item 복제 완료 시 클라이언트 확인 로그를 남긴다
	UFUNCTION()
	void OnRep_EquippedModItem();

	// 강화 단계 복제 완료 시 인벤토리 UI 갱신 신호를 발행한다
	UFUNCTION()
	void OnRep_UpgradeLevel();

public:
	// 현재 장착된 Mod 데이터
	UPROPERTY(ReplicatedUsing = OnRep_ModData, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponModDataAsset> ModData = nullptr;

	// 현재 장착된 Mod Item
	UPROPERTY(ReplicatedUsing = OnRep_EquippedModItem, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRItemInstance_Mod> EquippedModItem = nullptr;

	// 무기 장착으로 부여한 어빌리티 핸들
	UPROPERTY(Transient)
	FPRAbilitySetHandles WeaponAbilityHandles;

	// Mod 장착으로 부여한 어빌리티와 효과 핸들
	UPROPERTY(Transient)
	FPRAbilitySetHandles ModAbilityHandles;

	// 무기 슬롯 장착 여부
	UPROPERTY(Transient)
	bool bIsEquippedWeaponSlot = false;

	// 현재 무기 강화 단계
	UPROPERTY(ReplicatedUsing = OnRep_UpgradeLevel, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 UpgradeLevel = 0;

	// 현재 활성 사격 상태 //note: 사격 전환 모드 어빌리티가 내부적으로 모드 사격 어빌리티가 부여됐는지 체크. 무기인스턴스의 FireModeState는 필요 없음
	UPROPERTY(Transient)
	EPRWeaponFireModeState FireModeState = EPRWeaponFireModeState::BaseFire;

	// 최근 일반 무기 동작 실패 사유
	UPROPERTY(Transient)
	EPRWeaponActionFailReason LastWeaponFailReason = EPRWeaponActionFailReason::None;

	// 최근 Mod 동작 실패 사유
	UPROPERTY(Transient)
	EPRWeaponModFailReason LastModFailReason = EPRWeaponModFailReason::None;

	// 버프형 Mod의 종료 예정 시각 // 무기 인스턴스에 있어야하는지 확인 필요
	UPROPERTY(Transient)
	float ModEffectEndServerWorldTimeSeconds = 0.0f;
};
