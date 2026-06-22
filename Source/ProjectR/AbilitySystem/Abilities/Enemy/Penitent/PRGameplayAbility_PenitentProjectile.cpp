// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 투사체 공격 패턴 어빌리티 구현)
#include "PRGameplayAbility_PenitentProjectile.h"

#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentProjectile::UPRGameplayAbility_PenitentProjectile()
{
	FGameplayTagContainer PenitentProjectileTags;
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Projectile);
	SetAssetTags(PenitentProjectileTags);
	AbilityTag = PRGameplayTags::Ability_Enemy_Penitent_Projectile;
}
