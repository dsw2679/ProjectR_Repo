// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Weapon Visual 컴포넌트 구현)
#include "PRFaerinWeaponVisualComponent.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinWeaponVisual, Log, All);

namespace
{
	template <typename TComponent>
	TComponent* FindOwnerComponentByName(AActor* OwnerActor, const FName ComponentName)
	{
		if (!IsValid(OwnerActor) || ComponentName == NAME_None)
		{
			return nullptr;
		}

		TArray<TComponent*> Components;
		OwnerActor->GetComponents<TComponent>(Components);
		for (TComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}

		return nullptr;
	}

	void SetSkeletalMeshVisibility(USkeletalMeshComponent* MeshComponent, const bool bVisible)
	{
		if (!IsValid(MeshComponent))
		{
			return;
		}

		MeshComponent->SetVisibility(bVisible, true);
		MeshComponent->SetHiddenInGame(!bVisible, true);
	}
}

UPRFaerinWeaponVisualComponent::UPRFaerinWeaponVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRFaerinWeaponVisualComponent::OnRegister()
{
	Super::OnRegister();
	ResolveConfiguredComponents();
}

void UPRFaerinWeaponVisualComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveConfiguredComponents();
	LogMissingConfiguredComponents();
	SetBladeHitBoxEnabled(false);
}

void UPRFaerinWeaponVisualComponent::ResolveConfiguredComponents()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (!IsValid(SwordBladeMesh))
	{
		SwordBladeMesh = FindOwnerComponentByName<USkeletalMeshComponent>(OwnerActor, SwordBladeMeshName);
	}

	if (!IsValid(SwordHandleMesh))
	{
		SwordHandleMesh = FindOwnerComponentByName<USkeletalMeshComponent>(OwnerActor, SwordHandleMeshName);
	}

	if (!IsValid(SwordShardsMesh))
	{
		SwordShardsMesh = FindOwnerComponentByName<USkeletalMeshComponent>(OwnerActor, SwordShardsMeshName);
	}

	if (!IsValid(BladeHitBox))
	{
		BladeHitBox = FindOwnerComponentByName<UBoxComponent>(OwnerActor, BladeHitBoxName);
	}
}

void UPRFaerinWeaponVisualComponent::LogMissingConfiguredComponents() const
{
	if (!bWarnIfComponentResolveFails)
	{
		return;
	}

	if (IsValid(SwordBladeMesh)
		&& IsValid(SwordHandleMesh)
		&& IsValid(SwordShardsMesh)
		&& IsValid(BladeHitBox))
	{
		return;
	}

	UE_LOG(LogPRFaerinWeaponVisual, Warning,
		TEXT("Faerin weapon visual component resolve incomplete. Owner=%s, Blade=%s(%s), Handle=%s(%s), Shards=%s(%s), HitBox=%s(%s)"),
		*GetNameSafe(GetOwner()),
		*SwordBladeMeshName.ToString(),
		*GetNameSafe(SwordBladeMesh),
		*SwordHandleMeshName.ToString(),
		*GetNameSafe(SwordHandleMesh),
		*SwordShardsMeshName.ToString(),
		*GetNameSafe(SwordShardsMesh),
		*BladeHitBoxName.ToString(),
		*GetNameSafe(BladeHitBox));
}

void UPRFaerinWeaponVisualComponent::ShowSword()
{
	ResolveConfiguredComponents();

	SetSkeletalMeshVisibility(SwordBladeMesh, true);
	SetSkeletalMeshVisibility(SwordShardsMesh, false);
	if (bKeepHandleVisible)
	{
		SetSkeletalMeshVisibility(SwordHandleMesh, true);
	}
}

void UPRFaerinWeaponVisualComponent::ShowShards()
{
	ResolveConfiguredComponents();

	SetSkeletalMeshVisibility(SwordBladeMesh, false);
	SetSkeletalMeshVisibility(SwordShardsMesh, true);
	if (bKeepHandleVisible)
	{
		SetSkeletalMeshVisibility(SwordHandleMesh, true);
	}
}

void UPRFaerinWeaponVisualComponent::SetBladeHitBoxEnabled(bool bEnabled)
{
	ResolveConfiguredComponents();
	if (!IsValid(BladeHitBox))
	{
		return;
	}

	BladeHitBox->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
