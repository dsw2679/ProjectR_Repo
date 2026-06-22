// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Staff Swing 공격 패턴 어빌리티 구현)
#include "PRGameplayAbility_PenitentStaffSwing.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

/*~ 초기화 ~*/

UPRGameplayAbility_PenitentStaffSwing::UPRGameplayAbility_PenitentStaffSwing()
{
	FGameplayTagContainer PenitentStaffSwingTags;
	PenitentStaffSwingTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	PenitentStaffSwingTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	PenitentStaffSwingTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_StaffSwing);
	SetAssetTags(PenitentStaffSwingTags);
	AbilityTag = PRGameplayTags::Ability_Enemy_Penitent_StaffSwing;
	bAllowMultipleMeleeHits = true;
	bUseDefaultHitConfig = false;
}

/*~ UGameplayAbility Interface ~*/

void UPRGameplayAbility_PenitentStaffSwing::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	CurrentStaffSwingSection = EPRPenitentStaffSwingSection::Swing1;
	MontageStartSection = StaffSwing1StartSection;
	ApplyEnemyAttackHitConfig(GetCurrentHitConfig());
	BindComboWindowEvent();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_PenitentStaffSwing::EndAbility(const FGameplayAbilitySpecHandle Handle,
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

/*~ UPRGameplayAbility_EnemyMeleeAttack Interface ~*/

void UPRGameplayAbility_PenitentStaffSwing::ExecuteMeleeHit()
{
	ApplyEnemyAttackHitConfig(GetCurrentHitConfig());
	Super::ExecuteMeleeHit();
}

/*~ 콤보 흐름 ~*/

void UPRGameplayAbility_PenitentStaffSwing::HandleComboWindowGameplayEvent(FGameplayEventData Payload)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyComboWindow)
	{
		return;
	}

	if (CurrentStaffSwingSection != EPRPenitentStaffSwingSection::Swing1)
	{
		return;
	}

	const float DistanceToTarget = GetDistanceToCurrentTarget();
	const float ClampedChance = FMath::Clamp(StaffSwing2Chance, 0.0f, 1.0f);
	if (DistanceToTarget <= StaffSwing2MaxRange && FMath::FRand() <= ClampedChance)
	{
		JumpToSection(StaffSwing2EnterSection, EPRPenitentStaffSwingSection::Swing2);
	}
}

void UPRGameplayAbility_PenitentStaffSwing::BindComboWindowEvent()
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
		ActiveComboWindowEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_PenitentStaffSwing::HandleComboWindowGameplayEvent);
		ActiveComboWindowEventTask->ReadyForActivation();
	}
}

void UPRGameplayAbility_PenitentStaffSwing::JumpToSection(FName SectionName, EPRPenitentStaffSwingSection NewSection)
{
	if (SectionName == NAME_None)
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		CurrentStaffSwingSection = NewSection;
		ApplyEnemyAttackHitConfig(GetCurrentHitConfig());
		AbilitySystemComponent->CurrentMontageJumpToSection(SectionName);
	}
}

float UPRGameplayAbility_PenitentStaffSwing::GetDistanceToCurrentTarget() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const AActor* TargetActor = GetCurrentThreatTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
}

const FPREnemyAttackHitConfig& UPRGameplayAbility_PenitentStaffSwing::GetCurrentHitConfig() const
{
	switch (CurrentStaffSwingSection)
	{
	case EPRPenitentStaffSwingSection::Swing2:
		return StaffSwing2HitConfig;

	case EPRPenitentStaffSwingSection::Swing1:
	default:
		return StaffSwing1HitConfig;
	}
}