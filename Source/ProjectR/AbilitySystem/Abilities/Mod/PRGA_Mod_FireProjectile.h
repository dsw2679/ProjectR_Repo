// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod Fire 투사체 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGA_Mod.h"
#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Fire.h"
#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_FireProjectile.h"
#include "PRGA_Mod_FireProjectile.generated.h"

class UNiagaraSystem;
class APRProjectileBase;
class UGameplayEffect;

// 투사체를 발사하는 모드 스킬 어빌리티 베이스.
// Activate 시점에 카메라 조준 방향으로 총구에서 투사체를 1발 스폰.
// 서버 권위 투사체에 한해 모드 데미지 GE Spec을 부여하여 충돌 시 모드 GE 적용 흐름과 연결한다
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod_FireProjectile : public UPRGA_FireProjectile
{
	GENERATED_BODY()

public:
	UPRGA_Mod_FireProjectile();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
protected:
	/*~ UPRGameplayAbility Interface ~*/
	virtual FGameplayEffectSpecHandle MakeModEffectSpec(float Damage, float GroggyDamage = 0.0f, const FHitResult* HitResult = nullptr) const override;

protected:
	/*~ UPRGA_Fire Interface ~*/
	virtual void OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile) override;
	virtual void OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile) override;

	/*~ UPRGA_FireProjectile Interface ~*/
	virtual EPRFirePreviewMode GetPreviewFireMode() const override;
	
protected:
	// 모드 스킬 기본 데미지. SetByCaller로 GE Spec에 전달
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile", meta = (ClampMin = "0.0"))
	float Damage = 0.f;

	// 모드 스킬 그로기 데미지. SetByCaller로 GE Spec에 전달
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.f;
};
