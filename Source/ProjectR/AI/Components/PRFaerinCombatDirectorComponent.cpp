// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 전투 Director 컴포넌트 구현)
#include "PRFaerinCombatDirectorComponent.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinNearTeleportApproach.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Components/PRFaerinTeleportVFXComponent.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinCombatDirector, Log, All);

namespace
{
	constexpr float FaerinDirectAttackDistance = 50.0f;

	struct FPRFaerinPatternCandidate
	{
		const FPRPatternRule* PatternRule = nullptr;
		const FPRFaerinPatternLoopMetadata* LoopMetadata = nullptr;
		float FinalWeight = 0.0f;
	};

	bool ShouldIgnoreMainPatternRangeForPhase1And2(const FPRFaerinPhaseLoopConfig* PhaseConfig)
	{
		if (PhaseConfig == nullptr
			|| !PhaseConfig->bUsePrePatternApproachRoute
			|| (PhaseConfig->Phase != EPRBossPhase::Phase1 && PhaseConfig->Phase != EPRBossPhase::Phase2))
		{
			return false;
		}

		return true;
	}

	float ResolveEffectivePrePatternApproachMinDistance(const FPRFaerinPhaseLoopConfig& PhaseConfig)
	{
		return FMath::Max(PhaseConfig.PrePatternApproachMinDistance, FaerinDirectAttackDistance);
	}
}

// ===== 초기화 =====

UPRFaerinCombatDirectorComponent::UPRFaerinCombatDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPRFaerinCombatDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeDirector();

	if (bValidateOnBeginPlay && IsValid(LoopDataAsset))
	{
		TArray<FString> ValidationErrors;
		LoopDataAsset->ValidateLoopData(PatternDataAsset, nullptr, ValidationErrors);
		LogValidationErrors(ValidationErrors);
	}
}

void UPRFaerinCombatDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelCurrentLoopStep();
	Super::EndPlay(EndPlayReason);
}

// ===== 루프 실행 =====

bool UPRFaerinCombatDirectorComponent::RunNextLoopStep(UBehaviorTreeComponent& OwnerComp)
{
	if (!InitializeDirector())
	{
		if (!bHasLoggedInitializationError)
		{
			UE_LOG(
				LogPRFaerinCombatDirector,
				Warning,
				TEXT("Faerin CombatDirector 초기화 실패. Owner=%s, LoopDataAsset=%s, PatternDataAsset=%s, ASC=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(LoopDataAsset),
				*GetNameSafe(PatternDataAsset),
				*GetNameSafe(AbilitySystemComponent));
			bHasLoggedInitializationError = true;
		}
		return false;
	}

	bHasLoggedInitializationError = false;

	if (!CanRunLoopStep())
	{
		return false;
	}

	if (!OwnerEnemy->HasAuthority())
	{
		return false;
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning))
	{
		return false;
	}

	const EPRBossPhase CurrentPhase = ResolveCurrentPhase();
	const uint8 CurrentPhaseValue = static_cast<uint8>(CurrentPhase);
	if (IsValid(LoopDataAsset) && !RuntimeValidatedPhaseValues.Contains(CurrentPhaseValue))
	{
		TArray<FString> ValidationErrors;
		LoopDataAsset->ValidateLoopData(PatternDataAsset, AbilitySystemComponent, ValidationErrors);
		LogValidationErrors(ValidationErrors);
		RuntimeValidatedPhaseValues.Add(CurrentPhaseValue);
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(CurrentPhase);
	RefreshPeriodicSidePatternPhase(CurrentPhase);
	if (PhaseConfig != nullptr)
	{
		TryActivatePeriodicSidePatterns(*PhaseConfig);
	}

	LoopState = EPRFaerinCombatLoopState::SelectingPattern;
	SyncActiveTargetFromThreat(OwnerComp);

	FPRFaerinPatternPlan SelectedPlan;
	if (!SelectPatternPlan(OwnerComp, SelectedPlan))
	{
		if (StartOutOfRangeApproach(OwnerComp))
		{
			return true;
		}

		LoopState = EPRFaerinCombatLoopState::Idle;
		return false;
	}

	BeginLoopTargetCommit();

	if (PhaseConfig != nullptr)
	{
		const EPRFaerinPrePatternApproachStartResult ApproachStartResult =
			TryStartPrePatternApproach(*PhaseConfig, SelectedPlan);
		if (ApproachStartResult == EPRFaerinPrePatternApproachStartResult::Started)
		{
			return true;
		}

		if (ApproachStartResult == EPRFaerinPrePatternApproachStartResult::Failed)
		{
			LoopState = EPRFaerinCombatLoopState::Idle;
			ActivePatternPlan = FPRFaerinPatternPlan();
			PendingMainPatternPlan = FPRFaerinPatternPlan();
			bHasPendingMainPatternPlan = false;
			EndLoopTargetCommit();
			ActiveTarget = nullptr;
			return false;
		}
	}

	FPRFaerinPatternPlan PrePatternPortalPlan;
	if (PhaseConfig != nullptr
		&& TrySelectPrePatternPortalAssistPlan(OwnerComp, *PhaseConfig, SelectedPlan, PrePatternPortalPlan))
	{
		PendingMainPatternPlan = SelectedPlan;
		bHasPendingMainPatternPlan = true;
		ActivePatternPlan = PrePatternPortalPlan;
		if (StartPatternPlan(ActivePatternPlan, EPRFaerinObservedAbilityRole::PrePatternPortal))
		{
			return true;
		}

		PendingMainPatternPlan = FPRFaerinPatternPlan();
		bHasPendingMainPatternPlan = false;
	}

	ActivePatternPlan = SelectedPlan;
	if (!StartPatternPlan(ActivePatternPlan))
	{
		LoopState = EPRFaerinCombatLoopState::Idle;
		ActivePatternPlan = FPRFaerinPatternPlan();
		PendingMainPatternPlan = FPRFaerinPatternPlan();
		bHasPendingMainPatternPlan = false;
		EndLoopTargetCommit();
		ActiveTarget = nullptr;
		return false;
	}

	return true;
}

void UPRFaerinCombatDirectorComponent::CancelCurrentLoopStep()
{
	if (LoopState == EPRFaerinCombatLoopState::Idle)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopStepFailSafeTimerHandle);
		World->GetTimerManager().ClearTimer(PostPatternStrafeTimerHandle);
	}

	ClearTeleportVFXFinishedDelegate();
	if (UPRFaerinTeleportVFXComponent* TeleportVFXComponent = GetTeleportVFXComponent())
	{
		TeleportVFXComponent->CancelTeleportVFX();
	}

	if (IsObservedAbilityActive())
	{
		AbilitySystemComponent->CancelAbilityHandle(ActiveAbilityHandle);
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	ClearAbilityEndDelegate();
	EndLoopTargetCommit();
	ActivePatternPlan = FPRFaerinPatternPlan();
	PendingMainPatternPlan = FPRFaerinPatternPlan();
	bHasPendingMainPatternPlan = false;
	ClearActiveApproachSprintRequest();
	ClearActiveNearTeleportRequest();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;
	PendingTeleportVFXAbilityRole = EPRFaerinObservedAbilityRole::None;
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::Idle;
}

bool UPRFaerinCombatDirectorComponent::CanRunLoopStep() const
{
	return LoopState == EPRFaerinCombatLoopState::Idle
		&& IsValid(LoopDataAsset);
}

float UPRFaerinCombatDirectorComponent::CalculateStrafeDuration() const
{
	if (!IsValid(LoopDataAsset))
	{
		return 0.0f;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig == nullptr)
	{
		return 0.0f;
	}

	return PhaseConfig->StrafeDuration;
}

bool UPRFaerinCombatDirectorComponent::GetActiveApproachSprintRequest(FPRFaerinApproachSprintRequest& OutRequest) const
{
	OutRequest = ActiveApproachSprintRequest;
	return OutRequest.bIsValid && IsValid(OutRequest.TargetActor);
}

