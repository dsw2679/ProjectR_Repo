// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 기반 탐지 보정 연동)
// Author: 손승우 (일반 몬스터 및 보스 AI 인지 제어 및 비헤이비어 트리 연동 구현)
#include "PREnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

namespace
{
	const UPRCombatMoveDataAsset* ResolveCombatDataAssetFromPawn(const APawn* ControlledPawn)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return nullptr;
		}

		return EnemyInterface->GetCombatDataAsset();
	}

	FVector ResolveStimulusLocation(const AActor* Actor, const FAIStimulus& Stimulus)
	{
		if (!Stimulus.StimulusLocation.IsNearlyZero())
		{
			return Stimulus.StimulusLocation;
		}

		return IsValid(Actor) ? Actor->GetActorLocation() : FVector::ZeroVector;
	}

	bool IsSightStimulus(const UAISenseConfig_Sight* SightConfig, const FAIStimulus& Stimulus)
	{
		return IsValid(SightConfig) && Stimulus.Type == SightConfig->GetSenseID();
	}

	bool IsCurrentThreatTarget(const UPREnemyThreatComponent* ThreatComponent, const AActor* Actor)
	{
		return IsValid(ThreatComponent)
			&& IsValid(Actor)
			&& ThreatComponent->GetCurrentTarget() == Actor;
	}

	bool IsGeneralEnemyPawn(const AActor* Actor)
	{
		const APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(Actor);
		return IsValid(EnemyCharacter) && EnemyCharacter->GetCharacterRole() == EPRCharacterRole::Enemy;
	}

	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	void WriteCurrentTargetTrackingToBlackboard(
		UBlackboardComponent* BlackboardComponent,
		const FName TargetLocationKey,
		const FName LastKnownTargetLocationKey,
		const FName HasLOSKey,
		const FVector& TargetLocation,
		const FVector& LastKnownTargetLocation,
		const bool bHasLOS)
	{
		if (!IsValid(BlackboardComponent))
		{
			return;
		}

		BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
		BlackboardComponent->SetValueAsVector(LastKnownTargetLocationKey, LastKnownTargetLocation);
		BlackboardComponent->SetValueAsBool(HasLOSKey, bHasLOS);
	}

	void WriteCurrentTargetLocationToBlackboard(
		UBlackboardComponent* BlackboardComponent,
		const FName TargetLocationKey,
		const FName LastKnownTargetLocationKey,
		const FVector& TargetLocation,
		const FVector& LastKnownTargetLocation)
	{
		if (!IsValid(BlackboardComponent))
		{
			return;
		}

		BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
		BlackboardComponent->SetValueAsVector(LastKnownTargetLocationKey, LastKnownTargetLocation);
	}
}

APREnemyAIController::APREnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*EnemyPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	EnemyPerceptionComponent->ConfigureSense(*SightConfig);
	EnemyPerceptionComponent->ConfigureSense(*HearingConfig);
	EnemyPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &APREnemyAIController::HandleTargetPerceptionUpdated);
}

void APREnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(InPawn);
	if (EnemyInterface == nullptr)
	{
		return;
	}

	CachedPerceptionConfig = EnemyInterface->GetPerceptionConfig();
	ApplyPerceptionConfig(CachedPerceptionConfig);

	CachedThreatComponent = EnemyInterface->GetEnemyThreatComponent();
	if (IsValid(CachedThreatComponent))
	{
		// CombatDataAsset이 UPREnemyCombatDataAsset이면 그 TargetingConfig를 기본값으로 사용한다.
		// 보스처럼 UPRBossCombatDataAsset(UPREnemyCombatDataAsset 미상속)을 쓰는 경우엔 struct 기본값에서 시작하되,
		// 캐릭터별 CustomizeEnemyTargetingConfig는 데이터 에셋 타입과 무관하게 항상 적용한다.
		// (이전에는 캐스팅 실패 시 블록 전체가 스킵되어 보스의 TargetingConfig override가 적용되지 않았다.)
		FPREnemyTargetingConfig TargetingConfig;
		if (const UPREnemyCombatDataAsset* EnemyCombatDataAsset = Cast<UPREnemyCombatDataAsset>(EnemyInterface->GetCombatDataAsset()))
		{
			TargetingConfig = EnemyCombatDataAsset->TargetingConfig;
		}
		EnemyInterface->CustomizeEnemyTargetingConfig(TargetingConfig);
		CachedThreatComponent->SetTargetingConfig(TargetingConfig);

		CachedThreatComponent->OnTargetChanged.AddDynamic(this, &APREnemyAIController::HandleThreatTargetChanged);
	}

	UBehaviorTree* BehaviorTreeAsset = EnemyInterface->GetBehaviorTreeAsset();
	CacheBlackboardFromBehaviorTree(BehaviorTreeAsset);

	if (IsValid(CachedBlackboardComponent))
	{
		CachedBlackboardComponent->SetValueAsVector(HomeLocationKey, EnemyInterface->GetHomeLocation());
		WriteNearbyAlertBlackboardState(false, nullptr);
		EnemyInterface->InitializeEnemyBlackboard(CachedBlackboardComponent);
		ApplyTacticalModeState(EPRTacticalMode::Idle);
	}

	if (IsValid(CachedBlackboardComponent) && IsValid(BehaviorTreeAsset))
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	bPreserveAlertOnNextTargetClear = false;
}

