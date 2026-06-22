// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Debug Draw 컴포넌트 구현)
#include "PRFaerinDebugDrawComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TAutoConsoleVariable<int32> CVarPRFaerinDebugRanges(
		TEXT("pr.EnemyAI.DebugFaerinRanges"),
		0,
		TEXT("Faerin pattern range debug draw."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarPRFaerinDebugPerception(
		TEXT("pr.EnemyAI.DebugFaerinPerception"),
		0,
		TEXT("Faerin perception range debug draw."),
		ECVF_Cheat);
#endif

	FString GetShortGameplayTagName(const FGameplayTag& GameplayTag)
	{
		FString TagString = GameplayTag.ToString();
		int32 LastDotIndex = INDEX_NONE;
		if (TagString.FindLastChar(TEXT('.'), LastDotIndex))
		{
			TagString.RightChopInline(LastDotIndex + 1);
		}

		return TagString;
	}
}

// ===== 초기화 =====

UPRFaerinDebugDrawComponent::UPRFaerinDebugDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

// ===== Tick =====

void UPRFaerinDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldDrawDebugRanges())
	{
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bShouldDrawPatterns = bEnablePatternRangeDebug
		&& (!bRequirePatternConsoleVariable || CVarPRFaerinDebugRanges.GetValueOnGameThread() > 0);
	const bool bShouldDrawPerception = bEnablePerceptionDebug
		&& (!bRequirePerceptionConsoleVariable || CVarPRFaerinDebugPerception.GetValueOnGameThread() > 0);

	if (bShouldDrawPatterns)
	{
		DrawPatternRanges();
	}

	if (bShouldDrawPerception)
	{
		DrawPerceptionRanges();
	}
#endif
}

// ===== 디버그 가능 여부 =====

bool UPRFaerinDebugDrawComponent::ShouldDrawDebugRanges() const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const bool bShouldDrawPatterns = bEnablePatternRangeDebug
		&& (!bRequirePatternConsoleVariable || CVarPRFaerinDebugRanges.GetValueOnGameThread() > 0);
	const bool bShouldDrawPerception = bEnablePerceptionDebug
		&& (!bRequirePerceptionConsoleVariable || CVarPRFaerinDebugPerception.GetValueOnGameThread() > 0);

	if (!bShouldDrawPatterns && !bShouldDrawPerception)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	if (World->WorldType != EWorldType::PIE && World->WorldType != EWorldType::Game)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (bDrawOnlyAuthority && (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()))
	{
		return false;
	}

	return true;
#endif
}

// ===== 패턴 범위 =====

void UPRFaerinDebugDrawComponent::DrawPatternRanges() const
{
	const AActor* OwnerActor = GetOwner();
	const UPRPatternDataAsset* PatternDataAsset = GetPatternDataAsset();
	if (!IsValid(OwnerActor) || !IsValid(PatternDataAsset))
	{
		return;
	}

	FPRPatternContext PatternContext;
	AActor* CurrentTarget = nullptr;
	if (!BuildPatternContext(PatternContext, CurrentTarget))
	{
		return;
	}

	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	const FName SelectedAbilityTagName = GetSelectedAbilityTagName(BlackboardComponent);
	const FVector Center = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, DrawHeightOffset);

	int32 LabelIndex = 0;
	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (!PatternRule.IsValid())
		{
			continue;
		}

		const FColor RuleColor = ResolvePatternRuleColor(PatternRule, PatternContext, SelectedAbilityTagName);
		if (PatternRule.MinRange > 0.0f)
		{
			DrawRangeCircle(Center, PatternRule.MinRange, RuleColor);
		}
		DrawRangeCircle(Center, PatternRule.MaxRange, RuleColor);

		if (bDrawLabels)
		{
			DrawRangeLabel(MakePatternRuleLabel(PatternRule, PatternContext), Center, PatternRule.MaxRange, RuleColor, LabelIndex);
		}

		++LabelIndex;
	}

	if (bDrawTargetLine && IsValid(CurrentTarget))
	{
		DrawDebugLine(
			GetWorld(),
			Center,
			CurrentTarget->GetActorLocation() + FVector(0.0f, 0.0f, DrawHeightOffset),
			TargetLineColor,
			false,
			DrawDuration,
			0,
			LineThickness);
	}

	DrawContextLabel(PatternContext, SelectedAbilityTagName);
}