bool UPRFaerinCombatDirectorComponent::GetActiveNearTeleportRequest(FPRFaerinNearTeleportRequest& OutRequest) const
{
	OutRequest = ActiveNearTeleportRequest;
	return OutRequest.bIsValid && IsValid(OutRequest.TargetActor);
}

bool UPRFaerinCombatDirectorComponent::InitializeDirector()
{
	OwnerEnemy = Cast<APREnemyBaseCharacter>(GetOwner());
	if (!IsValid(OwnerEnemy))
	{
		return false;
	}

	AbilitySystemComponent = OwnerEnemy->GetEnemyAbilitySystemComponent();
	PatternDataAsset = OwnerEnemy->GetPatternDataAsset();

	return IsValid(AbilitySystemComponent)
		&& IsValid(PatternDataAsset)
		&& IsValid(LoopDataAsset);
}

bool UPRFaerinCombatDirectorComponent::SelectPatternPlan(UBehaviorTreeComponent& OwnerComp, FPRFaerinPatternPlan& OutPlan)
{
	OutPlan = FPRFaerinPatternPlan();

	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = IsValid(LoopDataAsset)
		? LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase())
		: nullptr;
	const EPRPatternContextMatchMode MainSelectionMatchMode =
		ShouldIgnoreMainPatternRangeForPhase1And2(PhaseConfig)
			? EPRPatternContextMatchMode::IgnoreRange
			: EPRPatternContextMatchMode::FullMatch;

	TArray<const FPRPatternRule*> MatchedRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		true,
		MainSelectionMatchMode,
		MatchedRules);

	TArray<FPRFaerinPatternCandidate> Candidates;
	float TotalWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		if (PatternRule == nullptr)
		{
			continue;
		}

		if (PhaseConfig != nullptr && ShouldExcludePatternRuleFromMainSelection(*PatternRule, *PhaseConfig))
		{
			continue;
		}

		const FPRFaerinPatternLoopMetadata* LoopMetadata = LoopDataAsset->FindPatternMetadata(PatternRule->AbilityTag);
		if (LoopMetadata == nullptr || !LoopMetadata->bEnabled || PatternRule->SelectionWeight <= 0.0f)
		{
			continue;
		}

		if (!CanActivatePatternAbilityTag(PatternRule->AbilityTag))
		{
			continue;
		}

		FPRFaerinPatternCandidate Candidate;
		Candidate.PatternRule = PatternRule;
		Candidate.LoopMetadata = LoopMetadata;
		Candidate.FinalWeight = PatternRule->SelectionWeight;
		Candidates.Add(Candidate);
		TotalWeight += Candidate.FinalWeight;
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return false;
	}

	const float PickValue = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;
	const FPRFaerinPatternCandidate* SelectedCandidate = &Candidates.Last();
	for (const FPRFaerinPatternCandidate& Candidate : Candidates)
	{
		AccumulatedWeight += Candidate.FinalWeight;
		if (PickValue <= AccumulatedWeight)
		{
			SelectedCandidate = &Candidate;
			break;
		}
	}

	OutPlan.AbilityTag = SelectedCandidate->PatternRule->AbilityTag;
	OutPlan.PatternRule = *SelectedCandidate->PatternRule;
	OutPlan.LoopMetadata = *SelectedCandidate->LoopMetadata;
	OutPlan.FinalSelectionWeight = SelectedCandidate->FinalWeight;
	ActiveTarget = PatternRuntime.CurrentTarget;

	LogSelectedPattern(OutPlan, PatternRuntime);
	return true;
}

bool UPRFaerinCombatDirectorComponent::SyncActiveTargetFromThreat(UBehaviorTreeComponent& OwnerComp)
{
	if (!IsValid(OwnerEnemy))
	{
		return false;
	}

	UPREnemyThreatComponent* ThreatComponent = OwnerEnemy->GetEnemyThreatComponent();
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->RefreshTargetCandidates();
	}

	AActor* TargetActor = IsValid(ThreatComponent)
		? ThreatComponent->GetAttackTarget()
		: nullptr;
	if (!IsValid(TargetActor) && IsValid(ThreatComponent))
	{
		TargetActor = ThreatComponent->GetCurrentTarget();
	}

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(TargetActor) && IsValid(BlackboardComponent))
	{
		TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (!IsValid(TargetActor))
	{
		ActiveTarget = nullptr;
		return false;
	}

	ActiveTarget = TargetActor;
	if (PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		BlackboardComponent->SetValueAsObject(CurrentTargetKey, TargetActor);
	}

	return true;
}

bool UPRFaerinCombatDirectorComponent::BeginLoopTargetCommit()
{
	if (bHasLoopTargetCommit || !IsValid(OwnerEnemy) || !IsValid(ActiveTarget))
	{
		return false;
	}

	UPREnemyThreatComponent* ThreatComponent = OwnerEnemy->GetEnemyThreatComponent();
	if (!IsValid(ThreatComponent))
	{
		return false;
	}

	ThreatComponent->BeginAttackCommit(ActiveTarget, ActiveTarget->GetActorLocation());
	bHasLoopTargetCommit = ThreatComponent->IsAttackCommitted()
		&& ThreatComponent->GetAttackTarget() == ActiveTarget;
	return bHasLoopTargetCommit;
}

void UPRFaerinCombatDirectorComponent::EndLoopTargetCommit()
{
	if (!bHasLoopTargetCommit)
	{
		return;
	}

	UPREnemyThreatComponent* ThreatComponent = IsValid(OwnerEnemy)
		? OwnerEnemy->GetEnemyThreatComponent()
		: nullptr;
	if (IsValid(ThreatComponent))
	{
		ThreatComponent->EndAttackCommit();
	}

	bHasLoopTargetCommit = false;
}

bool UPRFaerinCombatDirectorComponent::StartOutOfRangeApproach(UBehaviorTreeComponent& OwnerComp)
{
	if (IsApproachSprintRepeatBlocked())
	{
		return false;
	}

	if (!IsValid(LoopDataAsset))
	{
		return false;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig == nullptr || !PhaseConfig->ApproachAbilityTag.IsValid())
	{
		return false;
	}

	FPRFaerinPatternPlan ApproachPlan;
	if (!SelectOutOfRangeApproachPlan(OwnerComp, *PhaseConfig, ApproachPlan))
	{
		return false;
	}

	ActivePatternPlan = ApproachPlan;
	BeginLoopTargetCommit();
	if (StartApproachAbility(
			*PhaseConfig,
			false,
			EPRFaerinNearTeleportPlacementMode::SelfEQS,
			EPRFaerinObservedAbilityRole::Approach))
	{
		return true;
	}

	ActivePatternPlan = FPRFaerinPatternPlan();
	EndLoopTargetCommit();
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::SelectingPattern;
	return false;
}