void APREnemyAIController::OnUnPossess()
{
	ClearCombatMovePresentationContext(true);
	ClearThreatBinding();
	CachedBlackboardComponent = nullptr;
	CachedPerceptionConfig = nullptr;
	bPreserveAlertOnNextTargetClear = false;
	bSuppressAlertPropagationForSharedAlert = false;
	bHandlingSharedAlertTargetChange = false;

	Super::OnUnPossess();
}

void APREnemyAIController::ApplyTacticalModeState(
	EPRTacticalMode NewMode,
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig* OverridePresentationConfig)
{
	SetBlackboardTacticalMode(NewMode);
	ApplyPresentationForTacticalMode(NewMode, FocusTarget, OverridePresentationConfig);

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] ApplyTacticalMode Mode=%s Override=%s Focus=%s Pawn=%s"),
			*StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(NewMode)),
			OverridePresentationConfig != nullptr ? TEXT("true") : TEXT("false"),
			*GetNameSafe(FocusTarget),
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}

	if (UPRCombatStatics::GetActorTeam(Actor) != EPRTeam::Player)
	{
		return;
	}

	const FVector StimulusLocation = ResolveStimulusLocation(Actor, Stimulus);

	if (!IsValid(CachedThreatComponent))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bIsSightStimulus = IsSightStimulus(SightConfig, Stimulus);
		bool bHasLOS = bIsSightStimulus;
		if (!bIsSightStimulus)
		{
			FPREnemyTargetCandidate ExistingCandidate;
			bHasLOS = CachedThreatComponent->GetTargetCandidate(Actor, ExistingCandidate)
				&& ExistingCandidate.bHasLOS;
		}

		CachedThreatComponent->UpdatePerceivedTarget(Actor, StimulusLocation, bHasLOS);

		// Perception은 여러 플레이어의 이벤트를 받을 수 있으므로,
		// Blackboard 추적값은 현재 공격 대상 기준일 때만 갱신한다.
		if (IsCurrentThreatTarget(CachedThreatComponent, Actor))
		{
			if (bIsSightStimulus)
			{
				WriteCurrentTargetTrackingToBlackboard(
					CachedBlackboardComponent,
					TargetLocationKey,
					LastKnownTargetLocationKey,
					HasLOSKey,
					StimulusLocation,
					StimulusLocation,
					true);
			}
			else
			{
				WriteCurrentTargetLocationToBlackboard(
					CachedBlackboardComponent,
					TargetLocationKey,
					LastKnownTargetLocationKey,
					StimulusLocation,
					StimulusLocation);
			}
		}

		bPreserveAlertOnNextTargetClear = false;
	}
	else
	{
		const bool bWasCurrentTarget = IsCurrentThreatTarget(CachedThreatComponent, Actor);

		switch (TargetLostPolicy)
		{
		case EPRTargetLostPolicy::ClearCurrentTarget:
			bPreserveAlertOnNextTargetClear = bWasCurrentTarget;
			CachedThreatComponent->MarkTargetPerceptionLost(Actor, StimulusLocation);
			break;
		case EPRTargetLostPolicy::RemoveThreatEntry:
			bPreserveAlertOnNextTargetClear = false;
			if (bWasCurrentTarget && IsValid(CachedBlackboardComponent))
			{
				CachedBlackboardComponent->ClearValue(LastKnownTargetLocationKey);
			}
			CachedThreatComponent->OnTargetLost(Actor);
			break;
		case EPRTargetLostPolicy::KeepCurrentTarget:
		default:
			bPreserveAlertOnNextTargetClear = false;
			CachedThreatComponent->MarkTargetPerceptionLost(Actor, StimulusLocation);
			break;
		}

		if (bWasCurrentTarget && IsCurrentThreatTarget(CachedThreatComponent, Actor))
		{
			WriteCurrentTargetTrackingToBlackboard(
				CachedBlackboardComponent,
				TargetLocationKey,
				LastKnownTargetLocationKey,
				HasLOSKey,
				Actor->GetActorLocation(),
				StimulusLocation,
				false);
		}
	}
}

