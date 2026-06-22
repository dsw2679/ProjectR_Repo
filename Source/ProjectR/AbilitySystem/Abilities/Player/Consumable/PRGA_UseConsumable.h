// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (인벤토리 아이템 소모 및 장비 연동 구현)
// Author: 이건주 (소모품 사용 애니메이션 중 무기 비활성화 및 행동 차단 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_UseConsumable.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UPRItemInstance_Consumable;
struct FGameplayEventData;

// 소비 아이템 사용 몽타주를 재생하고 노티파이 커밋 시점에 효과와 스택 소모를 처리하는 어빌리티다
UCLASS()
class PROJECTR_API UPRGA_UseConsumable : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_UseConsumable();

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

public:
	// 외부 사용 요청에서 이번 어빌리티가 사용할 소비 Item을 지정한다
	void SetConsumableItem(UPRItemInstance_Consumable* InConsumableItem);

protected:
	// 소비 아이템 노티파이 이벤트 수신 시 효과 적용과 스택 소모를 확정한다
	UFUNCTION()
	void OnConsumableCommitEvent(FGameplayEventData EventData);

	// 소비 아이템 사용 몽타주가 정상 종료된 경우 어빌리티를 종료한다
	UFUNCTION()
	void OnConsumableMontageCompleted();

	// 소비 아이템 사용 몽타주가 인터럽트 또는 취소된 경우 어빌리티를 취소 종료한다
	UFUNCTION()
	void OnConsumableMontageInterrupted();

	// 현재 소비 Item에 대응하는 GE를 적용한다
	bool ApplyConsumableEffect();

	// 현재 소비 Item의 스택을 1개 소모한다
	bool ConsumeActiveItem();

	// 트리거 이벤트에서 소비 Item 인스턴스를 추출한다
	UPRItemInstance_Consumable* ResolveConsumableItemFromEventData(const FGameplayEventData* TriggerEventData) const;

protected:
	// 소비 아이템 사용 시 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Consumable|Montage")
	TObjectPtr<UAnimMontage> UseMontage = nullptr;

	// 소비 아이템 사용 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Consumable|Montage", meta = (ClampMin = "0.1"))
	float UseMontagePlayRate = 1.0f;

	// 커밋 노티파이 시점에 적용할 GE
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Consumable|Effect")
	FPREffectEntry UseEffect;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveCommitEventTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPRItemInstance_Consumable> ActiveConsumableItem = nullptr;

	bool bCommitted = false;
};
