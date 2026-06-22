// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Soldier Armored Debug Draw 컴포넌트 구현)
#include "PRSoldierArmoredDebugDrawComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredGameplayTags.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TAutoConsoleVariable<int32> CVarPRSoldierArmoredDebugRanges(
		TEXT("pr.EnemyAI.DebugSoldierArmoredRanges"),
		0,
		TEXT("Soldier_Armored 전투 거리 디버그를 표시한다."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarPRSoldierArmoredDebugPerception(
		TEXT("pr.EnemyAI.DebugSoldierArmoredPerception"),
		0,
		TEXT("Soldier_Armored 감지 범위 디버그를 표시한다."),
		ECVF_Cheat);
#endif
}

// ===== 초기화 =====

UPRSoldierArmoredDebugDrawComponent::UPRSoldierArmoredDebugDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

// ===== Tick =====

void UPRSoldierArmoredDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldDrawDebugRanges())
	{
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bShouldDrawCombat = bEnableRangeDebug
		&& (!bRequireConsoleVariable || CVarPRSoldierArmoredDebugRanges.GetValueOnGameThread() > 0);
	const bool bShouldDrawPerception = bEnablePerceptionDebug
		&& (!bRequirePerceptionConsoleVariable || CVarPRSoldierArmoredDebugPerception.GetValueOnGameThread() > 0);

	if (bShouldDrawCombat)
	{
		DrawCombatRanges();
	}

	if (bShouldDrawPerception)
	{
		DrawPerceptionRanges();
	}
#endif
}

// ===== 디버그 가능 여부 =====

bool UPRSoldierArmoredDebugDrawComponent::ShouldDrawDebugRanges() const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const bool bShouldDrawCombat = bEnableRangeDebug
		&& (!bRequireConsoleVariable || CVarPRSoldierArmoredDebugRanges.GetValueOnGameThread() > 0);
	const bool bShouldDrawPerception = bEnablePerceptionDebug
		&& (!bRequirePerceptionConsoleVariable || CVarPRSoldierArmoredDebugPerception.GetValueOnGameThread() > 0);

	if (!bShouldDrawCombat && !bShouldDrawPerception)
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

// ===== 전투 범위 =====

void UPRSoldierArmoredDebugDrawComponent::DrawCombatRanges() const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	const AActor* CurrentTarget = GetCurrentTarget();
	const float DistanceToTarget = GetDistanceToTarget(CurrentTarget);
	const FVector Center = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, DrawHeightOffset);

	if (IsValid(CombatDataAsset))
	{
		DrawMaxRange(TEXT("Combo2"), Center, CombatDataAsset->HammerFlow.Combo2MaxRange, DistanceToTarget);
		DrawMaxRange(TEXT("Finisher"), Center, CombatDataAsset->HammerFlow.FinisherMaxRange, DistanceToTarget);
		DrawMaxRange(TEXT("SprintHit"), Center, CombatDataAsset->SprintHammerHitConfig.CollisionConfig.AttackRange, DistanceToTarget);
	}

	const FPRPatternRule* SprintHammerRule = FindSprintHammerRule(GetPatternDataAsset());
	if (SprintHammerRule != nullptr)
	{
		const bool bInSprintRange = DistanceToTarget >= SprintHammerRule->MinRange
			&& DistanceToTarget <= SprintHammerRule->MaxRange;
		const FColor SprintColor = bInSprintRange ? SprintActiveColor : InactiveColor;

		DrawRangeCircle(Center, SprintHammerRule->MinRange, SprintColor);
		DrawRangeCircle(Center, SprintHammerRule->MaxRange, SprintColor);

		if (bDrawLabels)
		{
			DrawRangeLabel(TEXT("SprintMin"), Center, SprintHammerRule->MinRange, SprintColor);
			DrawRangeLabel(TEXT("SprintMax"), Center, SprintHammerRule->MaxRange, SprintColor);
		}
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawPerceptionRanges() const
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
		const FColor SightColor = bInSight ? PerceptionActiveColor : InactiveColor;
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
		const FColor LoseRangeColor = bInLoseSight ? LoseSightColor : InactiveColor;
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
		const FColor HearingRangeColor = bInHearingRange ? HearingColor : InactiveColor;
		DrawRangeCircle(Center, PerceptionConfig->HearingRange, HearingRangeColor);

		if (bDrawLabels)
		{
			DrawRangeLabel(TEXT("Hearing"), Center, PerceptionConfig->HearingRange, HearingRangeColor);
		}
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawMaxRange(const FString& Label, const FVector& Center, float Radius, float DistanceToTarget) const
{
	if (Radius <= 0.0f)
	{
		return;
	}

	const bool bInRange = DistanceToTarget <= Radius;
	const FColor RangeColor = bInRange ? ActiveColor : InactiveColor;
	DrawRangeCircle(Center, Radius, RangeColor);

	if (bDrawLabels)
	{
		DrawRangeLabel(Label, Center, Radius, RangeColor);
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const
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

void UPRSoldierArmoredDebugDrawComponent::DrawVisionArc(const FVector& Center, const FVector& ForwardDirection, float Radius, float PeripheralVisionAngleDegrees, const FColor& Color) const
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

void UPRSoldierArmoredDebugDrawComponent::DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f)
	{
		return;
	}

	const FVector LabelLocation = Center + FVector(Radius, 0.0f, 48.0f);
	DrawDebugString(World, LabelLocation, FString::Printf(TEXT("%s %.0f"), *Label, Radius), GetOwner(), Color, DrawDuration, true);
}

// ===== 데이터 조회 =====

AActor* UPRSoldierArmoredDebugDrawComponent::GetCurrentTarget() const
{
	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	const UPREnemyThreatComponent* ThreatComponent = EnemyInterface != nullptr
		? EnemyInterface->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetCurrentTarget()
		: nullptr;
}

const UPRPatternDataAsset* UPRSoldierArmoredDebugDrawComponent::GetPatternDataAsset() const
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

const UPRPerceptionConfig* UPRSoldierArmoredDebugDrawComponent::GetPerceptionConfig() const
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

const FPRPatternRule* UPRSoldierArmoredDebugDrawComponent::FindSprintHammerRule(const UPRPatternDataAsset* PatternDataAsset) const
{
	if (!IsValid(PatternDataAsset))
	{
		return nullptr;
	}

	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (PatternRule.AbilityTag == PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer)
		{
			return &PatternRule;
		}
	}

	return nullptr;
}

float UPRSoldierArmoredDebugDrawComponent::GetDistanceToTarget(const AActor* CurrentTarget) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist(OwnerActor->GetActorLocation(), CurrentTarget->GetActorLocation());
}

bool UPRSoldierArmoredDebugDrawComponent::IsTargetInsideSightCone(const AActor* CurrentTarget, float Radius, float PeripheralVisionAngleDegrees) const
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
