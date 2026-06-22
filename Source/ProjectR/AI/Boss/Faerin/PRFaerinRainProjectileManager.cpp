// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 Rain Portal 경량 투사체(Niagara 배열 렌더 매니저) 구현)
#include "PRFaerinRainProjectileManager.h"

#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	// 1이면 경량 rain 투사체 시뮬 위치를 디버그 스피어로 그리고, 매니저 상태를 화면에 표시한다. (Niagara와 무관하게 시뮬 확인용)
	TAutoConsoleVariable<int32> CVarFaerinRainDebugDraw(
		TEXT("pr.Faerin.RainPortal.DebugDraw"),
		0,
		TEXT("1이면 경량 rain 투사체 시뮬 위치/매니저 상태를 디버그로 표시한다."),
		ECVF_Default);
}

APRFaerinRainProjectileManager::APRFaerinRainProjectileManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// 다수 클라이언트가 멀티캐스트 스폰/제거 RPC를 받을 수 있도록 항상 관련 처리한다.
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	ProjectileNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileNiagara"));
	SetRootComponent(ProjectileNiagara);
	ProjectileNiagara->SetAutoActivate(true);
	// 위치는 월드 좌표 배열로 직접 push하므로 컴포넌트 이동/충돌은 사용하지 않는다.
	ProjectileNiagara->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APRFaerinRainProjectileManager::BeginPlay()
{
	Super::BeginPlay();

	// per-projectile 방식에서는 투사체마다 Niagara를 따로 스폰하므로(AddProjectileLocal),
	// 루트 컴포넌트에 시스템을 틀지 않는다. (틀면 매니저 원점에 잉여 이펙트가 하나 더 떠버린다)
}

void APRFaerinRainProjectileManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ProjectileNiagara))
	{
		ProjectileNiagara->Deactivate();
	}

	// 매니저가 rain 도중 파괴될 때 떠 있는 투사체별 Niagara를 정리한다.
	for (FPRRainProjectileSim& Projectile : Projectiles)
	{
		if (IsValid(Projectile.SpawnedNiagara))
		{
			Projectile.SpawnedNiagara->DeactivateImmediate();
		}
	}

	Projectiles.Reset();
	ServerDamageById.Reset();
	PendingSpawnCommands.Reset();
	PendingRemoveIds.Reset();
	Super::EndPlay(EndPlayReason);
}

void APRFaerinRainProjectileManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SimulateProjectiles(DeltaSeconds);

	if (HasAuthority())
	{
		ServerCollisionAndDamage();
	}

	CompactDeadProjectiles();
	SyncNiagaraRender();

	if (HasAuthority())
	{
		FlushPendingNetCommands();
		UpdateAutoShutdown(DeltaSeconds);
	}

#if ENABLE_DRAW_DEBUG
	// 진단: Niagara 렌더와 무관하게 시뮬이 실제로 도는지 눈으로 확인한다.
	if (CVarFaerinRainDebugDraw.GetValueOnGameThread() != 0)
	{
		if (UWorld* DebugWorld = GetWorld())
		{
			for (const FPRRainProjectileSim& Projectile : Projectiles)
			{
				DrawDebugSphere(DebugWorld, Projectile.Position, HitRadius, 8, FColor::Yellow, false, 0.0f, 0, 1.0f);
			}

			if (GEngine != nullptr)
			{
				GEngine->AddOnScreenDebugMessage(
					static_cast<uint64>(reinterpret_cast<UPTRINT>(this)),
					0.0f,
					FColor::Cyan,
					FString::Printf(TEXT("[RainMgr %s] Projectiles=%d HasAuthority=%d NiagaraSystem=%s NiagaraActive=%d"),
						*GetName(),
						Projectiles.Num(),
						HasAuthority() ? 1 : 0,
						IsValid(ProjectileNiagaraSystem) ? TEXT("SET") : TEXT("NULL"),
						(IsValid(ProjectileNiagara) && ProjectileNiagara->IsActive()) ? 1 : 0));
			}
		}
	}
#endif
}

