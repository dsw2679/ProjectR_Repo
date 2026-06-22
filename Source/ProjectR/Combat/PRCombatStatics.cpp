// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 보정용 정적 함수 구현)
// Author: 배유찬 (약점 판정 및 기본 피해 계산 로직 구현)
// Author: 손승우 (적 AI 상태별 피해 보정 정적 함수 구현)
// Author: 이건주 (Penitent 몬스터용 특수 피해 계산식 구현)
#include "PRCombatStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatInterface.h"

namespace
{
	// 피해 컨텍스트용 플레이어 컨트롤러 해석
	APlayerController* ResolveDamageInstigatorController(
		const FGameplayEffectContextHandle& ContextHandle,
		const AActor* SourceActor)
	{
		AActor* InstigatorActor = ContextHandle.GetInstigator();
		if (APlayerController* PlayerController = Cast<APlayerController>(InstigatorActor))
		{
			return PlayerController;
		}

		if (APlayerState* PlayerState = Cast<APlayerState>(InstigatorActor))
		{
			return PlayerState->GetPlayerController();
		}

		if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			return Cast<APlayerController>(InstigatorPawn->GetController());
		}

		if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
		{
			return Cast<APlayerController>(SourcePawn->GetController());
		}

		return nullptr;
	}
}

UAbilitySystemComponent* UPRCombatStatics::FindAbilitySystemComponent(const AActor* Actor)
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
}

EPRTeam UPRCombatStatics::GetActorTeam(const AActor* Actor)
{
	if (const IPRCombatInterface* Combat = Cast<IPRCombatInterface>(Actor))
	{
		return Combat->GetTeam();
	}
	return EPRTeam::Neutral;
}

bool UPRCombatStatics::IsFriendly(const AActor* SourceActor, const AActor* TargetActor)
{
	const EPRTeam SourceTeam = GetActorTeam(SourceActor);
	const EPRTeam TargetTeam = GetActorTeam(TargetActor);
	if (SourceTeam == EPRTeam::Neutral || TargetTeam == EPRTeam::Neutral)
	{
		return false;
	}
	return SourceTeam == TargetTeam;
}

FPRDamageOutputs UPRCombatStatics::ComputeDamage(const FPRDamageInputs& Inputs, const FHitResult& HitResult, const AActor* TargetActor)
{
	FPRDamageOutputs Outputs;
	Outputs.bIsFromFriendly = Inputs.bIsFromFriendly;
	
	// 부위 위치는 타깃이 알고 있다 (IPRCombatInterface::GetDamageRegionInfo)
	if (const IPRCombatInterface* Combat = Cast<IPRCombatInterface>(TargetActor))
	{
		Outputs.Region = Combat->GetDamageRegionInfo(HitResult.BoneName);
	}

	// 부위별 데미지 배율. 약점은 증폭, 장갑은 방어력 보너스로 처리
	float DamageMultiplier = 1.0f;
	float ArmorBonus = 0.0f;

	if (Outputs.Region.RegionDamageMultiplier > 0.f)
	{
		DamageMultiplier *= Outputs.Region.RegionDamageMultiplier;
	}
	
	if (Outputs.Region.IsWeakpoint())
	{
		DamageMultiplier *= FMath::Max(Inputs.WeakpointMultiplier, 0.0f);
	}
	else if (Outputs.Region.IsArmor())
	{
		ArmorBonus = PRCombatConstants::ArmorRegionBonus;
	}
	
	// 프렌들리 파이어 감쇠
	if (Inputs.bIsFromFriendly)
	{
		if (Inputs.bIsFromPlayer)
		{
			DamageMultiplier *= FMath::Max(PRCombatConstants::FriendlyFireMultiplier_PvP, 0.f);
		}
		else
		{
			DamageMultiplier *= FMath::Max(PRCombatConstants::FriendlyFireMultiplier_EvE, 0.f);
		}
	}
	
	// 치명타 판정
	Outputs.bIsCritical = FMath::FRand() <= Inputs.CriticalHitChance;
	const float CriticalMultiplier = Outputs.bIsCritical ? FMath::Max(Inputs.CriticalDamageMultiplier, 1.0f) : 1.0f;

	// 배율 적용 (부위·프렌들리·치명타)
	const float PreArmorDamage = FMath::Max(Inputs.BaseDamage * DamageMultiplier * CriticalMultiplier, 0.0f);
	const float PreArmorGroggy = FMath::Max(Inputs.GroggyDamage * DamageMultiplier, 0.0f);

	// 방어력 경감: EffectiveArmor = max(Armor + ArmorBonus - ArmorPenetration, 0)
	// 경감률 = EffectiveArmor / (EffectiveArmor + K)
	const float EffectiveArmor = FMath::Max(Inputs.TargetArmor + ArmorBonus - Inputs.ArmorPenetration, 0.0f);
	const float ArmorReduction = EffectiveArmor / (EffectiveArmor + PRCombatConstants::ArmorScaling);

	Outputs.FinalDamage = PreArmorDamage * (1.0f - ArmorReduction);
	Outputs.GroggyDamage = PreArmorGroggy * (1.0f - ArmorReduction);

	return Outputs;
}

