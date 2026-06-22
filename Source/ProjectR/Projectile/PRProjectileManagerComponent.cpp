// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (투사체 매니저 컴포넌트 구현)
#include "PRProjectileManagerComponent.h"

#include "PRProjectileBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"

UPRProjectileManagerComponent::UPRProjectileManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/*~ Lookup ~*/

UPRProjectileManagerComponent* UPRProjectileManagerComponent::FindForActor(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return nullptr;
	}

	if (UPRProjectileManagerComponent* Found = InActor->FindComponentByClass<UPRProjectileManagerComponent>())
	{
		return Found;
	}

	// Pawn이면 Controller로 탐색
	if (APawn* AsPawn = Cast<APawn>(InActor))
	{
		if (AController* Ctrl = AsPawn->GetController())
		{
			if (UPRProjectileManagerComponent* Found = Ctrl->FindComponentByClass<UPRProjectileManagerComponent>())
			{
				return Found;
			}
		}
	}

	return nullptr;
}

/*~ Prediction Tunables ~*/

float UPRProjectileManagerComponent::GetForwardPredictionTime() const
{
	APlayerController* PC = ResolveOwningController();
	if (!IsValid(PC) || !IsValid(PC->PlayerState))
	{
		return 0.f;
	}

	const float ExactPingMs = PC->PlayerState->ExactPing;
	const float EffectivePingMs = FMath::Max(0.f, ExactPingMs - PredictionLatencyReduction);
	const float HalfPingMs = 0.5f * FMath::Min(EffectivePingMs, MaxPredictionPing);
	return 0.001f * ClientBiasPct * HalfPingMs;
}

float UPRProjectileManagerComponent::GetProjectileSpawnDelay() const
{
	APlayerController* PC = ResolveOwningController();
	if (!IsValid(PC) || !IsValid(PC->PlayerState))
	{
		return 0.f;
	}

	const float ExactPingMs = PC->PlayerState->ExactPing;
	const float OverflowMs = ExactPingMs - PredictionLatencyReduction - MaxPredictionPing;
	return 0.001f * FMath::Max(0.f, OverflowMs); // ms -> seconds 단위로 변환
}

/*~ Id Allocation ~*/

uint32 UPRProjectileManagerComponent::GenerateNewProjectileId()
{
	const uint32 NewId = NextSpawnedProjectileId++;
	if (NextSpawnedProjectileId == NULL_PROJECTILE_ID)
	{
		// 0은 무효 ID이므로 건너뜀
		NextSpawnedProjectileId = 1;
	}
	return NewId;
}

/*~ Map Registration ~*/

void UPRProjectileManagerComponent::RegisterPredictedProjectile(uint32 Id, APRProjectileBase* ProjectileActor)
{
	if (Id == NULL_PROJECTILE_ID || !IsValid(ProjectileActor))
	{
		return;
	}

	FPRLinkedProjectile& Entry = SpawnedProjectilesMap.FindOrAdd(Id);
	Entry.PredictedProjectile = ProjectileActor;
}

void UPRProjectileManagerComponent::UnregisterPredictedProjectile(uint32 Id)
{
	SpawnedProjectilesMap.Remove(Id);
}

APRProjectileBase* UPRProjectileManagerComponent::FindPredictedProjectile(uint32 Id) const
{
	if (const FPRLinkedProjectile* Entry = SpawnedProjectilesMap.Find(Id))
	{
		return Entry->PredictedProjectile.Get();
	}
	return nullptr;
}

void UPRProjectileManagerComponent::NotifyAuthArrived(uint32 Id, APRProjectileBase* AuthProjectile)
{
	if (Id == NULL_PROJECTILE_ID || !IsValid(AuthProjectile))
	{
		return;
	}

	FPRLinkedProjectile& Entry = SpawnedProjectilesMap.FindOrAdd(Id);
	Entry.AuthProjectile = AuthProjectile;
}

/*~ Spawn ~*/