bool UPRFaerinDebugDrawComponent::BuildPatternContext(FPRPatternContext& OutContext, AActor*& OutCurrentTarget) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	OutCurrentTarget = GetCurrentTarget();
	if (!IsValid(OutCurrentTarget) && HasBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		OutCurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	OutContext.DistanceToTarget = GetDistanceToTarget(OutCurrentTarget);
	OutContext.bHasLOS = HasBlackboardKey(BlackboardComponent, HasLOSKey)
		? BlackboardComponent->GetValueAsBool(HasLOSKey)
		: IsTargetInsideSightCone(OutCurrentTarget, OutContext.DistanceToTarget + 1.0f, 360.0f);
	OutContext.TacticalMode = HasBlackboardKey(BlackboardComponent, TacticalModeKey)
		? static_cast<EPRTacticalMode>(BlackboardComponent->GetValueAsEnum(TacticalModeKey))
		: EPRTacticalMode::Idle;
	OutContext.bChargePathClear = HasBlackboardKey(BlackboardComponent, ChargePathClearKey)
		? BlackboardComponent->GetValueAsBool(ChargePathClearKey)
		: false;
	OutContext.CurrentAttackPressure = HasBlackboardKey(BlackboardComponent, AttackPressureKey)
		? BlackboardComponent->GetValueAsFloat(AttackPressureKey)
		: 0.0f;
	OutContext.bIsBoss = true;
	OutContext.BossPhase = GetCurrentBossPhase();

	return true;
}

FColor UPRFaerinDebugDrawComponent::ResolvePatternRuleColor(
	const FPRPatternRule& PatternRule,
	const FPRPatternContext& PatternContext,
	FName SelectedAbilityTagName) const
{
	if (PatternRule.AbilityTag.GetTagName() == SelectedAbilityTagName)
	{
		return SelectedColor;
	}

	if (!IsRuleAllowedInCurrentPhase(PatternRule, PatternContext.BossPhase))
	{
		return PhaseBlockedColor;
	}

	const bool bInRange = PatternContext.DistanceToTarget >= PatternRule.MinRange
		&& PatternContext.DistanceToTarget <= PatternRule.MaxRange;
	if (!bInRange)
	{
		return RangeMismatchColor;
	}

	if (PatternRule.MatchesContext(PatternContext))
	{
		return ActiveColor;
	}

	return BlockedColor;
}

FString UPRFaerinDebugDrawComponent::MakePatternRuleLabel(const FPRPatternRule& PatternRule, const FPRPatternContext& PatternContext) const
{
	const FString ShortTagName = GetShortGameplayTagName(PatternRule.AbilityTag);
	const FString Status = PatternRule.MatchesContext(PatternContext) ? TEXT("OK") : TEXT("BLOCK");
	return FString::Printf(
		TEXT("%s %.0f-%.0f %s"),
		*ShortTagName,
		PatternRule.MinRange,
		PatternRule.MaxRange,
		*Status);
}

bool UPRFaerinDebugDrawComponent::IsRuleAllowedInCurrentPhase(const FPRPatternRule& PatternRule, EPRBossPhase CurrentPhase) const
{
	return !PatternRule.bRestrictBossPhases || PatternRule.AllowedBossPhases.Contains(CurrentPhase);
}

void UPRFaerinDebugDrawComponent::DrawContextLabel(const FPRPatternContext& PatternContext, FName SelectedAbilityTagName) const
{
	if (!bDrawContextLabel)
	{
		return;
	}

	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(World) || !IsValid(OwnerActor))
	{
		return;
	}

	const FString PhaseName = StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PatternContext.BossPhase));
	const FString TacticalModeName = StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(PatternContext.TacticalMode));
	const FString SelectedName = SelectedAbilityTagName != NAME_None
		? SelectedAbilityTagName.ToString()
		: TEXT("None");
	const FString Label = FString::Printf(
		TEXT("Faerin %s | Dist %.0f | LOS %s | Charge %s | Mode %s | Selected %s"),
		*PhaseName,
		PatternContext.DistanceToTarget,
		PatternContext.bHasLOS ? TEXT("Y") : TEXT("N"),
		PatternContext.bChargePathClear ? TEXT("Y") : TEXT("N"),
		*TacticalModeName,
		*SelectedName);

	DrawDebugString(
		World,
		OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 220.0f),
		Label,
		GetOwner(),
		SelectedColor,
		DrawDuration,
		true);
}

// ===== 감지 범위 =====