bool UPRFaerinCombatDirectorComponent::SelectOutOfRangeApproachPlan(
	UBehaviorTreeComponent& OwnerComp,
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	FPRFaerinPatternPlan& OutPlan)
{
	OutPlan = FPRFaerinPatternPlan();

	if (!PhaseConfig.ApproachAbilityTag.IsValid())
	{
		return false;
	}

	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	TArray<const FPRPatternRule*> RangeIgnoredRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		true,
		EPRPatternContextMatchMode::IgnoreRange,
		RangeIgnoredRules);

	const FPRPatternRule* BestRule = nullptr;
	const FPRFaerinPatternLoopMetadata* BestMetadata = nullptr;
	float BestWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : RangeIgnoredRules)
	{
		if (PatternRule == nullptr || PatternRuntime.PatternContext.DistanceToTarget <= PatternRule->MaxRange)
		{
			continue;
		}

		if (ShouldExcludePatternRuleFromMainSelection(*PatternRule, PhaseConfig))
		{
			continue;
		}

		const FPRFaerinPatternLoopMetadata* LoopMetadata = LoopDataAsset->FindPatternMetadata(PatternRule->AbilityTag);
		if (LoopMetadata == nullptr || !LoopMetadata->bEnabled || PatternRule->SelectionWeight <= 0.0f)
		{
			continue;
		}

		if (!CanActivatePatternAbilityTag(PatternRule->AbilityTag))
		{
			continue;
		}

		const float CandidateWeight = PatternRule->SelectionWeight;
		const bool bPreferCandidate = BestRule == nullptr
			|| CandidateWeight > BestWeight
			|| (FMath::IsNearlyEqual(CandidateWeight, BestWeight) && PatternRule->MaxRange > BestRule->MaxRange);
		if (bPreferCandidate)
		{
			BestRule = PatternRule;
			BestMetadata = LoopMetadata;
			BestWeight = CandidateWeight;
		}
	}

	if (BestRule == nullptr || BestMetadata == nullptr)
	{
		return false;
	}

	OutPlan.AbilityTag = BestRule->AbilityTag;
	OutPlan.PatternRule = *BestRule;
	OutPlan.LoopMetadata = *BestMetadata;
	OutPlan.FinalSelectionWeight = BestWeight;
	ActiveTarget = PatternRuntime.CurrentTarget;

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Verbose,
			TEXT("Faerin 사거리 밖 sprint 접근 선택. Owner=%s, AbilityTag=%s, Distance=%.1f, MaxRange=%.1f"),
			*GetNameSafe(PatternRuntime.ControlledPawn),
			*OutPlan.AbilityTag.ToString(),
			PatternRuntime.PatternContext.DistanceToTarget,
			OutPlan.PatternRule.MaxRange);
	}

	return true;
}

bool UPRFaerinCombatDirectorComponent::CanActivatePatternAbilityTag(const FGameplayTag& AbilityTag) const
{
	if (!IsValid(AbilitySystemComponent) || !AbilityTag.IsValid())
	{
		return false;
	}

	FGameplayTagContainer FailureTags;
	return AbilitySystemComponent->CanActivateAbilityByTag(AbilityTag, FailureTags);
}

bool UPRFaerinCombatDirectorComponent::ShouldExcludePatternRuleFromMainSelection(
	const FPRPatternRule& PatternRule,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (PhaseConfig.MainPatternExcludedGroupTags.IsEmpty() || !PatternRule.PatternGroupTag.IsValid())
	{
		return false;
	}

	return PatternRule.PatternGroupTag.MatchesAny(PhaseConfig.MainPatternExcludedGroupTags);
}

bool UPRFaerinCombatDirectorComponent::StartPatternPlan(
	const FPRFaerinPatternPlan& PatternPlan,
	const EPRFaerinObservedAbilityRole ObservedRole)
{
	ActivePatternPlan = PatternPlan;

	if (ShouldRunTeleportVFXWrapper(PatternPlan))
	{
		return StartTeleportVFXWrapper(PatternPlan, ObservedRole);
	}

	return ActivatePatternAbility(PatternPlan, ObservedRole);
}

EPRFaerinPrePatternApproachStartResult UPRFaerinCombatDirectorComponent::TryStartPrePatternApproach(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const FPRFaerinPatternPlan& PatternPlan)
{
	bool bUseNearTeleport = false;
	EPRFaerinNearTeleportPlacementMode TeleportPlacementMode = EPRFaerinNearTeleportPlacementMode::SelfEQS;
	if (!ResolvePrePatternApproachRoute(PhaseConfig, PatternPlan, bUseNearTeleport, TeleportPlacementMode))
	{
		return EPRFaerinPrePatternApproachStartResult::NotNeeded;
	}

	PendingMainPatternPlan = PatternPlan;
	bHasPendingMainPatternPlan = true;
	ActivePatternPlan = PatternPlan;

	if (StartApproachAbility(
			PhaseConfig,
			bUseNearTeleport,
			TeleportPlacementMode,
			EPRFaerinObservedAbilityRole::PrePatternApproach))
	{
		return EPRFaerinPrePatternApproachStartResult::Started;
	}

	PendingMainPatternPlan = FPRFaerinPatternPlan();
	bHasPendingMainPatternPlan = false;
	ActivePatternPlan = FPRFaerinPatternPlan();
	return EPRFaerinPrePatternApproachStartResult::Failed;
}

bool UPRFaerinCombatDirectorComponent::ResolvePrePatternApproachRoute(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const FPRFaerinPatternPlan& PatternPlan,
	bool& bOutUseNearTeleport,
	EPRFaerinNearTeleportPlacementMode& OutTeleportPlacementMode) const
{
	bOutUseNearTeleport = false;
	OutTeleportPlacementMode = EPRFaerinNearTeleportPlacementMode::SelfEQS;

	const AActor* OwnerActor = GetOwner();
	if (!PhaseConfig.bUsePrePatternApproachRoute
		|| !IsPrePatternApproachPhase(PhaseConfig.Phase)
		|| !IsValid(OwnerActor)
		|| !IsValid(ActiveTarget))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	const float MinDistance = ResolveEffectivePrePatternApproachMinDistance(PhaseConfig);
	const float CloseMaxDistance = FMath::Max(PhaseConfig.PrePatternApproachCloseMaxDistance, MinDistance);
	const float MidMaxDistance = FMath::Max(PhaseConfig.PrePatternApproachMidMaxDistance, CloseMaxDistance);
	if (DistanceToTarget < MinDistance)
	{
		return false;
	}

	const bool bIsSpoke = IsSpokePatternPlan(PatternPlan);
	const bool bIsRanged = IsRangedPreApproachPatternPlan(PatternPlan);
	if (!bIsSpoke && !bIsRanged)
	{
		return false;
	}

	if (DistanceToTarget <= CloseMaxDistance)
	{
		bOutUseNearTeleport = bIsRanged;
		return true;
	}

	if (bIsRanged)
	{
		bOutUseNearTeleport = true;
		return true;
	}

	if (DistanceToTarget > MidMaxDistance)
	{
		bOutUseNearTeleport = true;
		OutTeleportPlacementMode = EPRFaerinNearTeleportPlacementMode::TargetBack;
		return true;
	}

	const float TargetBackTeleportChance = FMath::Clamp(PhaseConfig.MidRangeSpokeTargetBackTeleportChance, 0.0f, 1.0f);
	bOutUseNearTeleport = FMath::FRand() <= TargetBackTeleportChance;
	OutTeleportPlacementMode = bOutUseNearTeleport
		? EPRFaerinNearTeleportPlacementMode::TargetBack
		: EPRFaerinNearTeleportPlacementMode::SelfEQS;
	return true;
}

bool UPRFaerinCombatDirectorComponent::IsPrePatternApproachPhase(const EPRBossPhase Phase) const
{
	return Phase == EPRBossPhase::Phase1 || Phase == EPRBossPhase::Phase2;
}

bool UPRFaerinCombatDirectorComponent::IsSpokePatternPlan(const FPRFaerinPatternPlan& PatternPlan) const
{
	return PatternPlan.AbilityTag.MatchesTagExact(PRGameplayTags::Ability_Boss_Faerin_SpokeCombo);
}

bool UPRFaerinCombatDirectorComponent::IsRangedPreApproachPatternPlan(const FPRFaerinPatternPlan& PatternPlan) const
{
	if (PatternPlan.AbilityTag.MatchesTagExact(PRGameplayTags::Ability_Boss_Faerin_ShiftPlayerClose))
	{
		return true;
	}

	if (PatternPlan.AbilityTag.MatchesTagExact(PRGameplayTags::Ability_Boss_Faerin_CloneSequence))
	{
		return true;
	}

	if (PatternPlan.PatternRule.PatternGroupTag.MatchesTag(PRGameplayTags::Pattern_Boss_Faerin_Portal))
	{
		return true;
	}

	return PatternPlan.PatternRule.PatternCategory == EPRPatternCategory::Ranged;
}