void APRFaerinRainProjectileManager::ServerEmitRainProjectile(
	const FVector& SpawnLocation,
	const FVector& InitialVelocity,
	float Lifetime,
	const FGameplayEffectSpecHandle& DamageSpec,
	AActor* DamageInstigator)
{
	if (!HasAuthority())
	{
		return;
	}

	const uint32 ProjectileId = NextProjectileId++;
	if (NextProjectileId == 0)
	{
		NextProjectileId = 1;
	}

	FPRRainSpawnCommand SpawnCommand;
	SpawnCommand.ProjectileId = ProjectileId;
	SpawnCommand.Position = SpawnLocation;
	SpawnCommand.Velocity = InitialVelocity;
	SpawnCommand.Lifetime = FMath::Max(Lifetime, 0.1f);
	PendingSpawnCommands.Add(SpawnCommand);

	// 서버 충돌 판정에서 사용할 피해 데이터는 복제하지 않고 서버 로컬 맵에만 보관한다.
	FServerProjectileDamage& DamageData = ServerDamageById.Add(ProjectileId);
	DamageData.DamageSpec = DamageSpec;
	DamageData.Instigator = DamageInstigator;

	bHasEverEmitted = true;
	IdleSeconds = 0.0f;
}

void APRFaerinRainProjectileManager::SimulateProjectiles(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	for (FPRRainProjectileSim& Projectile : Projectiles)
	{
		if (!Projectile.bAlive)
		{
			continue;
		}

		Projectile.Velocity.Z += GravityZ * DeltaSeconds;
		Projectile.Position += Projectile.Velocity * DeltaSeconds;
		Projectile.Age += DeltaSeconds;
		
		if (IsValid(Projectile.SpawnedNiagara))
		{
			Projectile.SpawnedNiagara->SetWorldLocation(Projectile.Position);	
		}

		if (Projectile.Age >= Projectile.Lifetime)
		{
			Projectile.bAlive = false;
			if (IsValid(Projectile.SpawnedNiagara))
			{
				Projectile.SpawnedNiagara->DeactivateImmediate();
			}
			continue;
		}

		if (bUseKillBelowZ && Projectile.Position.Z < KillBelowZ)
		{
			Projectile.bAlive = false;
			if (IsValid(Projectile.SpawnedNiagara))
			{
				Projectile.SpawnedNiagara->DeactivateImmediate();
			}
		}
	}
}

void APRFaerinRainProjectileManager::ServerCollisionAndDamage()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Projectiles.Num() == 0)
	{
		return;
	}

	// 살아있는 피격 가능 플레이어를 한 번만 수집한다. (보통 소수)
	TArray<TPair<AActor*, UAbilitySystemComponent*>> Targets;
	for (TActorIterator<APRPlayerCharacter> It(World); It; ++It)
	{
		APRPlayerCharacter* Player = *It;
		if (!IsDamageablePlayer(Player))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UPRCombatStatics::FindAbilitySystemComponent(Player);
		if (IsValid(TargetASC))
		{
			Targets.Emplace(Player, TargetASC);
		}
	}

	if (Targets.Num() == 0)
	{
		return;
	}

	const float HitRadiusSquared = HitRadius * HitRadius;
	for (FPRRainProjectileSim& Projectile : Projectiles)
	{
		if (!Projectile.bAlive)
		{
			continue;
		}

		for (const TPair<AActor*, UAbilitySystemComponent*>& Target : Targets)
		{
			AActor* TargetActor = Target.Key;
			if (!IsValid(TargetActor))
			{
				continue;
			}

			const float DistSquared = FVector::DistSquared(Projectile.Position, TargetActor->GetActorLocation());
			if (DistSquared > HitRadiusSquared)
			{
				continue;
			}

			// 기존 투사체와 동일하게 GE Spec을 대상 ASC에 직접 적용한다.
			if (const FServerProjectileDamage* DamageData = ServerDamageById.Find(Projectile.ProjectileId))
			{
				FGameplayEffectSpec* EffectSpec = DamageData->DamageSpec.Data.Get();
				if (EffectSpec != nullptr)
				{
					Target.Value->ApplyGameplayEffectSpecToSelf(*EffectSpec);
				}
			}

			Projectile.bAlive = false;
			PendingRemoveIds.Add(Projectile.ProjectileId);
			break;
		}
	}
}

void APRFaerinRainProjectileManager::CompactDeadProjectiles()
{
	for (int32 Index = Projectiles.Num() - 1; Index >= 0; --Index)
	{
		if (!Projectiles[Index].bAlive)
		{
			if (HasAuthority())
			{
				ServerDamageById.Remove(Projectiles[Index].ProjectileId);
			}
			Projectiles.RemoveAtSwap(Index);
		}
	}
}

