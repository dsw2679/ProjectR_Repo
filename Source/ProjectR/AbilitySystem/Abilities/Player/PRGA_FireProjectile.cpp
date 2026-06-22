// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (특성 기반 투사체 피해 보정 연동 구현)
// Author: 배유찬 (예측 투사체 발사 경로 프리뷰 및 발사 제어 구현)
#include "PRGA_FireProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/Projectile/PRFirePreviewComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRPathPreviewAbility, Log, All);

void UPRGA_FireProjectile::OnAvatarSet(const FGameplayAbilitySpec& Spec, const FGameplayAbilityActorInfo* ActorInfo) const
{
	Super::OnAvatarSet(Spec, ActorInfo);

	if (ActorInfo == nullptr)
	{
		UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnAvatarSet 중단. ActorInfo 없음, Ability=%s"),
			*GetNameSafe(this));
		return;
	}

	UE_LOG(LogPRPathPreviewAbility, Log,
		TEXT("OnAvatarSet 진입. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d, Avatar=%s"),
		*GetNameSafe(this),
		bShouldPreviewPath,
		ActorInfo->IsLocallyControlledPlayer(),
		*GetNameSafe(ActorInfo->AvatarActor.Get()));
	
	// 로컬 전용 프리뷰 등록
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		if (!IsValid(ProjectileClass))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnAvatarSet 중단. ProjectileClass 없음, Ability=%s"),
				*GetNameSafe(this));
			return;
		}

		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnAvatarSet 중단. PlayerChar 없음, Avatar=%s"),
				*GetNameSafe(ActorInfo->AvatarActor.Get()));
			return;
		}

		UPRFirePreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRFirePreviewComponent>();
		if (!IsValid(Preview))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnAvatarSet 중단. PreviewComponent 없음, Player=%s"),
				*GetNameSafe(PlayerChar));
			return;
		}

		FPRFirePreviewKey PreviewKey;
		if (!TryBuildPreviewKey(Spec, PreviewKey))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning,
				TEXT("OnAvatarSet 중단. PreviewKey 생성 실패, Ability=%s, SourceObject=%s"),
				*GetNameSafe(this),
				*GetNameSafe(Spec.SourceObject.Get()));
			return;
		}

		FPRFirePreviewEntry PreviewEntry;
		PreviewEntry.Params = BuildResolvedPreviewParams();
		PreviewEntry.AbilitySpecHandle = Spec.Handle;
		PreviewEntry.SourceObject = Spec.SourceObject.Get();
		PreviewEntry.ProjectileClass = ProjectileClass;
		Preview->RegisterProjectilePreviewParams(PreviewKey, PreviewEntry);

		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 등록 요청 완료. Ability=%s, Player=%s, Preview=%s, Slot=%s, Mode=%s, ProjectileClass=%s, InitialSpeed=%.2f, GravityScale=%.2f, CollisionRadius=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerChar),
			*GetNameSafe(Preview),
			*UEnum::GetValueAsString(PreviewKey.WeaponSlot),
			*UEnum::GetValueAsString(PreviewKey.FireMode),
			*GetNameSafe(ProjectileClass.Get()),
			PreviewEntry.Params.InitialSpeed,
			PreviewEntry.Params.GravityScale,
			PreviewEntry.Params.CollisionRadius);
	}
	else
	{
		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 활성화 생략. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d"),
			*GetNameSafe(this),
			bShouldPreviewPath,
			ActorInfo->IsLocallyControlledPlayer());
	}
}

void UPRGA_FireProjectile::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);

	if (ActorInfo == nullptr)
	{
		UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnRemoveAbility 중단. ActorInfo 없음, Ability=%s"),
			*GetNameSafe(this));
		return;
	}

	UE_LOG(LogPRPathPreviewAbility, Log,
		TEXT("OnRemoveAbility 진입. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d, Avatar=%s"),
		*GetNameSafe(this),
		bShouldPreviewPath,
		ActorInfo->IsLocallyControlledPlayer(),
		*GetNameSafe(ActorInfo->AvatarActor.Get()));
	
	// 로컬 전용 프리뷰 해제
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnRemoveAbility 중단. PlayerChar 없음, Avatar=%s"),
				*GetNameSafe(ActorInfo->AvatarActor.Get()));
			return;
		}

		UPRFirePreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRFirePreviewComponent>();
		if (!IsValid(Preview))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning,
				TEXT("OnRemoveAbility 중단. PreviewComponent 없음, Player=%s, Ability=%s"),
				*GetNameSafe(PlayerChar),
				*GetNameSafe(this));
			return;
		}

		FPRFirePreviewKey PreviewKey;
		if (!TryBuildPreviewKey(Spec, PreviewKey))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning,
				TEXT("OnRemoveAbility 중단. PreviewKey 생성 실패, Ability=%s, SourceObject=%s"),
				*GetNameSafe(this),
				*GetNameSafe(Spec.SourceObject.Get()));
			return;
		}

		Preview->UnregisterProjectilePreviewParams(PreviewKey, Spec.Handle);

		UE_LOG(LogPRPathPreviewAbility, Log, TEXT("Preview 해제 요청 완료. Ability=%s, Player=%s, Preview=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerChar),
			*GetNameSafe(Preview));
	}
	else
	{
		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 비활성화 생략. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d"),
			*GetNameSafe(this),
			bShouldPreviewPath,
			ActorInfo->IsLocallyControlledPlayer());
	}
}

