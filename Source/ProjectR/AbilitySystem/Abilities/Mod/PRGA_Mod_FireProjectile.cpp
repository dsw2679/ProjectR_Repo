// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod Fire 투사체 구현)
#include "PRGA_Mod_FireProjectile.h"
#include "NiagaraSystem.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_SpawnPredictedProjectile.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"

UPRGA_Mod_FireProjectile::UPRGA_Mod_FireProjectile()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InputTag = PRGameplayTags::Input_Ability_Fire_Primary;
}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod_FireProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	FTransform LaunchTransform = GetProjectileLaunchTransform();
	FireProjectile(ProjectileClass, LaunchTransform.GetLocation(), LaunchTransform.Rotator());
}

void UPRGA_Mod_FireProjectile::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (UPRGA_Mod_FireProjectile* Inst = GetAbilityInstance<UPRGA_Mod_FireProjectile>(Handle, ActorInfo))
	{
		if (UPRGA_Mod* SourceMod = Cast<UPRGA_Mod>(Inst->GetCurrentSourceObject()))
		{
			SourceMod->ApplyModCost(
				SourceMod->GetCurrentAbilitySpecHandle(),
				SourceMod->GetCurrentActorInfo(),
				SourceMod->GetCurrentActivationInfo());
		}
	}
	
	if (CheckCost(Handle,ActorInfo))
	{
		Super::ApplyCost(Handle, ActorInfo, ActivationInfo);	
	}
}

/*~ 투사체 발사 ~*/

void UPRGA_Mod_FireProjectile::OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		K2_EndAbility();
		return;
	}

	// 서버 권위에 한해 모드 데미지 GE Spec 부여. Predicted 클라이언트는 데미지 적용 책임 없음
	if (HasAuthority(&CurrentActivationInfo))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeModEffectSpec(Damage, GroggyDamage);
		if (SpecHandle.IsValid())
		{
			SpawnedProjectile->InitGameplayEffectSpec(SpecHandle);
		}
	}

	K2_OnProjectileSpawnSuccess(SpawnedProjectile);
	K2_EndAbility();
}

void UPRGA_Mod_FireProjectile::OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile)
{
	K2_OnProjectileSpawnFailed(SpawnedProjectile);
	K2_EndAbility();
}

/*~ 프리뷰 ~*/

EPRFirePreviewMode UPRGA_Mod_FireProjectile::GetPreviewFireMode() const
{
	return EPRFirePreviewMode::ModFire;
}

/*~ EffectSpec 오버라이드 ~*/

FGameplayEffectSpecHandle UPRGA_Mod_FireProjectile::MakeModEffectSpec(float InDamage, float InGroggyDamage, const FHitResult* HitResult) const
{
	// Override가 비어있으면 베이스 흐름(Registry의 DamageGE_FromMod) 사용
	if (!IsValid(ProjectileEffectOverride))
	{
		return Super::MakeModEffectSpec(InDamage, InGroggyDamage, HitResult);
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		ProjectileEffectOverride);

	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	if (InDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, InDamage);
	}

	if (InGroggyDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, InGroggyDamage);
	}

	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}
