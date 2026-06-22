// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 게이지 충전 시 어빌리티 활성화 제어 구현)
// Author: 이건주 (드론/배리어 관련 액티브 어빌리티 부여 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGA_Mod.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRGA_Mod_GrantAbility.generated.h"

class UPRWeaponManagerComponent;

/**
 * 
 */
UCLASS()
class PROJECTR_API UPRGA_Mod_GrantAbility : public UPRGA_Mod
{
	GENERATED_BODY()
	
public:
	UPRGA_Mod_GrantAbility();
	
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 어빌리티 회수 시 부여 어빌리티 런타임 정리
	virtual void CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	void WaitModStackExhausted(EPRWeaponSlotType WeaponSlotType);

	// 부여 어빌리티와 비용 감시 정리
	void CleanupGrantedAbilityRuntime(const FGameplayAbilityActorInfo* ActorInfo);

	// 무기 변경 이벤트 바인딩
	void BindWeaponEquipmentChanged();

	// 무기 변경 이벤트 해제
	void UnbindWeaponEquipmentChanged();

	// 무기 변경 이벤트 처리
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot);
	
	UFUNCTION()
	void OnInputPressed(float TimeWaited);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> AbilityToGrant;
	
private:
	FGameplayAbilitySpecHandle GrantedSpecHandle;
	FGameplayAttribute CostAttribute;
	FDelegateHandle CostExhaustedHandle;
	EPRWeaponSlotType SourceWeaponSlot = EPRWeaponSlotType::None;
	TWeakObjectPtr<UPRWeaponManagerComponent> BoundWeaponManager;
};