void APREnemyAIController::HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent->SetValueAsObject(CurrentTargetKey, NewTarget);

	if (IsValid(NewTarget))
	{
		const bool bHasPreviousTarget = IsValid(OldTarget);
		FPREnemyTargetCandidate TargetCandidate;
		const bool bHasCandidateInfo = IsValid(CachedThreatComponent)
			&& CachedThreatComponent->GetTargetCandidate(NewTarget, TargetCandidate);
		const bool bHasLOS = bHasCandidateInfo ? TargetCandidate.bHasLOS : true;
		const FVector TargetLocation = bHasCandidateInfo && !TargetCandidate.LastKnownLocation.IsNearlyZero()
			? TargetCandidate.LastKnownLocation
			: NewTarget->GetActorLocation();

		WriteCurrentTargetTrackingToBlackboard(
			CachedBlackboardComponent,
			TargetLocationKey,
			LastKnownTargetLocationKey,
			HasLOSKey,
			TargetLocation,
			TargetLocation,
			bHasLOS);
		if (!bHasPreviousTarget)
		{
			const EPRTacticalMode InitialCombatMode = bHandlingSharedAlertTargetChange
				? EPRTacticalMode::FastApproach
				: EPRTacticalMode::Alert;
			ApplyTacticalModeState(InitialCombatMode, NewTarget);
			ApplyInitialAttackPressureOnAlert();

			if (!bSuppressAlertPropagationForSharedAlert)
			{
				PropagateAlertToNearbyEnemies(NewTarget);
			}
		}
		else
		{
			ApplyPresentationForTacticalMode(GetBlackboardTacticalMode(), NewTarget);
		}

		bPreserveAlertOnNextTargetClear = false;
	}
	else
	{
		CachedBlackboardComponent->ClearValue(CurrentTargetKey);
		CachedBlackboardComponent->SetValueAsBool(HasLOSKey, false);
		WriteNearbyAlertBlackboardState(false, nullptr);

		const bool bShouldInvestigate = bPreserveAlertOnNextTargetClear && HasLastKnownTargetLocation();
		ApplyTacticalModeState(bShouldInvestigate ? EPRTacticalMode::Alert : EPRTacticalMode::Return);
		bPreserveAlertOnNextTargetClear = false;
	}
}

void APREnemyAIController::PropagateAlertToNearbyEnemies(AActor* AlertTarget)
{
	if (!HasAuthority()
		|| !IsValid(CachedPerceptionConfig)
		|| !CachedPerceptionConfig->bPropagateAlertToNearbyEnemies
		|| CachedPerceptionConfig->AlertPropagationRadius <= 0.0f
		|| UPRCombatStatics::GetActorTeam(AlertTarget) != EPRTeam::Player)
	{
		return;
	}

	APawn* SourcePawn = GetPawn();
	UWorld* World = GetWorld();
	if (!IsValid(SourcePawn) || !IsValid(World) || !IsGeneralEnemyPawn(SourcePawn))
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyAlertPropagation), false, SourcePawn);
	QueryParams.AddIgnoredActor(SourcePawn);

	const FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(CachedPerceptionConfig->AlertPropagationRadius);
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		SourcePawn->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		CollisionShape,
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		APawn* NearbyPawn = Cast<APawn>(OverlapResult.GetActor());
		if (!IsValid(NearbyPawn)
			|| NearbyPawn == SourcePawn
			|| !IsGeneralEnemyPawn(NearbyPawn)
			|| UPRCombatStatics::GetActorTeam(NearbyPawn) != EPRTeam::Enemy)
		{
			continue;
		}

		APREnemyAIController* NearbyEnemyController = Cast<APREnemyAIController>(NearbyPawn->GetController());
		if (!IsValid(NearbyEnemyController))
		{
			continue;
		}

		NearbyEnemyController->ReceiveSharedAlert(this, AlertTarget);
	}
}

