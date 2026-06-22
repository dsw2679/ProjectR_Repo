// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (적 AI Update Penitent 몬스터 전투 블랙보드 갱신용 비헤이비어 트리 서비스 구현)
#include "BTService_PRUpdatePenitentCombatBlackboard.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectR/AI/Data/UPRPenitentCombatDataAsset.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	bool HasPenitentCombatBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	APRPenitentCharacter* ResolvePenitentCharacter(const UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		return IsValid(AIController) ? Cast<APRPenitentCharacter>(AIController->GetPawn()) : nullptr;
	}

	const UPRPenitentCombatDataAsset* ResolvePenitentCombatDataAsset(const APRPenitentCharacter* PenitentCharacter)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(PenitentCharacter);
		return EnemyInterface
			? Cast<UPRPenitentCombatDataAsset>(EnemyInterface->GetCombatDataAsset())
			: nullptr;
	}

	UAbilitySystemComponent* ResolvePenitentAbilitySystem(const APRPenitentCharacter* PenitentCharacter)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(PenitentCharacter);
		return EnemyInterface ? EnemyInterface->GetEnemyAbilitySystemComponent() : nullptr;
	}

	EPRPenitentDistanceBand ResolveDistanceBand(float DistanceToTarget, const FPRPenitentCombatRangeData& RangeData)
	{
		if (DistanceToTarget <= RangeData.MeleeMaxRange)
		{
			return EPRPenitentDistanceBand::Melee;
		}

		if (DistanceToTarget < RangeData.SpacingMinRange)
		{
			return EPRPenitentDistanceBand::CloserThanSpace;
		}

		if (DistanceToTarget <= RangeData.SpacingMaxRange)
		{
			return EPRPenitentDistanceBand::Spacing;
		}

		if (DistanceToTarget < RangeData.RangedMaxRange)
		{
			return EPRPenitentDistanceBand::FartherThanSpace;
		}

		return EPRPenitentDistanceBand::OutOfRanged;
	}

	EPRPenitentCombatSpacingDirection ResolveCombatSpacingDirection(float DistanceToTarget, const FPRPenitentCombatRangeData& RangeData)
	{
		if (DistanceToTarget < RangeData.SpacingMinRange)
		{
			return EPRPenitentCombatSpacingDirection::Retreat;
		}

		if (DistanceToTarget <= RangeData.SpacingMaxRange)
		{
			return EPRPenitentCombatSpacingDirection::Hold;
		}

		return EPRPenitentCombatSpacingDirection::Approach;
	}

	bool CanPenitentDodge(const UAbilitySystemComponent* AbilitySystemComponent)
	{
		return IsValid(AbilitySystemComponent)
			&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::Cooldown_Enemy_Evade)
			&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
			&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
			&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Attacking);
	}
}

UBTService_PRUpdatePenitentCombatBlackboard::UBTService_PRUpdatePenitentCombatBlackboard()
{
	NodeName = TEXT("Update Penitent Combat Blackboard");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_PRUpdatePenitentCombatBlackboard::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	APRPenitentCharacter* PenitentCharacter = ResolvePenitentCharacter(OwnerComp);
	const UPRPenitentCombatDataAsset* CombatDataAsset = ResolvePenitentCombatDataAsset(PenitentCharacter);
	if (!IsValid(BlackboardComponent) || !IsValid(PenitentCharacter) || !IsValid(CombatDataAsset))
	{
		return;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ResolvePenitentAbilitySystem(PenitentCharacter);
	const bool bHasActiveBarrier = PenitentCharacter->HasActiveBarrier();

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, SpawnedBarrierActorKey))
	{
		BlackboardComponent->SetValueAsObject(SpawnedBarrierActorKey, PenitentCharacter->GetSpawnedBarrierActor());
	}

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, HasActiveBarrierKey))
	{
		BlackboardComponent->SetValueAsBool(HasActiveBarrierKey, bHasActiveBarrier);
	}

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, CanDodgeKey))
	{
		BlackboardComponent->SetValueAsBool(CanDodgeKey, CanPenitentDodge(AbilitySystemComponent));
	}

	AActor* CurrentTarget = HasPenitentCombatBlackboardKey(BlackboardComponent, CurrentTargetKey)
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey))
		: nullptr;
	const bool bHasTarget = IsValid(CurrentTarget);
	const float DistanceToTarget = bHasTarget
		? FVector::Dist(PenitentCharacter->GetActorLocation(), CurrentTarget->GetActorLocation())
		: 0.0f;

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, DistanceToTargetKey))
	{
		// Penitent 거리 판정 원본값 동기화
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, DistanceToTarget);
	}

	const FPRPenitentCombatRangeData& RangeData = CombatDataAsset->CombatRangeData;
	const EPRPenitentDistanceBand DistanceBand = bHasTarget && RangeData.IsValidRangeOrder()
		? ResolveDistanceBand(DistanceToTarget, RangeData)
		: EPRPenitentDistanceBand::NoTarget;
	const EPRPenitentCombatSpacingDirection SpacingDirection = bHasTarget && RangeData.IsValidRangeOrder()
		? ResolveCombatSpacingDirection(DistanceToTarget, RangeData)
		: EPRPenitentCombatSpacingDirection::None;

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, DistanceBandKey))
	{
		BlackboardComponent->SetValueAsEnum(DistanceBandKey, static_cast<uint8>(DistanceBand));
	}

	if (HasPenitentCombatBlackboardKey(BlackboardComponent, CombatSpacingDirectionKey))
	{
		BlackboardComponent->SetValueAsEnum(CombatSpacingDirectionKey, static_cast<uint8>(SpacingDirection));
	}
}

FString UBTService_PRUpdatePenitentCombatBlackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nDistanceBand: %s\nSpacingDirection: %s\nCanDodge: %s"),
		*Super::GetStaticDescription(),
		*DistanceBandKey.ToString(),
		*CombatSpacingDirectionKey.ToString(),
		*CanDodgeKey.ToString());
}
