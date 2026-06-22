// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (돌진 중 조준 피격 보정 연동)
// Author: 손승우 (아머드 솔저 돌진 해머 공격 구현)
#include "PRGameplayAbility_SoldierArmoredSprintHammer.h"

#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredGameplayTags.h"

UPRGameplayAbility_SoldierArmoredSprintHammer::UPRGameplayAbility_SoldierArmoredSprintHammer()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer));
	AbilityTag = PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer;
	bUseDefaultHitConfig = false;
}

void UPRGameplayAbility_SoldierArmoredSprintHammer::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(CombatDataAsset))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타이밍/몽타주는 Ability BP, 타격 수치는 공용 전투 DataAsset 책임
	const FPREnemyAttackHitConfig& HitConfig = CombatDataAsset->SprintHammerHitConfig;
	ApplyEnemyAttackHitConfig(HitConfig);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