bool UPRFaerinCombatDirectorComponent::ShouldRunTeleportVFXWrapper(
	const FPRFaerinPatternPlan& PatternPlan) const
{
	return static_cast<uint8>(ResolveCurrentPhase()) >= static_cast<uint8>(EPRBossPhase::Phase3)
		&& PatternPlan.LoopMetadata.TeleportWrapperPolicy != EPRFaerinTeleportWrapperPolicy::None;
}

bool UPRFaerinCombatDirectorComponent::StartTeleportVFXWrapper(
	const FPRFaerinPatternPlan& PatternPlan,
	const EPRFaerinObservedAbilityRole ObservedRole)
{
	UPRFaerinTeleportVFXComponent* TeleportVFXComponent = GetTeleportVFXComponent();
	if (!IsValid(TeleportVFXComponent) || !IsValid(ActiveTarget))
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin Teleport VFX 래퍼를 시작할 수 없다. Owner=%s, AbilityTag=%s, Component=%s, Target=%s"),
			*GetNameSafe(GetOwner()),
			*PatternPlan.AbilityTag.ToString(),
			*GetNameSafe(TeleportVFXComponent),
			*GetNameSafe(ActiveTarget));
		return false;
	}

	ClearTeleportVFXFinishedDelegate();
	TeleportVFXFinishedDelegateHandle = TeleportVFXComponent->OnTeleportVFXFinished.AddUObject(
		this,
		&UPRFaerinCombatDirectorComponent::HandleTeleportVFXFinished);

	LoopState = EPRFaerinCombatLoopState::PreparingTeleportVFX;
	PendingTeleportVFXAbilityRole = ObservedRole;
	if (!TeleportVFXComponent->StartTeleportVFX(PatternPlan, ActiveTarget))
	{
		ClearTeleportVFXFinishedDelegate();
		PendingTeleportVFXAbilityRole = EPRFaerinObservedAbilityRole::None;
		LoopState = EPRFaerinCombatLoopState::SelectingPattern;
		return false;
	}

	return true;
}

void UPRFaerinCombatDirectorComponent::HandleTeleportVFXFinished(const bool bSucceeded)
{
	const EPRFaerinObservedAbilityRole AbilityRole = PendingTeleportVFXAbilityRole;
	ClearTeleportVFXFinishedDelegate();
	PendingTeleportVFXAbilityRole = EPRFaerinObservedAbilityRole::None;

	if (!bSucceeded)
	{
		FinishLoopStep(false);
		return;
	}

	if (!ActivatePatternAbility(ActivePatternPlan, AbilityRole))
	{
		FinishLoopStep(false);
	}
}

void UPRFaerinCombatDirectorComponent::ClearTeleportVFXFinishedDelegate()
{
	if (!TeleportVFXFinishedDelegateHandle.IsValid())
	{
		return;
	}

	if (UPRFaerinTeleportVFXComponent* TeleportVFXComponent = GetTeleportVFXComponent())
	{
		TeleportVFXComponent->OnTeleportVFXFinished.Remove(TeleportVFXFinishedDelegateHandle);
	}

	TeleportVFXFinishedDelegateHandle = FDelegateHandle();
}

UPRFaerinTeleportVFXComponent* UPRFaerinCombatDirectorComponent::GetTeleportVFXComponent() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor)
		? OwnerActor->FindComponentByClass<UPRFaerinTeleportVFXComponent>()
		: nullptr;
}

bool UPRFaerinCombatDirectorComponent::ActivatePatternAbility(
	const FPRFaerinPatternPlan& PatternPlan,
	EPRFaerinObservedAbilityRole ObservedRole)
{
	if (!IsValid(AbilitySystemComponent) || !PatternPlan.AbilityTag.IsValid())
	{
		return false;
	}

	if (!AbilitySystemComponent->TryActivateAbilityOnServer(PatternPlan.AbilityTag, ActiveAbilityHandle))
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin 패턴 Ability 활성화에 실패했다. Owner=%s, AbilityTag=%s"),
			*GetNameSafe(GetOwner()),
			*PatternPlan.AbilityTag.ToString());
		return false;
	}

	if (ObservedRole == EPRFaerinObservedAbilityRole::Pattern)
	{
		bHasStartedMainPatternInCurrentPhase = true;
	}

	LoopState = EPRFaerinCombatLoopState::WaitingPatternEnd;
	ObservedAbilityRole = ObservedRole;
	BindAbilityEndDelegate();

	if (LoopStepFailSafeSeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				LoopStepFailSafeTimerHandle,
				this,
				&UPRFaerinCombatDirectorComponent::HandleLoopStepFailSafeElapsed,
				LoopStepFailSafeSeconds,
				false);
		}
	}

	if (IsObservedAbilityActive())
	{
		return true;
	}

	if (ObservedRole == EPRFaerinObservedAbilityRole::PrePatternPortal)
	{
		ClearAbilityEndDelegate();
		ActiveAbilityHandle = FGameplayAbilitySpecHandle();
		ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
				this,
				&UPRFaerinCombatDirectorComponent::HandlePrePatternPortalAssistFinished,
				true));
			return true;
		}

		HandlePrePatternPortalAssistFinished(true);
		return true;
	}

	QueueFinishLoopStep(true);
	return true;
}

bool UPRFaerinCombatDirectorComponent::TrySelectPrePatternPortalAssistPlan(
	UBehaviorTreeComponent& OwnerComp,
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const FPRFaerinPatternPlan& MainPatternPlan,
	FPRFaerinPatternPlan& OutAssistPlan)
{
	OutAssistPlan = FPRFaerinPatternPlan();

	if (!PhaseConfig.bUsePrePatternPortalAssist || PhaseConfig.PrePatternPortalAssistChance <= 0.0f)
	{
		return false;
	}

	if (ShouldSuppressPortalAssistForCurrentPhase())
	{
		return false;
	}

	if (IsPortalPatternPlan(MainPatternPlan)
		&& !PhaseConfig.bAllowPrePatternPortalAssistBeforePortalPatterns)
	{
		return false;
	}

	const float AssistChance = FMath::Clamp(PhaseConfig.PrePatternPortalAssistChance, 0.0f, 1.0f);
	if (FMath::FRand() > AssistChance)
	{
		return false;
	}

	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	TArray<const FPRPatternRule*> MatchedRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		ResolvePrePatternPortalGroupTag(PhaseConfig),
		true,
		EPRPatternContextMatchMode::FullMatch,
		MatchedRules);

	TArray<FPRFaerinPatternCandidate> Candidates;
	float TotalWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		if (PatternRule == nullptr
			|| PatternRule->AbilityTag.MatchesTagExact(MainPatternPlan.AbilityTag))
		{
			continue;
		}

		const FPRFaerinPatternLoopMetadata* LoopMetadata = LoopDataAsset->FindPatternMetadata(PatternRule->AbilityTag);
		if (LoopMetadata == nullptr || !LoopMetadata->bEnabled || PatternRule->SelectionWeight <= 0.0f)
		{
			continue;
		}

		if (!CanActivatePatternAbilityTag(PatternRule->AbilityTag))
		{
			continue;
		}

		FPRFaerinPatternCandidate Candidate;
		Candidate.PatternRule = PatternRule;
		Candidate.LoopMetadata = LoopMetadata;
		Candidate.FinalWeight = PatternRule->SelectionWeight;
		Candidates.Add(Candidate);
		TotalWeight += Candidate.FinalWeight;
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return false;
	}

	const float PickValue = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;
	const FPRFaerinPatternCandidate* SelectedCandidate = &Candidates.Last();
	for (const FPRFaerinPatternCandidate& Candidate : Candidates)
	{
		AccumulatedWeight += Candidate.FinalWeight;
		if (PickValue <= AccumulatedWeight)
		{
			SelectedCandidate = &Candidate;
			break;
		}
	}

	OutAssistPlan.AbilityTag = SelectedCandidate->PatternRule->AbilityTag;
	OutAssistPlan.PatternRule = *SelectedCandidate->PatternRule;
	OutAssistPlan.LoopMetadata = *SelectedCandidate->LoopMetadata;
	OutAssistPlan.FinalSelectionWeight = SelectedCandidate->FinalWeight;
	ActiveTarget = PatternRuntime.CurrentTarget;

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Verbose,
			TEXT("Faerin 선행 포털 보조 선택. Owner=%s, PortalAbilityTag=%s, MainAbilityTag=%s, Phase=%s"),
			*GetNameSafe(PatternRuntime.ControlledPawn),
			*OutAssistPlan.AbilityTag.ToString(),
			*MainPatternPlan.AbilityTag.ToString(),
			*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PatternRuntime.PatternContext.BossPhase)));
	}

	return true;
}