bool APREnemyAIController::ReceiveSharedAlert(APREnemyAIController* AlertSourceController, AActor* AlertTarget)
{
	if (!HasAuthority()
		|| !IsValid(AlertSourceController)
		|| AlertSourceController == this
		|| !CanReceiveSharedAlert(AlertTarget))
	{
		return false;
	}

	AActor* AlertSourceActor = AlertSourceController->GetPawn();
	WriteNearbyAlertBlackboardState(true, AlertSourceActor);

	const bool bPreviousSuppressAlertPropagation = bSuppressAlertPropagationForSharedAlert;
	const bool bPreviousHandlingSharedAlertTargetChange = bHandlingSharedAlertTargetChange;
	bSuppressAlertPropagationForSharedAlert = !CachedPerceptionConfig->bRelayReceivedAlert;
	bHandlingSharedAlertTargetChange = true;
	CachedThreatComponent->ForceCurrentTarget(AlertTarget);
	bSuppressAlertPropagationForSharedAlert = bPreviousSuppressAlertPropagation;
	bHandlingSharedAlertTargetChange = bPreviousHandlingSharedAlertTargetChange;

	return CachedThreatComponent->GetCurrentTarget() == AlertTarget;
}

bool APREnemyAIController::CanReceiveSharedAlert(AActor* AlertTarget) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!IsValid(CachedPerceptionConfig)
		|| !CachedPerceptionConfig->bPropagateAlertToNearbyEnemies
		|| !IsValid(CachedThreatComponent)
		|| !IsValid(CachedBlackboardComponent)
		|| !IsValid(ControlledPawn)
		|| !IsGeneralEnemyPawn(ControlledPawn)
		|| UPRCombatStatics::GetActorTeam(AlertTarget) != EPRTeam::Player)
	{
		return false;
	}

	if (IsValid(CachedThreatComponent->GetCurrentTarget()))
	{
		return false;
	}

	return true;
}

void APREnemyAIController::WriteNearbyAlertBlackboardState(bool bNearbyAlerted, AActor* AlertSourceActor)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	if (HasBlackboardKey(CachedBlackboardComponent, NearbyAIAlertedKey))
	{
		CachedBlackboardComponent->SetValueAsBool(NearbyAIAlertedKey, bNearbyAlerted);
	}

	if (HasBlackboardKey(CachedBlackboardComponent, NearbyAIAlertSourceKey))
	{
		if (bNearbyAlerted && IsValid(AlertSourceActor))
		{
			CachedBlackboardComponent->SetValueAsObject(NearbyAIAlertSourceKey, AlertSourceActor);
		}
		else
		{
			CachedBlackboardComponent->ClearValue(NearbyAIAlertSourceKey);
		}
	}
}

void APREnemyAIController::ApplyPerceptionConfig(const UPRPerceptionConfig* Config)
{
	if (!IsValid(SightConfig) || !IsValid(HearingConfig) || !IsValid(EnemyPerceptionComponent))
	{
		return;
	}

	const float SightRadius = IsValid(Config) ? Config->SightRadius : 1500.0f;
	const float LoseSightRadius = IsValid(Config) ? Config->LoseSightRadius : 1800.0f;
	const float PeripheralVisionAngle = IsValid(Config) ? Config->PeripheralVisionAngle : 90.0f;
	const float HearingRange = IsValid(Config) ? Config->HearingRange : 800.0f;
	const float StimulusMaxAge = IsValid(Config) ? Config->StimulusMaxAge : 5.0f;

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
	SightConfig->SetMaxAge(StimulusMaxAge);

	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(StimulusMaxAge);

	EnemyPerceptionComponent->ConfigureSense(*SightConfig);
	EnemyPerceptionComponent->ConfigureSense(*HearingConfig);
	EnemyPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void APREnemyAIController::ApplyCombatMovePresentationContext(
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig& PresentationConfig)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn))
		{
			EnemyInterface->ApplyCombatMovePresentationContext(PresentationConfig);
		}
	}

	if (PresentationConfig.bMaintainTargetFocus && IsValid(FocusTarget))
	{
		SetFocus(FocusTarget);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] Apply MoveSpeed=%.1f YawRate=%.1f Focus=%s CombatMove=%s AimOffset=%s StrafeState=%s Pawn=%s"),
			PresentationConfig.MoveSpeedOverride,
			PresentationConfig.RotationYawRate,
			PresentationConfig.bMaintainTargetFocus ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatMovePose ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatAimOffset ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatStrafeState ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::ClearCombatMovePresentationContext(bool bClearGameplayFocus)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn))
		{
			EnemyInterface->ClearCombatMovePresentationContext();
		}
	}

	if (bClearGameplayFocus)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] Clear Focus=%s Pawn=%s"),
			bClearGameplayFocus ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GetPawn()));
	}
}

