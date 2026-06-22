// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (특성 기반 투사체 피해 보정 연동 구현)
// Author: 배유찬 (예측 투사체 발사 경로 프리뷰 및 발사 제어 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGA_Fire.h"
#include "ProjectR/Projectile/PRFirePreviewTypes.h"
#include "ProjectR/Projectile/PRProjectileTypes.h"
#include "PRGA_FireProjectile.generated.h"

class APRProjectileBase;
class UGameplayEffect;

/**
 * 투사체 발사 어빌리티.
 * AvatarActor 설정 이후 ProjectileClass CDO의 발사 기본값을 추출해 플레이어의 예측 경로 표시 컴포넌트에 PreviewParams 주입.
 */
UCLASS()
class PROJECTR_API UPRGA_FireProjectile : public UPRGA_Fire
{
	GENERATED_BODY()

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	/*~ UPRGameplayAbility Interface ~*/
	virtual void OnAvatarSet(const FGameplayAbilitySpec& Spec, const FGameplayAbilityActorInfo* ActorInfo) const override;

	/*~ UPRGA_Fire Interface ~*/
	virtual FGameplayEffectSpecHandle MakeWeaponEffectSpec(const FHitResult* HitResult = nullptr) const override;
	virtual void OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile) override;

	/*~ UPRGA_FireProjectile Interface ~*/
	// 프리뷰 등록에 사용할 발사 모드 반환
	virtual EPRFirePreviewMode GetPreviewFireMode() const;

	// 어빌리티 SourceObject 기준 프리뷰 키 생성
	bool TryBuildPreviewKey(const FGameplayAbilitySpec& Spec, FPRFirePreviewKey& OutKey) const;

	// 투사체 CDO 기준 프리뷰 파라미터 합성
	FPRProjectilePreviewParams BuildResolvedPreviewParams() const;

protected:
	// 본 어빌리티가 발사할 투사체 클래스. CDO의 ProjectileMovement/Sphere 컴포넌트에서 속력/중력/반지름을 추출하여 PreviewParams로 합성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Fire|Projectile")
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 궤적 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Projectile")
	bool bShouldPreviewPath = false;
	
	// 예측 경로 표시 컴포넌트에 주입할 기본 파라미터.
	// InitialSpeed/GravityScale/CollisionRadius는 OnGive에서 ProjectileClass CDO 값으로 덮어쓰며,
	// 그 외 시뮬레이션/샘플링 파라미터는 본 값을 그대로 사용. WeaponActor는 OnGive에서 활성 무기로 바인딩
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Projectile")
	FPRProjectilePreviewParams PreviewParams;

	// 투사체에 부여할 GE 오버라이드. 비워두면 Registry의 DamageGE_FromWeapon 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Fire|Projectile")
	TSubclassOf<UGameplayEffect> ProjectileEffectOverride;
};