void UPRFaerinDebugDrawComponent::DrawPerceptionRanges() const
{
	const AActor* OwnerActor = GetOwner();
	const UPRPerceptionConfig* PerceptionConfig = GetPerceptionConfig();
	if (!IsValid(OwnerActor) || !IsValid(PerceptionConfig))
	{
		return;
	}

	const AActor* CurrentTarget = GetCurrentTarget();
	const float DistanceToTarget = GetDistanceToTarget(CurrentTarget);
	const FVector Center = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, DrawHeightOffset);
	FVector ForwardDirection = OwnerActor->GetActorForwardVector();
	ForwardDirection.Z = 0.0f;
	if (!ForwardDirection.Normalize())
	{
		ForwardDirection = FVector::ForwardVector;
	}

	if (PerceptionConfig->SightRadius > 0.0f)
	{
		const bool bInSight = IsTargetInsideSightCone(CurrentTarget, PerceptionConfig->SightRadius, PerceptionConfig->PeripheralVisionAngle);
		const FColor SightColor = bInSight ? PerceptionActiveColor : RangeMismatchColor;
		DrawVisionArc(Center, ForwardDirection, PerceptionConfig->SightRadius, PerceptionConfig->PeripheralVisionAngle, SightColor);

		if (bDrawLabels)
		{
			const FVector LabelLocation = Center + (ForwardDirection * PerceptionConfig->SightRadius) + FVector(0.0f, 0.0f, 48.0f);
			DrawDebugString(GetWorld(), LabelLocation, FString::Printf(TEXT("Sight %.0f / %.0f"), PerceptionConfig->SightRadius, PerceptionConfig->PeripheralVisionAngle), GetOwner(), SightColor, DrawDuration, true);
		}
	}

	if (PerceptionConfig->LoseSightRadius > 0.0f)
	{
		const bool bInLoseSight = IsTargetInsideSightCone(CurrentTarget, PerceptionConfig->LoseSightRadius, PerceptionConfig->PeripheralVisionAngle);
		const FColor LoseRangeColor = bInLoseSight ? LoseSightColor : RangeMismatchColor;
		DrawVisionArc(Center, ForwardDirection, PerceptionConfig->LoseSightRadius, PerceptionConfig->PeripheralVisionAngle, LoseRangeColor);

		if (bDrawLabels)
		{
			const FVector LabelLocation = Center + (ForwardDirection * PerceptionConfig->LoseSightRadius) + FVector(0.0f, 0.0f, 72.0f);
			DrawDebugString(GetWorld(), LabelLocation, FString::Printf(TEXT("LoseSight %.0f"), PerceptionConfig->LoseSightRadius), GetOwner(), LoseRangeColor, DrawDuration, true);
		}
	}

	if (PerceptionConfig->HearingRange > 0.0f)
	{
		const bool bInHearingRange = DistanceToTarget <= PerceptionConfig->HearingRange;
		const FColor HearingRangeColor = bInHearingRange ? HearingColor : RangeMismatchColor;
		DrawRangeCircle(Center, PerceptionConfig->HearingRange, HearingRangeColor);

		if (bDrawLabels)
		{
			DrawRangeLabel(TEXT("Hearing"), Center, PerceptionConfig->HearingRange, HearingRangeColor, 0);
		}
	}
}

void UPRFaerinDebugDrawComponent::DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f || CircleSegments < 3)
	{
		return;
	}

	FVector PreviousPoint = Center + FVector(Radius, 0.0f, 0.0f);
	for (int32 SegmentIndex = 1; SegmentIndex <= CircleSegments; ++SegmentIndex)
	{
		const float SegmentRatio = static_cast<float>(SegmentIndex) / static_cast<float>(CircleSegments);
		const float Angle = SegmentRatio * 2.0f * UE_PI;
		const FVector CurrentPoint = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

		DrawDebugLine(World, PreviousPoint, CurrentPoint, Color, false, DrawDuration, 0, LineThickness);
		PreviousPoint = CurrentPoint;
	}
}

void UPRFaerinDebugDrawComponent::DrawVisionArc(const FVector& Center, const FVector& ForwardDirection, float Radius, float PeripheralVisionAngleDegrees, const FColor& Color) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f || CircleSegments < 3)
	{
		return;
	}

	const float HalfAngleRadians = FMath::DegreesToRadians(FMath::Clamp(PeripheralVisionAngleDegrees * 0.5f, 0.0f, 180.0f));
	const float ForwardYawRadians = FMath::Atan2(ForwardDirection.Y, ForwardDirection.X);
	const float StartYawRadians = ForwardYawRadians - HalfAngleRadians;
	const float EndYawRadians = ForwardYawRadians + HalfAngleRadians;
	const int32 ArcSegments = FMath::Max(CircleSegments / 4, 8);

	FVector PreviousPoint = Center + FVector(FMath::Cos(StartYawRadians) * Radius, FMath::Sin(StartYawRadians) * Radius, 0.0f);
	DrawDebugLine(World, Center, PreviousPoint, Color, false, DrawDuration, 0, LineThickness);

	for (int32 SegmentIndex = 1; SegmentIndex <= ArcSegments; ++SegmentIndex)
	{
		const float SegmentRatio = static_cast<float>(SegmentIndex) / static_cast<float>(ArcSegments);
		const float CurrentYawRadians = FMath::Lerp(StartYawRadians, EndYawRadians, SegmentRatio);
		const FVector CurrentPoint = Center + FVector(FMath::Cos(CurrentYawRadians) * Radius, FMath::Sin(CurrentYawRadians) * Radius, 0.0f);

		DrawDebugLine(World, PreviousPoint, CurrentPoint, Color, false, DrawDuration, 0, LineThickness);
		PreviousPoint = CurrentPoint;
	}

	DrawDebugLine(World, Center, PreviousPoint, Color, false, DrawDuration, 0, LineThickness);
}