FGameplayTag UPRFaerinCombatDirectorComponent::ResolvePrePatternPortalGroupTag(
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	return PhaseConfig.PrePatternPortalGroupTag.IsValid()
		? PhaseConfig.PrePatternPortalGroupTag
		: PRGameplayTags::Pattern_Boss_Faerin_Portal;
}

bool UPRFaerinCombatDirectorComponent::IsPortalPatternPlan(const FPRFaerinPatternPlan& PatternPlan) const
{
	return PatternPlan.PatternRule.PatternGroupTag.MatchesTag(PRGameplayTags::Pattern_Boss_Faerin_Portal);
}

bool UPRFaerinCombatDirectorComponent::ShouldSuppressPortalAssistForCurrentPhase() const
{
	const EPRBossPhase CurrentPhase = bHasPeriodicSidePatternPhase
		? PeriodicSidePatternPhase
		: ResolveCurrentPhase();
	return IsFirstMainPatternPortalSuppressionPhase(CurrentPhase)
		&& !bHasStartedMainPatternInCurrentPhase;
}

bool UPRFaerinCombatDirectorComponent::IsFirstMainPatternPortalSuppressionPhase(const EPRBossPhase Phase) const
{
	return Phase == EPRBossPhase::Phase3 || Phase == EPRBossPhase::Phase4;
}

void UPRFaerinCombatDirectorComponent::RefreshPeriodicSidePatternPhase(const EPRBossPhase CurrentPhase)
{
	if (bHasPeriodicSidePatternPhase && PeriodicSidePatternPhase == CurrentPhase)
	{
		return;
	}

	PeriodicSidePatternPhase = CurrentPhase;
	bHasPeriodicSidePatternPhase = true;
	PeriodicSidePatternNextAllowedTimes.Reset();
	bHasStartedMainPatternInCurrentPhase = false;
}

void UPRFaerinCombatDirectorComponent::TryActivatePeriodicSidePatterns(
	const FPRFaerinPhaseLoopConfig& PhaseConfig)
{
	if (ShouldSuppressPortalAssistForCurrentPhase())
	{
		return;
	}

	for (const FPRFaerinPeriodicSidePatternConfig& SidePatternConfig : PhaseConfig.PeriodicSidePatterns)
	{
		if (TryActivatePeriodicSidePattern(SidePatternConfig))
		{
			return;
		}
	}
}

bool UPRFaerinCombatDirectorComponent::TryActivatePeriodicSidePattern(
	const FPRFaerinPeriodicSidePatternConfig& SidePatternConfig)
{
	if (!SidePatternConfig.bEnabled
		|| !SidePatternConfig.AbilityTag.IsValid()
		|| SidePatternConfig.IntervalSeconds <= 0.0f
		|| SidePatternConfig.ActivationChance <= 0.0f
		|| !IsValid(AbilitySystemComponent))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float InitialDelay = FMath::Max(SidePatternConfig.InitialDelaySeconds, 0.0f);
	const float Interval = FMath::Max(SidePatternConfig.IntervalSeconds, UE_SMALL_NUMBER);

	if (float* NextAllowedTime = PeriodicSidePatternNextAllowedTimes.Find(SidePatternConfig.AbilityTag))
	{
		if (CurrentTime < *NextAllowedTime)
		{
			return false;
		}
	}
	else
	{
		const float FirstAllowedTime = CurrentTime + InitialDelay;
		PeriodicSidePatternNextAllowedTimes.Add(SidePatternConfig.AbilityTag, FirstAllowedTime);
		if (CurrentTime < FirstAllowedTime)
		{
			return false;
		}
	}

	PeriodicSidePatternNextAllowedTimes.Add(SidePatternConfig.AbilityTag, CurrentTime + Interval);

	if (FMath::FRand() > FMath::Clamp(SidePatternConfig.ActivationChance, 0.0f, 1.0f))
	{
		return false;
	}

	if (!CanActivatePatternAbilityTag(SidePatternConfig.AbilityTag))
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPRFaerinCombatDirector,
				Verbose,
				TEXT("Faerin 주기 보조 패턴 활성화 조건 미충족. Owner=%s, AbilityTag=%s"),
				*GetNameSafe(GetOwner()),
				*SidePatternConfig.AbilityTag.ToString());
		}
		return false;
	}

	FGameplayAbilitySpecHandle SidePatternAbilityHandle;
	const bool bActivated = AbilitySystemComponent->TryActivateAbilityOnServer(
		SidePatternConfig.AbilityTag,
		SidePatternAbilityHandle);

	if (bActivated && PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Verbose,
			TEXT("Faerin 주기 보조 패턴 활성화. Owner=%s, AbilityTag=%s, Phase=%s"),
			*GetNameSafe(GetOwner()),
			*SidePatternConfig.AbilityTag.ToString(),
			*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PeriodicSidePatternPhase)));
	}

	return bActivated;
}

void UPRFaerinCombatDirectorComponent::BindAbilityEndDelegate()
{
	ClearAbilityEndDelegate();

	UAbilitySystemComponent* BaseAbilitySystemComponent = AbilitySystemComponent.Get();
	if (!IsValid(BaseAbilitySystemComponent))
	{
		return;
	}

	AbilityEndedDelegateHandle = BaseAbilitySystemComponent->OnAbilityEnded.AddUObject(
		this,
		&UPRFaerinCombatDirectorComponent::HandleObservedAbilityEnded);
}