void APRFaerinRainProjectileManager::SyncNiagaraRender()
{
	// per-projectile 방식에서는 투사체마다 개별 Niagara를 스폰/이동하므로(AddProjectileLocal/SimulateProjectiles),
	// 단일 배열 push 렌더는 사용하지 않는다. (배열 방식으로 되돌릴 경우 아래 push를 복구)
}

void APRFaerinRainProjectileManager::FlushPendingNetCommands()
{
	if (PendingSpawnCommands.Num() > 0)
	{
		Multicast_AddRainProjectiles(PendingSpawnCommands);
		PendingSpawnCommands.Reset();
	}

	if (PendingRemoveIds.Num() > 0)
	{
		Multicast_RemoveRainProjectiles(PendingRemoveIds);
		PendingRemoveIds.Reset();
	}
}

void APRFaerinRainProjectileManager::UpdateAutoShutdown(float DeltaSeconds)
{
	const bool bEmpty = Projectiles.Num() == 0 && PendingSpawnCommands.Num() == 0;
	if (!bEmpty)
	{
		IdleSeconds = 0.0f;
		return;
	}

	IdleSeconds += DeltaSeconds;
	const float Threshold = bHasEverEmitted ? EmptyShutdownGrace : InitialIdleTimeout;
	if (IdleSeconds >= Threshold)
	{
		Destroy();
	}
}

void APRFaerinRainProjectileManager::Multicast_AddRainProjectiles_Implementation(const TArray<FPRRainSpawnCommand>& SpawnCommands)
{
	for (const FPRRainSpawnCommand& Command : SpawnCommands)
	{
		AddProjectileLocal(Command);
	}
}

void APRFaerinRainProjectileManager::Multicast_RemoveRainProjectiles_Implementation(const TArray<uint32>& ProjectileIds)
{
	RemoveProjectilesByIds(ProjectileIds);
}

void APRFaerinRainProjectileManager::AddProjectileLocal(const FPRRainSpawnCommand& Command)
{
	FPRRainProjectileSim& Projectile = Projectiles.AddDefaulted_GetRef();
	Projectile.ProjectileId = Command.ProjectileId;
	Projectile.Position = Command.Position;
	Projectile.Velocity = Command.Velocity;
	Projectile.Age = 0.0f;
	Projectile.Lifetime = FMath::Max(Command.Lifetime, 0.1f);
	Projectile.bAlive = true;
	
	if (IsValid(ProjectileNiagaraSystem))
	{
		// bAutoDestroy=false: 수명/이동을 매니저가 직접 관리한다. (true면 한 발짜리 이펙트가 버스트 종료 시 자가소멸해 낙하 도중 사라질 수 있음)
		// 풀(AutoRelease)이므로 DeactivateImmediate 시 자동으로 풀에 반환된다.
		Projectile.SpawnedNiagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ProjectileNiagaraSystem, Projectile.Position,
			FRotator::ZeroRotator, FVector(1), false, true, ENCPoolMethod::AutoRelease);
	}
}

void APRFaerinRainProjectileManager::RemoveProjectilesByIds(const TArray<uint32>& ProjectileIds)
{
	if (ProjectileIds.Num() == 0 || Projectiles.Num() == 0)
	{
		return;
	}

	for (FPRRainProjectileSim& Projectile : Projectiles)
	{
		if (Projectile.bAlive && ProjectileIds.Contains(Projectile.ProjectileId))
		{
			Projectile.bAlive = false;
			if (IsValid(Projectile.SpawnedNiagara))
			{
				Projectile.SpawnedNiagara->DeactivateImmediate();
			}
		}
	}
	// 실제 배열 정리는 다음 Tick의 CompactDeadProjectiles에서 일괄 수행한다.
}

bool APRFaerinRainProjectileManager::IsDamageablePlayer(const AActor* PlayerActor) const
{
	if (!IsValid(PlayerActor))
	{
		return false;
	}

	if (UPRCombatStatics::GetActorTeam(PlayerActor) != EPRTeam::Player)
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = UPRCombatStatics::FindAbilitySystemComponent(PlayerActor);
	if (!IsValid(TargetASC))
	{
		return false;
	}

	return !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}
