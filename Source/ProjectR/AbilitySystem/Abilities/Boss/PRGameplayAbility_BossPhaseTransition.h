// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Phase Transition 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRGameplayAbility_BossPhaseTransition.generated.h"

// 보스 페이즈 전환 이벤트를 받아 실제 BossCharacter의 페이즈 확정을 호출하는 Ability다.
// 전환 몽타주/연출이 추가되면 이 Ability 안에서 완료 시점을 조절하면 된다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_BossPhaseTransition : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossPhaseTransition();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 이 Ability가 확정할 목표 페이즈다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	EPRBossPhase TargetPhase = EPRBossPhase::Phase1;
};