void UPRFaerinCombatDirectorComponent::ClearAbilityEndDelegate()
{
	if (!AbilityEndedDelegateHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* BaseAbilitySystemComponent = AbilitySystemComponent.Get();
	if (IsValid(BaseAbilitySystemComponent))
	{
		BaseAbilitySystemComponent->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}

	AbilityEndedDelegateHandle = FDelegateHandle();
}

bool UPRFaerinCombatDirectorComponent::IsObservedAbilityActive() const
{
	if (!IsValid(AbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpec* ActiveSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	return ActiveSpec != nullptr && ActiveSpec->IsActive();
}

void UPRFaerinCombatDirectorComponent::HandleObservedAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (!ActiveAbilityHandle.IsValid() || EndedData.AbilitySpecHandle != ActiveAbilityHandle)
	{
		return;
	}

	const EPRFaerinObservedAbilityRole FinishedRole = ObservedAbilityRole;
	const bool bSucceeded = !EndedData.bWasCancelled;
	ClearAbilityEndDelegate();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;

	if (FinishedRole == EPRFaerinObservedAbilityRole::Pattern)
	{
		HandlePatternExecutionFinished(bSucceeded);
		return;
	}

	if (FinishedRole == EPRFaerinObservedAbilityRole::PrePatternPortal)
	{
		HandlePrePatternPortalAssistFinished(bSucceeded);
		return;
	}

	if (FinishedRole == EPRFaerinObservedAbilityRole::PrePatternApproach)
	{
		const bool bCanStartMainPattern = bSucceeded && CanStartMainPatternAfterPrePatternApproach();
		if (UWorld* World = GetWorld())
		{
			LastApproachEndTime = World->GetTimeSeconds();
		}

		ClearActiveApproachSprintRequest();
		ClearActiveNearTeleportRequest();
		HandlePrePatternApproachFinished(bCanStartMainPattern);
		return;
	}

	if (FinishedRole == EPRFaerinObservedAbilityRole::Approach)
	{
		if (UWorld* World = GetWorld())
		{
			LastApproachEndTime = World->GetTimeSeconds();
		}

		ClearActiveApproachSprintRequest();
		ClearActiveNearTeleportRequest();
		FinishLoopStep(bSucceeded);
		return;
	}

	FinishLoopStep(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::HandlePrePatternPortalAssistFinished(bool bSucceeded)
{
	if (!bSucceeded || !bHasPendingMainPatternPlan)
	{
		PendingMainPatternPlan = FPRFaerinPatternPlan();
		bHasPendingMainPatternPlan = false;
		FinishLoopStep(false);
		return;
	}

	ActivePatternPlan = PendingMainPatternPlan;
	PendingMainPatternPlan = FPRFaerinPatternPlan();
	bHasPendingMainPatternPlan = false;

	if (!StartPatternPlan(ActivePatternPlan))
	{
		FinishLoopStep(false);
	}
}

void UPRFaerinCombatDirectorComponent::HandlePrePatternApproachFinished(bool bSucceeded)
{
	ClearActiveApproachSprintRequest();
	ClearActiveNearTeleportRequest();

	if (!bSucceeded || !bHasPendingMainPatternPlan)
	{
		PendingMainPatternPlan = FPRFaerinPatternPlan();
		bHasPendingMainPatternPlan = false;
		FinishLoopStep(false);
		return;
	}

	ActivePatternPlan = PendingMainPatternPlan;
	PendingMainPatternPlan = FPRFaerinPatternPlan();
	bHasPendingMainPatternPlan = false;

	if (!StartPatternPlan(ActivePatternPlan))
	{
		FinishLoopStep(false);
	}
}

bool UPRFaerinCombatDirectorComponent::CanStartMainPatternAfterPrePatternApproach() const
{
	if (!ActiveApproachSprintRequest.bIsValid)
	{
		return true;
	}

	const AActor* OwnerActor = GetOwner();
	const AActor* TargetActor = ActiveApproachSprintRequest.TargetActor.Get();
	if (!IsValid(OwnerActor) || !IsValid(TargetActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), TargetActor->GetActorLocation());
	const float AllowedDistance = FMath::Max(ActiveApproachSprintRequest.StopDistance, 0.0f)
		+ FMath::Max(ActiveApproachSprintRequest.AcceptanceRadius, 0.0f);
	return DistanceToTarget <= AllowedDistance;
}

void UPRFaerinCombatDirectorComponent::HandlePatternExecutionFinished(bool bSucceeded)
{
	if (!bSucceeded)
	{
		FinishLoopStep(false);
		return;
	}

	if (TryStartShiftFollowUpSpoke())
	{
		return;
	}

	if (!IsValid(LoopDataAsset))
	{
		FinishLoopStep(true);
		return;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig != nullptr && ShouldUsePhase12PostPatternStrafeLoop(*PhaseConfig))
	{
		if (StartPostPatternStrafe(*PhaseConfig))
		{
			return;
		}

		FinishLoopStep(true);
		return;
	}

	if (PhaseConfig != nullptr
		&& ShouldRunUrgentPatternRangeApproach(ActivePatternPlan, *PhaseConfig)
		&& StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	if (PhaseConfig != nullptr
		&& ShouldRunPostPatternStrafe(ActivePatternPlan)
		&& StartPostPatternStrafe(*PhaseConfig))
	{
		return;
	}

	if (PhaseConfig != nullptr && StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	FinishLoopStep(true);
}

bool UPRFaerinCombatDirectorComponent::TryStartShiftFollowUpSpoke()
{
	if (!IsPrePatternApproachPhase(ResolveCurrentPhase())
		|| !ActivePatternPlan.AbilityTag.MatchesTagExact(PRGameplayTags::Ability_Boss_Faerin_ShiftPlayerClose))
	{
		return false;
	}

	FPRFaerinPatternPlan SpokePlan;
	if (!BuildForcedPatternPlanByTag(PRGameplayTags::Ability_Boss_Faerin_SpokeCombo, SpokePlan, true)
		|| !StartForcedFollowUpPatternPlan(SpokePlan))
	{
		FinishLoopStep(true);
		return true;
	}

	return true;
}

bool UPRFaerinCombatDirectorComponent::StartForcedFollowUpPatternPlan(const FPRFaerinPatternPlan& PatternPlan)
{
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_Boss_Faerin_ForcedFollowUp);
	const bool bStarted = StartPatternPlan(PatternPlan);
	AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Boss_Faerin_ForcedFollowUp);
	return bStarted;
}

bool UPRFaerinCombatDirectorComponent::ShouldUsePhase12PostPatternStrafeLoop(
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	return PhaseConfig.bUsePrePatternApproachRoute && IsPrePatternApproachPhase(PhaseConfig.Phase);
}

bool UPRFaerinCombatDirectorComponent::BuildForcedPatternPlanByTag(
	const FGameplayTag& AbilityTag,
	FPRFaerinPatternPlan& OutPlan,
	const bool bIgnoreCooldown) const
{
	OutPlan = FPRFaerinPatternPlan();

	if (!AbilityTag.IsValid()
		|| !IsValid(PatternDataAsset)
		|| !IsValid(LoopDataAsset))
	{
		return false;
	}

	if (!bIgnoreCooldown && !CanActivatePatternAbilityTag(AbilityTag))
	{
		return false;
	}

	const EPRBossPhase CurrentPhase = ResolveCurrentPhase();
	const FPRFaerinPatternLoopMetadata* FoundMetadata = LoopDataAsset->FindPatternMetadata(AbilityTag);
	if (FoundMetadata == nullptr || !FoundMetadata->bEnabled)
	{
		return false;
	}

	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (!PatternRule.AbilityTag.MatchesTagExact(AbilityTag) || !PatternRule.IsValid())
		{
			continue;
		}

		if (PatternRule.bRestrictBossPhases && !PatternRule.AllowedBossPhases.Contains(CurrentPhase))
		{
			continue;
		}

		OutPlan.AbilityTag = PatternRule.AbilityTag;
		OutPlan.PatternRule = PatternRule;
		OutPlan.LoopMetadata = *FoundMetadata;
		OutPlan.FinalSelectionWeight = FMath::Max(PatternRule.SelectionWeight, 0.0f);
		return true;
	}

	return false;
}

bool UPRFaerinCombatDirectorComponent::ShouldRunPostPatternStrafe(const FPRFaerinPatternPlan& PatternPlan) const
{
	if (PatternPlan.LoopMetadata.PostPatternPolicy == EPRFaerinPostPatternPolicy::ForceImmediateNext)
	{
		return false;
	}

	if (PatternPlan.LoopMetadata.PostPatternPolicy == EPRFaerinPostPatternPolicy::ForceStrafe)
	{
		return true;
	}

	return false;
}

bool UPRFaerinCombatDirectorComponent::ShouldRunUrgentPatternRangeApproach(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (!IsValid(ActiveTarget))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(PatternPlan, PhaseConfig);
	if (ApproachPolicy == EPRFaerinApproachPolicy::None
		|| ApproachPolicy == EPRFaerinApproachPolicy::PhaseDefault
		|| ApproachPolicy == EPRFaerinApproachPolicy::ShiftClose)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	return DistanceToTarget > PatternPlan.PatternRule.MaxRange;
}

bool UPRFaerinCombatDirectorComponent::StartPostPatternStrafe(const FPRFaerinPhaseLoopConfig& PhaseConfig)
{
	const float StrafeDuration = CalculateStrafeDuration();
	if (StrafeDuration <= 0.0f || !IsValid(ActiveTarget))
	{
		return false;
	}

	FVector StrafeDestination = FVector::ZeroVector;
	if (!ResolvePostPatternStrafeDestination(PhaseConfig, StrafeDestination))
	{
		return false;
	}

	AAIController* AIController = GetOwnerAIController();
	if (!IsValid(AIController) || !IsValid(OwnerEnemy))
	{
		return false;
	}

	LoopState = EPRFaerinCombatLoopState::PostPatternStrafe;
	OwnerEnemy->ApplyCombatMovePresentationContext(PhaseConfig.StrafePresentationConfig);
	AIController->SetFocus(ActiveTarget, EAIFocusPriority::Gameplay);

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		StrafeDestination,
		PhaseConfig.StrafeAcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		OwnerEnemy->ClearCombatMovePresentationContext();
		return false;
	}

	if (PhaseConfig.bAlternateStrafeDirection)
	{
		NextStrafeDirectionSign *= -1;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PostPatternStrafeTimerHandle,
			this,
			&UPRFaerinCombatDirectorComponent::HandlePostPatternStrafeElapsed,
			StrafeDuration,
			false);
		return true;
	}

	AIController->StopMovement();
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	OwnerEnemy->ClearCombatMovePresentationContext();
	return false;
}

bool UPRFaerinCombatDirectorComponent::ResolvePostPatternStrafeDestination(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	FVector& OutDestination) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(ActiveTarget))
	{
		return false;
	}

	FVector FromTarget = OwnerActor->GetActorLocation() - ActiveTarget->GetActorLocation();
	FromTarget.Z = 0.0f;

	if (!FromTarget.Normalize())
	{
		FromTarget = -ActiveTarget->GetActorForwardVector();
		FromTarget.Z = 0.0f;
		if (!FromTarget.Normalize())
		{
			FromTarget = FVector::ForwardVector;
		}
	}

	const float StrafeRadius = PhaseConfig.StrafeRadiusOverride > 0.0f
		? PhaseConfig.StrafeRadiusOverride
		: 300.0f;
	const float SignedArcAngle = PhaseConfig.StrafeArcAngleDegrees * static_cast<float>(NextStrafeDirectionSign);
	const FVector StrafeDirection = FromTarget.RotateAngleAxis(SignedArcAngle, FVector::UpVector);
	const FVector RawDestination = OwnerActor->GetActorLocation() + (StrafeDirection * StrafeRadius);

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(
				RawDestination,
				ProjectedLocation,
				PhaseConfig.StrafeNavProjectExtent))
			{
				OutDestination = ProjectedLocation.Location;
				return true;
			}
		}
	}

	OutDestination = RawDestination;
	return true;
}

