// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (전투 상태 중 스태미나 회복 제어 구현)
// Author: 배유찬 (보스 조우 시 전투 상태 전환 보정 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerCombatEngaged.generated.h"

class UGameplayEffect;

// 전투 교전 이벤트를 받아 전투 중 상태 GE를 갱신하는 플레이어 Ability
UCLASS()
class PROJECTR_API UPRGA_PlayerCombatEngaged : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerCombatEngaged();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 전투 중 상태 GE 적용
	void ApplyCombatingStateEffect();

protected:
	// State.Combating을 부여하는 Duration GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TSubclassOf<UGameplayEffect> CombatingStateEffectClass;
};
