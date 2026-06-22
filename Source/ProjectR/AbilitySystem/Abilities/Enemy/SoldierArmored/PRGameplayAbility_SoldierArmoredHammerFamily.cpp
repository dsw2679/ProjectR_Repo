// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피해 보정 연동 구현)
// Author: 손승우 (아머드 솔저 해머 콤보 시퀀스 및 모션 워핑 구현)
#include "PRGameplayAbility_SoldierArmoredHammerFamily.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredGameplayTags.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

UPRGameplayAbility_SoldierArmoredHammerFamily::UPRGameplayAbility_SoldierArmoredHammerFamily()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_HammerFamily));
	AbilityTag = PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_HammerFamily;
	bAllowMultipleMeleeHits = true;
	bUseDefaultHitConfig = false;
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(CombatDataAsset))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentHammerSection = EPRSoldierArmoredHammerSection::Combo1;
	MontageStartSection = CombatDataAsset->HammerSections.Combo1StartSection;
	ApplyCurrentHitConfig();
	BindComboWindowEvent();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsValid(ActiveComboWindowEventTask))
	{
		ActiveComboWindowEventTask->EndTask();
		ActiveComboWindowEventTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::ExecuteMeleeHit()
{
	ApplyCurrentHitConfig();
	Super::ExecuteMeleeHit();
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::HandleComboWindowGameplayEvent(FGameplayEventData Payload)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| !IsValid(CombatDataAsset)
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyComboWindow)
	{
		return;
	}

	const FPRSoldierArmoredHammerSectionNames& Sections = CombatDataAsset->HammerSections;
	const FPRSoldierArmoredHammerFlowConfig& Flow = CombatDataAsset->HammerFlow;
	const float DistanceToTarget = GetDistanceToCurrentTarget();
	switch (CurrentHammerSection)
	{
	case EPRSoldierArmoredHammerSection::Combo1:
		if (DistanceToTarget <= Flow.FinisherMaxRange && FMath::FRand() <= FMath::Clamp(Flow.Finisher1Chance, 0.0f, 1.0f))
		{
			JumpToSection(Sections.Finisher1EnterSection, EPRSoldierArmoredHammerSection::Finisher1);
		}
		else if (DistanceToTarget <= Flow.Combo2MaxRange)
		{
			JumpToSection(Sections.Combo2EnterSection, EPRSoldierArmoredHammerSection::Combo2);
		}
		else
		{
			return;
		}
		break;

	case EPRSoldierArmoredHammerSection::Combo2:
		if (DistanceToTarget <= Flow.FinisherMaxRange)
		{
			JumpToSection(Sections.Finisher2EnterSection, EPRSoldierArmoredHammerSection::Finisher2);
		}
		else
		{
			return;
		}
		break;

	default:
		break;
	}
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::BindComboWindowEvent()
{
	constexpr bool bOnlyTriggerOnce = false;
	constexpr bool bOnlyMatchExact = true;

	ActiveComboWindowEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRCombatGameplayTags::Event_Ability_EnemyComboWindow,
		nullptr,
		bOnlyTriggerOnce,
		bOnlyMatchExact);

	if (IsValid(ActiveComboWindowEventTask))
	{
		ActiveComboWindowEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_SoldierArmoredHammerFamily::HandleComboWindowGameplayEvent);
		ActiveComboWindowEventTask->ReadyForActivation();
	}
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::JumpToSection(FName SectionName, EPRSoldierArmoredHammerSection NewSection)
{
	if (SectionName == NAME_None)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		CurrentHammerSection = NewSection;
		ApplyCurrentHitConfig();
		ASC->CurrentMontageJumpToSection(SectionName);
	}
}

float UPRGameplayAbility_SoldierArmoredHammerFamily::GetDistanceToCurrentTarget() const
{
	const APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	const UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;
	const AActor* TargetActor = IsValid(ThreatComponent)
		? ThreatComponent->GetAttackTarget()
		: nullptr;

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
}

const FPREnemyAttackHitConfig& UPRGameplayAbility_SoldierArmoredHammerFamily::GetCurrentHitConfig() const
{
	static const FPREnemyAttackHitConfig EmptyHitConfig;
	if (!IsValid(CombatDataAsset))
	{
		return EmptyHitConfig;
	}

	switch (CurrentHammerSection)
	{
	case EPRSoldierArmoredHammerSection::Combo2:
		return CombatDataAsset->Combo2HitConfig;

	case EPRSoldierArmoredHammerSection::Finisher1:
		return CombatDataAsset->Finisher1HitConfig;

	case EPRSoldierArmoredHammerSection::Finisher2:
		return CombatDataAsset->Finisher2HitConfig;

	case EPRSoldierArmoredHammerSection::Combo1:
	default:
		return CombatDataAsset->Combo1HitConfig;
	}
}

void UPRGameplayAbility_SoldierArmoredHammerFamily::ApplyCurrentHitConfig()
{
	ApplyEnemyAttackHitConfig(GetCurrentHitConfig());
}
