// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Triple Arrow 공격 패턴 어빌리티 구현)
#include "PRGameplayAbility_FaeRoyalArcherTripleArrow.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "ProjectR/PRGameplayTags.h"

// ===== 초기화 =====

UPRGameplayAbility_FaeRoyalArcherTripleArrow::UPRGameplayAbility_FaeRoyalArcherTripleArrow()
{
	FGameplayTagContainer TripleArrowTags;
	TripleArrowTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	TripleArrowTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_Pattern);
	TripleArrowTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_TripleArrow);
	SetAssetTags(TripleArrowTags);

	AbilityTag = PRGameplayTags::Ability_Enemy_RoyalArcher_TripleArrow;
	TripleArrowSocketNames = {
		TEXT("ProjectileSocket_01"),
		TEXT("ProjectileSocket_02"),
		TEXT("ProjectileSocket_03")
	};
	TripleArrowFallbackLocalOffsets = {
		FVector(0.0f, 0.0f, 0.0f),
		FVector(0.0f, -18.0f, 6.0f),
		FVector(0.0f, 18.0f, -6.0f)
	};
	TripleArrowAimRotationOffsets = {
		FRotator(0.0f, 0.0f, 0.0f),
		FRotator(0.0f, -6.0f, 0.0f),
		FRotator(0.0f, 6.0f, 0.0f)
	};

	DamageMultiplier = 0.75f;
	PoiseDamage = 8.0f;
	WindupTime = 0.55f;
	RecoveryTime = 0.75f;
}

// ===== UPRGameplayAbility_EnemyProjectileAttack Interface =====

int32 UPRGameplayAbility_FaeRoyalArcherTripleArrow::GetProjectileFireCount() const
{
	return 3;
}

FTransform UPRGameplayAbility_FaeRoyalArcherTripleArrow::GetProjectileSpawnTransformForIndex(int32 ProjectileIndex) const
{
	if (TripleArrowSocketNames.IsValidIndex(ProjectileIndex))
	{
		FTransform SocketTransform;
		if (TryResolveTripleArrowSocketTransform(TripleArrowSocketNames[ProjectileIndex], SocketTransform))
		{
			return SocketTransform;
		}
	}

	FTransform SpawnTransform = Super::GetProjectileSpawnTransformForIndex(ProjectileIndex);
	if (TripleArrowFallbackLocalOffsets.IsValidIndex(ProjectileIndex))
	{
		const FVector OffsetLocation = SpawnTransform.TransformPositionNoScale(TripleArrowFallbackLocalOffsets[ProjectileIndex]);
		SpawnTransform.SetLocation(OffsetLocation);
	}

	return SpawnTransform;
}

FVector UPRGameplayAbility_FaeRoyalArcherTripleArrow::CalculateProjectileAimDirectionForIndex(const FVector& SpawnLocation, int32 ProjectileIndex) const
{
	const FVector BaseAimDirection = Super::CalculateProjectileAimDirectionForIndex(SpawnLocation, ProjectileIndex);
	if (!TripleArrowAimRotationOffsets.IsValidIndex(ProjectileIndex))
	{
		return BaseAimDirection;
	}

	const FRotator AimRotation = BaseAimDirection.Rotation() + TripleArrowAimRotationOffsets[ProjectileIndex];
	const FVector AimDirection = AimRotation.Vector().GetSafeNormal();
	return AimDirection.IsNearlyZero() ? BaseAimDirection : AimDirection;
}

// ===== TripleArrow Socket =====

bool UPRGameplayAbility_FaeRoyalArcherTripleArrow::TryResolveTripleArrowSocketTransform(FName SocketName, FTransform& OutTransform) const
{
	if (SocketName == NAME_None)
	{
		return false;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor);
	const USkeletalMeshComponent* MeshComponent = IsValid(SourceCharacter)
		? SourceCharacter->GetMesh()
		: nullptr;

	if (IsValid(MeshComponent) && MeshComponent->DoesSocketExist(SocketName))
	{
		OutTransform = MeshComponent->GetSocketTransform(SocketName);
		return true;
	}

	if (!IsValid(AvatarActor))
	{
		return false;
	}

	TArray<USceneComponent*> SceneComponents;
	AvatarActor->GetComponents(SceneComponents);
	for (const USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) && SceneComponent->GetFName() == SocketName)
		{
			OutTransform = SceneComponent->GetComponentTransform();
			return true;
		}
	}

	return false;
}