APRProjectileBase* UPRProjectileManagerComponent::SpawnPredictedProjectile(FPRProjectileSpawnInfo& SpawnInfo)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnInfo.ProjectileClass))
	{
		return nullptr;
	}
	
	check(!SpawnInfo.ProjectileId == NULL_PROJECTILE_ID);
	if (SpawnInfo.ProjectileId == NULL_PROJECTILE_ID)
	{
		return nullptr;
	}

	// Owner/Instigator 자동 설정 (호출자가 SpawnParameters에 미지정한 경우만)
	if (SpawnInfo.SpawnParameters.Owner == nullptr)
	{
		SpawnInfo.SpawnParameters.Owner = GetOwner();
	}
	if (SpawnInfo.SpawnParameters.Instigator == nullptr)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Pawn;
		}
		else if (AController* Ctrl = Cast<AController>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Ctrl->GetPawn();
		}
	}
	
	SpawnInfo.SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	SpawnInfo.SpawnParameters.CustomPreSpawnInitalization = [this, SpawnInfo](AActor* Actor)
	{
		if (APRProjectileBase* ProjectileBase = Cast<APRProjectileBase>(Actor))
		{
			ProjectileBase->InitializeProjectile(EPRProjectileRole::Predicted, SpawnInfo.ProjectileId);
		}
	};

	APRProjectileBase* Predicted = World->SpawnActor<APRProjectileBase>(
		SpawnInfo.ProjectileClass,
		SpawnInfo.SpawnLocation,
		SpawnInfo.SpawnRotation,
		SpawnInfo.SpawnParameters);

	if (!IsValid(Predicted))
	{
		return nullptr;
	}
	
	RegisterPredictedProjectile(SpawnInfo.ProjectileId, Predicted);
	return Predicted;
}

APRProjectileBase* UPRProjectileManagerComponent::SpawnAuthProjectile(FPRProjectileSpawnInfo& SpawnInfo)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnInfo.ProjectileClass) || SpawnInfo.ProjectileId == NULL_PROJECTILE_ID)
	{
		return nullptr;
	}

	if (SpawnInfo.SpawnParameters.Owner == nullptr)
	{
		SpawnInfo.SpawnParameters.Owner = GetOwner();
	}
	if (SpawnInfo.SpawnParameters.Instigator == nullptr)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Pawn;
		}
		else if (AController* Ctrl = Cast<AController>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Ctrl->GetPawn();
		}
	}
	
	SpawnInfo.SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnInfo.SpawnParameters.CustomPreSpawnInitalization = [this, SpawnInfo](AActor* Actor)
	{
		if (APRProjectileBase* ProjectileBase = Cast<APRProjectileBase>(Actor))
		{
			ProjectileBase->InitializeProjectile(EPRProjectileRole::Auth, SpawnInfo.ProjectileId);
		}
	};

	APRProjectileBase* Auth = World->SpawnActor<APRProjectileBase>(
		SpawnInfo.ProjectileClass,
		SpawnInfo.SpawnLocation,
		SpawnInfo.SpawnRotation,
		SpawnInfo.SpawnParameters);

	if (!IsValid(Auth))
	{
		return nullptr;
	}

	// Spawn RepMovement를 FastForward 전에 Push. SimulatedProxy는 원본 스폰 위치에서 시뮬 시작
	Auth->PushRepMovement(EPRRepMovementEvent::Spawn);
	
	NotifyAuthArrived(SpawnInfo.ProjectileId, Auth);
	return Auth;
}

void UPRProjectileManagerComponent::ClearProjectile(uint32 Id)
{
	if (SpawnedProjectilesMap.Contains(Id))
	{
		SpawnedProjectilesMap.Remove(Id);
	}
}

/*~ Internal ~*/

APlayerController* UPRProjectileManagerComponent::ResolveOwningController() const
{
	AActor* Owner = GetOwner();
	if (APlayerController* PC = Cast<APlayerController>(Owner))
	{
		return PC;
	}
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}
	return nullptr;
}