/*~ 프리뷰 ~*/

EPRFirePreviewMode UPRGA_FireProjectile::GetPreviewFireMode() const
{
	return EPRFirePreviewMode::BaseFire;
}

bool UPRGA_FireProjectile::TryBuildPreviewKey(const FGameplayAbilitySpec& Spec, FPRFirePreviewKey& OutKey) const
{
	UObject* SourceObject = Spec.SourceObject.Get();
	UPRWeaponDataAsset* WeaponData = nullptr;

	if (UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(SourceObject))
	{
		WeaponData = WeaponItem->GetWeaponData();
	}
	else if (UPRGameplayAbility* SourceAbility = Cast<UPRGameplayAbility>(SourceObject))
	{
		WeaponData = SourceAbility->GetCurrentWeaponData();
	}
	else
	{
		WeaponData = Cast<UPRWeaponDataAsset>(SourceObject);
	}

	if (IsValid(WeaponData))
	{
		OutKey.WeaponSlot = WeaponData->SlotType;
	}
	else
	{
		const FGameplayTagContainer& DynamicTags = Spec.GetDynamicSpecSourceTags();
		if (DynamicTags.HasTagExact(PRGameplayTags::State_CurrentWeaponSlot_Secondary))
		{
			// 보조 슬롯 차단 태그
			OutKey.WeaponSlot = EPRWeaponSlotType::Primary;
		}
		else if (DynamicTags.HasTagExact(PRGameplayTags::State_CurrentWeaponSlot_Primary))
		{
			// 주 슬롯 차단 태그
			OutKey.WeaponSlot = EPRWeaponSlotType::Secondary;
		}
	}

	if (OutKey.WeaponSlot != EPRWeaponSlotType::Primary && OutKey.WeaponSlot != EPRWeaponSlotType::Secondary)
	{
		return false;
	}

	OutKey.FireMode = GetPreviewFireMode();
	return OutKey.IsValid();
}

FPRProjectilePreviewParams UPRGA_FireProjectile::BuildResolvedPreviewParams() const
{
	// 어빌리티 템플릿 기준값
	FPRProjectilePreviewParams Params = PreviewParams;

	// 투사체 CDO 물리값
	if (IsValid(ProjectileClass))
	{
		const APRProjectileBase* ProjectileCDO = ProjectileClass->GetDefaultObject<APRProjectileBase>();
		if (IsValid(ProjectileCDO))
		{
			if (const UProjectileMovementComponent* PMC = ProjectileCDO->FindComponentByClass<UProjectileMovementComponent>())
			{
				Params.InitialSpeed = PMC->InitialSpeed;
				Params.GravityScale = PMC->ProjectileGravityScale;
			}
			if (const USphereComponent* Sphere = ProjectileCDO->FindComponentByClass<USphereComponent>())
			{
				Params.CollisionRadius = Sphere->GetUnscaledSphereRadius();
			}
		}
	}

	return Params;
}

/*~ EffectSpec 오버라이드 ~*/

FGameplayEffectSpecHandle UPRGA_FireProjectile::MakeWeaponEffectSpec(const FHitResult* HitResult) const
{
	// Override가 비어있으면 베이스 흐름(Registry의 DamageGE_FromWeapon) 사용
	if (!IsValid(ProjectileEffectOverride))
	{
		return Super::MakeWeaponEffectSpec(HitResult);
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

	AddCurrentWeaponDamageData(SpecHandle);

	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}

void UPRGA_FireProjectile::OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	// 서버 권위에 한해 무기 데미지 GE Spec 부여. Predicted 클라이언트는 데미지 적용 책임 없음
	if (HasAuthority(&CurrentActivationInfo))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeWeaponEffectSpec();
		if (SpecHandle.IsValid())
		{
			SpawnedProjectile->InitGameplayEffectSpec(SpecHandle);
		}
	}
	
	Super::OnProjectileSpawnSuccess(SpawnedProjectile);
}