void UPRFaerinCombatDirectorComponent::HandlePostPatternStrafeElapsed()
{
	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = IsValid(LoopDataAsset)
		? LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase())
		: nullptr;
	if (PhaseConfig != nullptr && ShouldUsePhase12PostPatternStrafeLoop(*PhaseConfig))
	{
		FinishLoopStep(true);
		return;
	}

	if (PhaseConfig != nullptr && StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	FinishLoopStep(true);
}

bool UPRFaerinCombatDirectorComponent::ShouldRunPostStrafeApproach(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (!IsValid(ActiveTarget))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(PatternPlan, PhaseConfig);
	if (ApproachPolicy == EPRFaerinApproachPolicy::None
		|| ApproachPolicy == EPRFaerinApproachPolicy::PhaseDefault
		|| ApproachPolicy == EPRFaerinApproachPolicy::ShiftClose)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	if (DistanceToTarget > PatternPlan.PatternRule.MaxRange)
	{
		return true;
	}

	if (ApproachPolicy == EPRFaerinApproachPolicy::KeepCurrentRange)
	{
		return false;
	}

	return DistanceToTarget > PhaseConfig.ApproachStopDistance;
}

bool UPRFaerinCombatDirectorComponent::StartPostStrafeApproach(const FPRFaerinPhaseLoopConfig& PhaseConfig)
{
	if (IsApproachSprintRepeatBlocked())
	{
		return false;
	}

	if (!ShouldRunPostStrafeApproach(ActivePatternPlan, PhaseConfig))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(ActivePatternPlan, PhaseConfig);
	const bool bUseNearTeleport = ShouldUseNearTeleportApproach(ApproachPolicy, PhaseConfig);
	return StartApproachAbility(
		PhaseConfig,
		bUseNearTeleport,
		EPRFaerinNearTeleportPlacementMode::SelfEQS,
		EPRFaerinObservedAbilityRole::Approach);
}

bool UPRFaerinCombatDirectorComponent::StartApproachAbility(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const bool bUseNearTeleport,
	const EPRFaerinNearTeleportPlacementMode TeleportPlacementMode,
	const EPRFaerinObservedAbilityRole InObservedAbilityRole)
{
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	const FGameplayTag ApproachAbilityTag = bUseNearTeleport
		? PhaseConfig.NearTeleportAbilityTag
		: PhaseConfig.ApproachAbilityTag;
	if (!ApproachAbilityTag.IsValid())
	{
		return false;
	}

	if (bUseNearTeleport)
	{
		FPRFaerinNearTeleportRequest Request;
		if (!BuildNearTeleportRequest(PhaseConfig, TeleportPlacementMode, Request))
		{
			return false;
		}

		LoopState = EPRFaerinCombatLoopState::ApproachNearTeleport;
		ActiveNearTeleportRequest = Request;
		ClearActiveApproachSprintRequest();
	}
	else
	{
		FPRFaerinApproachSprintRequest Request;
		if (!BuildApproachSprintRequest(PhaseConfig, InObservedAbilityRole, Request))
		{
			return false;
		}

		LoopState = EPRFaerinCombatLoopState::ApproachSprint;
		ActiveApproachSprintRequest = Request;
		ClearActiveNearTeleportRequest();
	}

	if (!AbilitySystemComponent->TryActivateAbilityOnServer(ApproachAbilityTag, ActiveAbilityHandle))
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin 접근 Ability 활성화에 실패했다. Owner=%s, AbilityTag=%s"),
			*GetNameSafe(GetOwner()),
			*ApproachAbilityTag.ToString());
		ClearActiveApproachSprintRequest();
		ClearActiveNearTeleportRequest();
		return false;
	}

	ObservedAbilityRole = InObservedAbilityRole;
	BindAbilityEndDelegate();

	if (IsObservedAbilityActive())
	{
		return true;
	}

	if (InObservedAbilityRole == EPRFaerinObservedAbilityRole::PrePatternApproach)
	{
		ClearAbilityEndDelegate();
		ActiveAbilityHandle = FGameplayAbilitySpecHandle();
		ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
				this,
				&UPRFaerinCombatDirectorComponent::HandlePrePatternApproachFinished,
				true));
			return true;
		}

		HandlePrePatternApproachFinished(true);
		return true;
	}

	QueueFinishLoopStep(true);
	return true;
}

bool UPRFaerinCombatDirectorComponent::BuildApproachSprintRequest(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const EPRFaerinObservedAbilityRole InObservedAbilityRole,
	FPRFaerinApproachSprintRequest& OutRequest) const
{
	OutRequest = FPRFaerinApproachSprintRequest();

	if (!IsValid(GetOwner()) || !IsValid(ActiveTarget))
	{
		return false;
	}

	OutRequest.bIsValid = true;
	OutRequest.TargetActor = ActiveTarget;
	OutRequest.StopDistance = ResolveApproachSprintStopDistance(PhaseConfig, InObservedAbilityRole);
	OutRequest.TimeoutSeconds = PhaseConfig.ApproachTimeoutSeconds;
	OutRequest.AcceptanceRadius = PhaseConfig.ApproachAcceptanceRadius;
	OutRequest.NavProjectExtent = PhaseConfig.ApproachNavProjectExtent;
	OutRequest.PresentationConfig = PhaseConfig.ApproachPresentationConfig;
	return true;
}

