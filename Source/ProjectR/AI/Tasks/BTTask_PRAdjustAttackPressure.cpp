// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Adjust 공격 Pressure 비헤이비어 트리 태스크 구현)
#include "BTTask_PRAdjustAttackPressure.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasAdjustPressureBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

// ===== 초기화 =====

UBTTask_PRAdjustAttackPressure::UBTTask_PRAdjustAttackPressure()
{
	NodeName = TEXT("Adjust Attack Pressure");
}

// ===== Blackboard 갱신 =====

EBTNodeResult::Type UBTTask_PRAdjustAttackPressure::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!HasAdjustPressureBlackboardKey(BlackboardComponent, AttackPressureKey))
	{
		return bFailIfKeyMissing ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;

	const float PreviousPressure = BlackboardComponent->GetValueAsFloat(AttackPressureKey);
	const float AdjustedPressure = CalculateAdjustedPressure(PreviousPressure);
	const float NextPressure = ClampAdjustedPressure(ControlledPawn, AdjustedPressure);

	BlackboardComponent->SetValueAsFloat(AttackPressureKey, NextPressure);

	if (PREnemyAIDebug::IsAttackPressureLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[AttackPressure] Adjust Mode=%s Prev=%.2f Next=%.2f Value=%.2f Pawn=%s"),
			*StaticEnum<EPRAttackPressureAdjustmentMode>()->GetNameStringByValue(static_cast<int64>(AdjustmentMode)),
			PreviousPressure,
			NextPressure,
			Value,
			*GetNameSafe(ControlledPawn));
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRAdjustAttackPressure::GetStaticDescription() const
{
	const TCHAR* MaxClampSource = TEXT("None");
	if (MaxPressureOverride > 0.0f)
	{
		MaxClampSource = TEXT("Override");
	}
	else if (bClampToEnemyCombatDataMax)
	{
		MaxClampSource = TEXT("CombatData");
	}

	return FString::Printf(
		TEXT("%s\nPressure Key: %s\nMode: %s\nValue: %.2f\nMin: %.2f\nMax Clamp: %s"),
		*Super::GetStaticDescription(),
		*AttackPressureKey.ToString(),
		*StaticEnum<EPRAttackPressureAdjustmentMode>()->GetNameStringByValue(static_cast<int64>(AdjustmentMode)),
		Value,
		MinPressure,
		MaxClampSource);
}

// ===== Pressure 계산 =====

bool UBTTask_PRAdjustAttackPressure::ResolveMaxPressure(const APawn* ControlledPawn, float& OutMaxPressure) const
{
	if (MaxPressureOverride > 0.0f)
	{
		OutMaxPressure = MaxPressureOverride;
		return true;
	}

	if (!bClampToEnemyCombatDataMax || !IsValid(ControlledPawn))
	{
		return false;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (EnemyInterface == nullptr)
	{
		return false;
	}

	const UPREnemyCombatDataAsset* CombatDataAsset = Cast<UPREnemyCombatDataAsset>(EnemyInterface->GetCombatDataAsset());
	if (!IsValid(CombatDataAsset))
	{
		return false;
	}

	OutMaxPressure = CombatDataAsset->AttackPressureConfig.MaxPressure;
	return true;
}

float UBTTask_PRAdjustAttackPressure::CalculateAdjustedPressure(const float CurrentPressure) const
{
	switch (AdjustmentMode)
	{
	case EPRAttackPressureAdjustmentMode::Set:
		return Value;

	case EPRAttackPressureAdjustmentMode::Add:
		return CurrentPressure + Value;

	case EPRAttackPressureAdjustmentMode::Subtract:
		return CurrentPressure - Value;

	default:
		return CurrentPressure;
	}
}

float UBTTask_PRAdjustAttackPressure::ClampAdjustedPressure(const APawn* ControlledPawn, const float AdjustedPressure) const
{
	float MaxPressure = 0.0f;
	if (ResolveMaxPressure(ControlledPawn, MaxPressure))
	{
		const float SafeMaxPressure = FMath::Max(MinPressure, MaxPressure);
		return FMath::Clamp(AdjustedPressure, MinPressure, SafeMaxPressure);
	}

	return FMath::Max(AdjustedPressure, MinPressure);
}