EPRTacticalMode APREnemyAIController::GetBlackboardTacticalMode() const
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return EPRTacticalMode::Idle;
	}

	return static_cast<EPRTacticalMode>(CachedBlackboardComponent->GetValueAsEnum(TacticalModeKey));
}

bool APREnemyAIController::HasLastKnownTargetLocation() const
{
	return IsValid(CachedBlackboardComponent)
		&& CachedBlackboardComponent->IsVectorValueSet(LastKnownTargetLocationKey);
}

void APREnemyAIController::SetBlackboardTacticalMode(EPRTacticalMode NewMode)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NewMode));
}

const UPRCombatMoveDataAsset* APREnemyAIController::GetCurrentCombatDataAsset() const
{
	return ResolveCombatDataAssetFromPawn(GetPawn());
}

void APREnemyAIController::ApplyPresentationForTacticalMode(
	EPRTacticalMode NewMode,
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig* OverridePresentationConfig)
{
	AActor* ResolvedFocusTarget = FocusTarget;
	if (!IsValid(ResolvedFocusTarget) && IsValid(CachedBlackboardComponent))
	{
		ResolvedFocusTarget = Cast<AActor>(CachedBlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (OverridePresentationConfig != nullptr)
	{
		ApplyCombatMovePresentationContext(ResolvedFocusTarget, *OverridePresentationConfig);
		return;
	}

	const UPRCombatMoveDataAsset* CombatDataAsset = GetCurrentCombatDataAsset();
	if (!IsValid(CombatDataAsset))
	{
		ClearCombatMovePresentationContext(true);
		return;
	}

	const FPREnemyMovePresentationConfig* PresentationConfig = CombatDataAsset->FindTacticalModePresentationConfig(NewMode);
	if (PresentationConfig == nullptr)
	{
		ClearCombatMovePresentationContext(true);
		return;
	}

	ApplyCombatMovePresentationContext(ResolvedFocusTarget, *PresentationConfig);
}

void APREnemyAIController::ApplyInitialAttackPressureOnAlert()
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	const UPREnemyCombatDataAsset* CombatDataAsset = Cast<UPREnemyCombatDataAsset>(GetCurrentCombatDataAsset());
	if (!IsValid(CombatDataAsset) || AttackPressureKey == NAME_None)
	{
		return;
	}

	if (CachedBlackboardComponent->GetKeyID(AttackPressureKey) == FBlackboard::InvalidKey)
	{
		return;
	}

	const float InitialAttackPressure = FMath::Clamp(
		CombatDataAsset->AttackPressureConfig.InitialAttackPressureOnAlert,
		0.0f,
		CombatDataAsset->AttackPressureConfig.MaxPressure);

	CachedBlackboardComponent->SetValueAsFloat(AttackPressureKey, InitialAttackPressure);

	if (PREnemyAIDebug::IsAttackPressureLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[AttackPressure] AlertInitial Value=%.2f Pawn=%s"),
			InitialAttackPressure,
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset)
{
	CachedBlackboardComponent = nullptr;

	if (!IsValid(BehaviorTreeAsset))
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;
	const bool bBlackboardReady = UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComponent);
	if (!bBlackboardReady || !IsValid(BlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent = BlackboardComponent;
}

void APREnemyAIController::ClearThreatBinding()
{
	if (IsValid(CachedThreatComponent))
	{
		CachedThreatComponent->OnTargetChanged.RemoveAll(this);
		CachedThreatComponent = nullptr;
	}
}