float UPRCombatStatics::CalculateBaseGroggyDamage(float BaseDamage)
{
	return BaseDamage * PRCombatConstants::GroggyDamageCoeff;
}

FPRDamageAppliedContext UPRCombatStatics::BuildSimpleDamageAppliedContext(
	const UAbilitySystemComponent* SourceAbilitySystemComponent,
	const FGameplayEffectSpecHandle& DamageEffectSpecHandle,
	const FHitResult* HitResult)
{
	// 단순 피해 컨텍스트 생성 실패
	if (!IsValid(SourceAbilitySystemComponent))
	{
		return FPRDamageAppliedContext();
	}

	FGameplayEffectSpec* EffectSpec = DamageEffectSpecHandle.Data.Get();
	// 피해 스펙 조회 실패
	if (!EffectSpec)
	{
		return FPRDamageAppliedContext();
	}

	const float BaseDamage = EffectSpec->GetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage,
		false,
		0.0f);

	const bool bHasWeaponBaseDamage = EffectSpec->SetByCallerTagMagnitudes.Contains(
		PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage);
	const bool bHasEnemyAttackMultiplier = EffectSpec->SetByCallerTagMagnitudes.Contains(
		PRCombatGameplayTags::SetByCaller_AttackMultiplier);

	float ResolvedBaseDamage = 0.0f;
	bool bCanApplyCritical = false;
	if (bHasWeaponBaseDamage)
	{
		// 플레이어 무기 대미지 해석
		ResolvedBaseDamage = BaseDamage;
		bCanApplyCritical = true;
	}
	else if (bHasEnemyAttackMultiplier)
	{
		const float AttackMultiplier = EffectSpec->GetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			false,
			1.0f);
		const float AttackPower = SourceAbilitySystemComponent->GetNumericAttribute(
			UPRAttributeSet_Enemy::GetAttackPowerAttribute());

		// 적 공격력 기반 대미지 해석
		ResolvedBaseDamage = AttackPower * AttackMultiplier;
	}

	bool bIsCritical = false;
	float ResolvedCriticalMultiplier = 1.0f;
	if (bCanApplyCritical)
	{
		const float CriticalHitChance = SourceAbilitySystemComponent->GetNumericAttribute(
			UPRAttributeSet_Player::GetCriticalHitChanceAttribute());

		const float CriticalDamageMultiplier = SourceAbilitySystemComponent->GetNumericAttribute(
			UPRAttributeSet_Player::GetCriticalDamageMultiplierAttribute());

		const float ClampedCriticalChance = FMath::Clamp(CriticalHitChance, 0.0f, 1.0f);

		// 플레이어 무기 치명타 판정
		bIsCritical = FMath::FRand() <= ClampedCriticalChance;
		ResolvedCriticalMultiplier = bIsCritical
			? FMath::Max(CriticalDamageMultiplier, 1.0f)
			: 1.0f;
	}

	const float FinalDamage = FMath::Max(ResolvedBaseDamage, 0.0f) * ResolvedCriticalMultiplier;
	const FGameplayEffectContextHandle& EffectContext = EffectSpec->GetEffectContext();

	FPRDamageAppliedContext DamageContext;
	EffectSpec->GetAllAssetTags(DamageContext.EffectTags);
	DamageContext.FinalDamage = FinalDamage;
	DamageContext.FinalGroggyDamage = 0.0f;
	DamageContext.bIsCritical = bIsCritical;
	DamageContext.SourceObject = EffectContext.GetSourceObject();
	DamageContext.Instigator = EffectContext.GetOriginalInstigator();
	DamageContext.InstigatorController = ResolveDamageInstigatorController(
		EffectContext,
		SourceAbilitySystemComponent->GetAvatarActor());
	if (HitResult)
	{
		DamageContext.HitResult = *HitResult;
	}

	return DamageContext;
}
