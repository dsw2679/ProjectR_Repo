// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Swap Weapon 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRGA_SwapWeapon.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 주무기와 보조무기 슬롯의 활성 상태를 토글하는 어빌리티.
 * 현재 활성 슬롯의 반대 슬롯으로 전환한 뒤 타겟 슬롯 Draw 몽타주를 재생.
 */
UCLASS()
class PROJECTR_API UPRGA_SwapWeapon : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_SwapWeapon();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Draw 몽타주 정상 종료 시 어빌리티 종료
	UFUNCTION()
	void OnDrawMontageCompleted();

	// Draw 몽타주 인터럽트 또는 취소 시 어빌리티 취소 종료
	UFUNCTION()
	void OnDrawMontageInterrupted();

	// 타겟 슬롯에 대응하는 Draw 몽타주 선택
	UAnimMontage* SelectDrawMontage(EPRWeaponSlotType TargetSlot) const;

	// 현재 슬롯 기준 다음 활성 슬롯 계산
	EPRWeaponSlotType ResolveTargetSlot(EPRWeaponSlotType CurrentSlot) const;

protected:
	// 주무기 Draw 몽타주. 두손 무기 Draw 모션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Weapon|Swap")
	TObjectPtr<UAnimMontage> PrimaryWeaponDrawMontage = nullptr;

	// 보조무기 Draw 몽타주. 한손 무기 Draw 모션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Weapon|Swap")
	TObjectPtr<UAnimMontage> SecondaryWeaponDrawMontage = nullptr;

	// Draw 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Weapon|Swap", meta = (ClampMin = "0.1"))
	float DrawMontagePlayRate = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask = nullptr;

	EPRWeaponSlotType CachedTargetSlot = EPRWeaponSlotType::None;
};
