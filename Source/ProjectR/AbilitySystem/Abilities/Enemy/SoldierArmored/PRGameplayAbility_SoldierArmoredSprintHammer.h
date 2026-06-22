// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (돌진 중 조준 피격 보정 연동)
// Author: 손승우 (아머드 솔저 돌진 해머 공격 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredSprintHammer.generated.h"

class UPRSoldierArmoredCombatDataAsset;

// Soldier_Armored 중거리 진입용 원본형 스프린트 해머 공격
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredSprintHammer : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	// Ability 태그 고정, 타격 수치는 CombatDataAsset 참조
	UPRGameplayAbility_SoldierArmoredSprintHammer();

	// SprintHammer 타격값 적용 후 공통 근접 공격 흐름 시작
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// SprintHammer 타격값 관리용 DataAsset
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Data")
	TObjectPtr<UPRSoldierArmoredCombatDataAsset> CombatDataAsset;
};