void UPRFaerinDebugDrawComponent::DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color, int32 LabelIndex) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f)
	{
		return;
	}

	const float LabelAngle = FMath::DegreesToRadians(LabelAngleStepDegrees * static_cast<float>(LabelIndex));
	const FVector LabelDirection(FMath::Cos(LabelAngle), FMath::Sin(LabelAngle), 0.0f);
	const float LabelHeight = 48.0f + (static_cast<float>(LabelIndex % 4) * 22.0f);
	const FVector LabelLocation = Center + (LabelDirection * Radius) + FVector(0.0f, 0.0f, LabelHeight);
	DrawDebugString(World, LabelLocation, Label, GetOwner(), Color, DrawDuration, true);
}

// ===== 데이터 조회 =====

AActor* UPRFaerinDebugDrawComponent::GetCurrentTarget() const
{
	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	const UPREnemyThreatComponent* ThreatComponent = EnemyInterface != nullptr
		? EnemyInterface->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetCurrentTarget()
		: nullptr;
}

const UPRPatternDataAsset* UPRFaerinDebugDrawComponent::GetPatternDataAsset() const
{
	if (IsValid(PatternDataAssetOverride))
	{
		return PatternDataAssetOverride;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	return EnemyInterface != nullptr
		? EnemyInterface->GetPatternDataAsset()
		: nullptr;
}

const UPRPerceptionConfig* UPRFaerinDebugDrawComponent::GetPerceptionConfig() const
{
	if (IsValid(PerceptionConfigOverride))
	{
		return PerceptionConfigOverride;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	return EnemyInterface != nullptr
		? EnemyInterface->GetPerceptionConfig()
		: nullptr;
}

const UBlackboardComponent* UPRFaerinDebugDrawComponent::GetBlackboardComponent() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn))
	{
		return nullptr;
	}

	const AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
	return IsValid(AIController)
		? AIController->GetBlackboardComponent()
		: nullptr;
}

bool UPRFaerinDebugDrawComponent::HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const
{
	return IsValid(BlackboardComponent)
		&& KeyName != NAME_None
		&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
}

FName UPRFaerinDebugDrawComponent::GetSelectedAbilityTagName(const UBlackboardComponent* BlackboardComponent) const
{
	return HasBlackboardKey(BlackboardComponent, SelectedAbilityTagKey)
		? BlackboardComponent->GetValueAsName(SelectedAbilityTagKey)
		: NAME_None;
}

EPRBossPhase UPRFaerinDebugDrawComponent::GetCurrentBossPhase() const
{
	const APRBossBaseCharacter* BossOwner = Cast<APRBossBaseCharacter>(GetOwner());
	return IsValid(BossOwner)
		? BossOwner->GetCurrentPhase()
		: EPRBossPhase::Phase1;
}

float UPRFaerinDebugDrawComponent::GetDistanceToTarget(const AActor* CurrentTarget) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist(OwnerActor->GetActorLocation(), CurrentTarget->GetActorLocation());
}

bool UPRFaerinDebugDrawComponent::IsTargetInsideSightCone(const AActor* CurrentTarget, float Radius, float PeripheralVisionAngleDegrees) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget) || Radius <= 0.0f)
	{
		return false;
	}

	FVector DirectionToTarget = CurrentTarget->GetActorLocation() - OwnerActor->GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	if (DirectionToTarget.IsNearlyZero())
	{
		return true;
	}

	if (DirectionToTarget.SizeSquared() > FMath::Square(Radius))
	{
		return false;
	}

	FVector ForwardDirection = OwnerActor->GetActorForwardVector();
	ForwardDirection.Z = 0.0f;
	if (!ForwardDirection.Normalize())
	{
		return false;
	}

	DirectionToTarget.Normalize();

	const float ClampedDot = FMath::Clamp(FVector::DotProduct(ForwardDirection, DirectionToTarget), -1.0f, 1.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));
	return AngleDegrees <= (PeripheralVisionAngleDegrees * 0.5f);
}