float UPRFaerinCombatDirectorComponent::ResolveApproachSprintStopDistance(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const EPRFaerinObservedAbilityRole InObservedAbilityRole) const
{
	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(ActivePatternPlan, PhaseConfig);
	float StopDistance = PhaseConfig.ApproachStopDistance;
	if (ApproachPolicy == EPRFaerinApproachPolicy::KeepCurrentRange)
	{
		StopDistance = FMath::Clamp(
			ActivePatternPlan.PatternRule.MaxRange * 0.85f,
			ActivePatternPlan.PatternRule.MinRange,
			ActivePatternPlan.PatternRule.MaxRange);
	}

	const AActor* OwnerActor = GetOwner();
	if (InObservedAbilityRole != EPRFaerinObservedAbilityRole::PrePatternApproach
		|| !IsPrePatternApproachPhase(PhaseConfig.Phase)
		|| !IsSpokePatternPlan(ActivePatternPlan)
		|| !IsValid(OwnerActor)
		|| !IsValid(ActiveTarget))
	{
		return StopDistance;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	const float MinDistance = ResolveEffectivePrePatternApproachMinDistance(PhaseConfig);
	const float CloseMaxDistance = FMath::Max(PhaseConfig.PrePatternApproachCloseMaxDistance, MinDistance);
	if (DistanceToTarget > CloseMaxDistance)
	{
		return StopDistance;
	}

	const float CloseSprintStopDistance = FMath::Max(MinDistance, PhaseConfig.ApproachAcceptanceRadius);
	return FMath::Min(StopDistance, CloseSprintStopDistance);
}

bool UPRFaerinCombatDirectorComponent::BuildNearTeleportRequest(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	const EPRFaerinNearTeleportPlacementMode PlacementMode,
	FPRFaerinNearTeleportRequest& OutRequest) const
{
	OutRequest = FPRFaerinNearTeleportRequest();

	if (!IsValid(GetOwner()) || !IsValid(ActiveTarget))
	{
		return false;
	}

	OutRequest.bIsValid = true;
	OutRequest.TargetActor = ActiveTarget;
	OutRequest.PlacementMode = PlacementMode;
	OutRequest.TargetBackDistance = PhaseConfig.TargetBackTeleportDistance;
	OutRequest.TargetBackNavProjectExtent = PhaseConfig.TargetBackTeleportNavProjectExtent;
	return true;
}

bool UPRFaerinCombatDirectorComponent::ShouldUseNearTeleportApproach(
	EPRFaerinApproachPolicy ApproachPolicy,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (ApproachPolicy == EPRFaerinApproachPolicy::NearTeleportToMeleeRange)
	{
		return true;
	}

	if (ApproachPolicy != EPRFaerinApproachPolicy::SprintOrNearTeleport)
	{
		return false;
	}

	const float SelectionChance = ResolveNearTeleportSelectionChance(PhaseConfig.NearTeleportAbilityTag);
	return FMath::FRand() <= FMath::Clamp(SelectionChance, 0.0f, 1.0f);
}

float UPRFaerinCombatDirectorComponent::ResolveNearTeleportSelectionChance(const FGameplayTag& AbilityTag) const
{
	if (!IsValid(AbilitySystemComponent) || !AbilityTag.IsValid())
	{
		return 0.0f;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
	for (const FGameplayAbilitySpec* MatchingSpec : MatchingSpecs)
	{
		if (MatchingSpec == nullptr)
		{
			continue;
		}

		const UPRGameplayAbility_FaerinNearTeleportApproach* NearTeleportAbility =
			Cast<UPRGameplayAbility_FaerinNearTeleportApproach>(MatchingSpec->Ability);
		if (IsValid(NearTeleportAbility))
		{
			return NearTeleportAbility->GetApproachSelectionChance();
		}
	}

	return 0.0f;
}

void UPRFaerinCombatDirectorComponent::ClearActiveApproachSprintRequest()
{
	ActiveApproachSprintRequest = FPRFaerinApproachSprintRequest();
}

void UPRFaerinCombatDirectorComponent::ClearActiveNearTeleportRequest()
{
	ActiveNearTeleportRequest = FPRFaerinNearTeleportRequest();
}

EPRFaerinApproachPolicy UPRFaerinCombatDirectorComponent::ResolveApproachPolicy(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (PatternPlan.LoopMetadata.ApproachPolicy == EPRFaerinApproachPolicy::PhaseDefault)
	{
		return EPRFaerinApproachPolicy::None;
	}

	return PatternPlan.LoopMetadata.ApproachPolicy;
}

bool UPRFaerinCombatDirectorComponent::IsApproachSprintRepeatBlocked() const
{
	if (ApproachSprintRepeatBlockSeconds <= 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	return World->GetTimeSeconds() - LastApproachEndTime < ApproachSprintRepeatBlockSeconds;
}

void UPRFaerinCombatDirectorComponent::HandleLoopStepFailSafeElapsed()
{
	UE_LOG(
		LogPRFaerinCombatDirector,
		Warning,
		TEXT("Faerin 전투 루프가 제한 시간을 초과했다. Owner=%s, State=%s, AbilityTag=%s"),
		*GetNameSafe(GetOwner()),
		*StaticEnum<EPRFaerinCombatLoopState>()->GetNameStringByValue(static_cast<int64>(LoopState)),
		ActivePatternPlan.AbilityTag.IsValid() ? *ActivePatternPlan.AbilityTag.ToString() : TEXT("None"));

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsObservedAbilityActive())
	{
		ClearAbilityEndDelegate();
		AbilitySystemComponent->CancelAbilityHandle(ActiveAbilityHandle);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	FinishLoopStep(false);
}

void UPRFaerinCombatDirectorComponent::FinishLoopStep(bool bSucceeded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopStepFailSafeTimerHandle);
		World->GetTimerManager().ClearTimer(PostPatternStrafeTimerHandle);
	}

	ClearTeleportVFXFinishedDelegate();
	if (UPRFaerinTeleportVFXComponent* TeleportVFXComponent = GetTeleportVFXComponent())
	{
		TeleportVFXComponent->CancelTeleportVFX();
	}

	ClearAbilityEndDelegate();
	EndLoopTargetCommit();

	ActivePatternPlan = FPRFaerinPatternPlan();
	PendingMainPatternPlan = FPRFaerinPatternPlan();
	bHasPendingMainPatternPlan = false;
	ClearActiveApproachSprintRequest();
	ClearActiveNearTeleportRequest();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;
	PendingTeleportVFXAbilityRole = EPRFaerinObservedAbilityRole::None;
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::Idle;

	OnFaerinLoopStepFinished.Broadcast(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::QueueFinishLoopStep(bool bSucceeded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
			this,
			&UPRFaerinCombatDirectorComponent::FinishLoopStep,
			bSucceeded));
		return;
	}

	FinishLoopStep(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::LogValidationErrors(const TArray<FString>& Errors) const
{
	for (const FString& Error : Errors)
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin LoopData 검증: %s"),
			*Error);
	}
}

EPRBossPhase UPRFaerinCombatDirectorComponent::ResolveCurrentPhase() const
{
	const APRBossBaseCharacter* BossOwner = Cast<APRBossBaseCharacter>(GetOwner());
	return IsValid(BossOwner)
		? BossOwner->GetCurrentPhase()
		: EPRBossPhase::Phase1;
}

void UPRFaerinCombatDirectorComponent::LogSelectedPattern(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPREnemyPatternQueryRuntime& PatternRuntime) const
{
	if (!PREnemyAIDebug::IsPatternLogEnabled())
	{
		return;
	}

	UE_LOG(
		LogPRFaerinCombatDirector,
		Verbose,
		TEXT("Faerin 패턴 선택. Owner=%s, AbilityTag=%s, Phase=%s, Distance=%.1f, FinalWeight=%.2f"),
		*GetNameSafe(PatternRuntime.ControlledPawn),
		*PatternPlan.AbilityTag.ToString(),
		*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PatternRuntime.PatternContext.BossPhase)),
		PatternRuntime.PatternContext.DistanceToTarget,
		PatternPlan.FinalSelectionWeight);
}

AAIController* UPRFaerinCombatDirectorComponent::GetOwnerAIController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return IsValid(OwnerPawn)
		? Cast<AAIController>(OwnerPawn->GetController())
		: nullptr;
}
