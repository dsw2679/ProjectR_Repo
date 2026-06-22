// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Wake From Perch 공격 패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyAlert.h"
#include "PRGameplayAbility_FaeRoyalArcherWakeFromPerch.generated.h"

// Fae Royal Archer가 Perch/Gargoyle 대기 상태에서 공중 전투로 진입할 때 사용하는 전용 기상 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaeRoyalArcherWakeFromPerch : public UPRGameplayAbility_EnemyAlert
{
	GENERATED_BODY()

public:
	// RoyalArcher 전용 WakeFromPerch 태그와 기본 몽타주 종료 규칙을 초기화한다.
	UPRGameplayAbility_FaeRoyalArcherWakeFromPerch();

protected:
	/*~ UGameplayAbility Interface ~*/
	// Wake 몽타주 재생 전에 Perch 대기 포즈 상태를 해제한다.
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
