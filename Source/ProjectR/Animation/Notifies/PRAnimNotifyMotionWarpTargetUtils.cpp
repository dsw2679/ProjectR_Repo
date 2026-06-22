// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 Motion Warp 타겟 Utils 윈도우/트리거 노티파이 구현)
#include "PRAnimNotifyMotionWarpTargetUtils.h"

#include "Components/SkeletalMeshComponent.h"
#include "MotionWarpingComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"

namespace PRAnimNotifyMotionWarpTargetUtils
{
	bool UpdateAttackTargetMotionWarp(USkeletalMeshComponent* MeshComp, bool bUseTargetLocation)
	{
		if (!IsValid(MeshComp))
		{
			return false;
		}

		AActor* OwnerActor = MeshComp->GetOwner();
		APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(OwnerActor);
		if (!IsValid(EnemyCharacter) || !EnemyCharacter->HasAuthority())
		{
			return false;
		}

		UPREnemyThreatComponent* ThreatComponent = EnemyCharacter->GetEnemyThreatComponent();
		AActor* TargetActor = IsValid(ThreatComponent) ? ThreatComponent->GetAttackTarget() : nullptr;
		if (!IsValid(TargetActor))
		{
			return false;
		}

		UMotionWarpingComponent* MotionWarpingComponent = EnemyCharacter->FindComponentByClass<UMotionWarpingComponent>();
		if (!IsValid(MotionWarpingComponent))
		{
			return false;
		}

		const FVector OwnerLocation = EnemyCharacter->GetActorLocation();
		const FVector TargetLocation = TargetActor->GetActorLocation();
		FVector DirectionToTarget = TargetLocation - OwnerLocation;
		DirectionToTarget.Z = 0.0f;
		if (DirectionToTarget.IsNearlyZero())
		{
			return false;
		}

		FRotator FacingRotation = DirectionToTarget.Rotation();
		FacingRotation.Pitch = 0.0f;
		FacingRotation.Roll = 0.0f;

		const FVector WarpLocation = bUseTargetLocation ? TargetLocation : OwnerLocation;
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("AttackTarget"), WarpLocation, FacingRotation);
		return true;
	}
}
