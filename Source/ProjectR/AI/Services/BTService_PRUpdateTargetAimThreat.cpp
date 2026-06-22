// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Update 타겟 조준 위협도 갱신용 비헤이비어 트리 서비스 구현)
#include "BTService_PRUpdateTargetAimThreat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	bool HasAimThreatBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

// ===== 초기화 =====

UBTService_PRUpdateTargetAimThreat::UBTService_PRUpdateTargetAimThreat()
{
	NodeName = TEXT("Update Target Aim Threat");
	Interval = 0.1f;
	RandomDeviation = 0.02f;
}

// ===== Blackboard 갱신 =====

void UBTService_PRUpdateTargetAimThreat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!HasAimThreatBlackboardKey(BlackboardComponent, TargetAimingAtSelfKey))
	{
		return;
	}

	BlackboardComponent->SetValueAsBool(TargetAimingAtSelfKey, false);

	if (!HasAimThreatBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	AActor* SelfActor = ResolveSelfActor(OwnerComp, BlackboardComponent);
	if (!IsValid(TargetActor) || !IsValid(SelfActor))
	{
		return;
	}

	BlackboardComponent->SetValueAsBool(TargetAimingAtSelfKey, IsTargetAimingAtSelf(TargetActor, SelfActor));
}

FString UBTService_PRUpdateTargetAimThreat::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nTarget Key: %s\nOutput Key: %s\nCone: %.1f\nMax Distance: %.1f\nRequire Aiming State: %s\nRequire LOS: %s"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*TargetAimingAtSelfKey.ToString(),
		AimConeHalfAngleDegrees,
		MaxAimThreatDistance,
		bRequireTargetAimingState ? TEXT("true") : TEXT("false"),
		bRequireLineOfSight ? TEXT("true") : TEXT("false"));
}

// ===== 조준 위협 판정 =====

AActor* UBTService_PRUpdateTargetAimThreat::ResolveSelfActor(
	const UBehaviorTreeComponent& OwnerComp,
	const UBlackboardComponent* BlackboardComponent) const
{
	if (HasAimThreatBlackboardKey(BlackboardComponent, SelfActorKey))
	{
		if (AActor* BlackboardSelfActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(SelfActorKey)))
		{
			return BlackboardSelfActor;
		}
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	return IsValid(AIController) ? AIController->GetPawn() : nullptr;
}

bool UBTService_PRUpdateTargetAimThreat::IsTargetAimingAtSelf(AActor* TargetActor, AActor* SelfActor) const
{
	if (!IsValid(TargetActor) || !IsValid(SelfActor))
	{
		return false;
	}

	if (!HasRequiredAimingState(TargetActor))
	{
		return false;
	}

	FVector TargetViewLocation = FVector::ZeroVector;
	FVector SelfAimLocation = FVector::ZeroVector;
	if (!PassesAimCone(TargetActor, SelfActor, TargetViewLocation, SelfAimLocation))
	{
		return false;
	}

	return PassesVisibilityTrace(TargetActor, SelfActor, TargetViewLocation, SelfAimLocation);
}

bool UBTService_PRUpdateTargetAimThreat::HasRequiredAimingState(AActor* TargetActor) const
{
	if (!bRequireTargetAimingState)
	{
		return true;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	return IsValid(TargetAbilitySystem)
		&& TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Aiming);
}

bool UBTService_PRUpdateTargetAimThreat::PassesAimCone(
	AActor* TargetActor,
	AActor* SelfActor,
	FVector& OutTargetViewLocation,
	FVector& OutSelfAimLocation) const
{
	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (!IsValid(TargetPawn) || !IsValid(SelfActor))
	{
		return false;
	}

	OutTargetViewLocation = TargetPawn->GetPawnViewLocation();
	OutSelfAimLocation = SelfActor->GetActorLocation() + FVector(0.0f, 0.0f, SelfAimTargetHeightOffset);

	const FVector ToSelf = OutSelfAimLocation - OutTargetViewLocation;
	if (MaxAimThreatDistance > 0.0f && ToSelf.SizeSquared() > FMath::Square(MaxAimThreatDistance))
	{
		return false;
	}

	const FVector DirectionToSelf = ToSelf.GetSafeNormal();
	if (DirectionToSelf.IsNearlyZero())
	{
		return false;
	}

	const FVector AimDirection = TargetPawn->GetBaseAimRotation().Vector().GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		return false;
	}

	const float DotThreshold = FMath::Cos(FMath::DegreesToRadians(AimConeHalfAngleDegrees));
	return FVector::DotProduct(AimDirection, DirectionToSelf) >= DotThreshold;
}

bool UBTService_PRUpdateTargetAimThreat::PassesVisibilityTrace(
	AActor* TargetActor,
	AActor* SelfActor,
	const FVector& TargetViewLocation,
	const FVector& SelfAimLocation) const
{
	if (!bRequireLineOfSight)
	{
		return true;
	}

	if (!IsValid(TargetActor) || !IsValid(SelfActor))
	{
		return false;
	}

	const UWorld* World = TargetActor->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRTargetAimThreatTrace), false, TargetActor);
	QueryParams.AddIgnoredActor(TargetActor);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TargetViewLocation,
		SelfAimLocation,
		TraceChannel,
		QueryParams);

	return !bHit || HitResult.GetActor() == SelfActor;
}
