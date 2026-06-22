// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterBoundaryActor.h"

#include "Components/BoxComponent.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Character/PRPlayerCharacter.h"

APRFaerinEncounterBoundaryActor::APRFaerinEncounterBoundaryActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	ArenaVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ArenaVolume"));
	ArenaVolume->SetupAttachment(SceneRoot);
	ArenaVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArenaVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArenaVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ArenaVolume->SetGenerateOverlapEvents(true);
}

void APRFaerinEncounterBoundaryActor::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(ArenaVolume))
	{
		return;
	}

	ArenaVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleArenaBeginOverlap);
	ArenaVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleArenaEndOverlap);

	if (!HasAuthority())
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	ArenaVolume->GetOverlappingActors(OverlappingActors, APRPlayerCharacter::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		RegisterArenaEnter(ResolvePlayerCharacter(OverlappingActor));
	}
}

void APRFaerinEncounterBoundaryActor::RegisterArenaEnter(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !IsValid(Player))
	{
		return;
	}

	CleanupInvalidPlayers();
	ArenaInsidePlayers.Add(Player);

	if (BoundaryMode == EFaerinBoundaryMode::Combat && IsValid(EncounterDirector))
	{
		EncounterDirector->NotifyPlayerEnteredCombatArena(Player);
	}
}

void APRFaerinEncounterBoundaryActor::RegisterArenaExit(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !IsValid(Player))
	{
		return;
	}

	CleanupInvalidPlayers();
	RemovePlayer(ArenaInsidePlayers, Player);

	// 진행 중 Gather 이탈 시 해당 플레이어의 자막/시퀀스/입력/카메라를 Director가 정리한다.
	if (IsValid(EncounterDirector))
	{
		EncounterDirector->NotifyPlayerExitedGather(Player);
	}
}

void APRFaerinEncounterBoundaryActor::GetArenaInsidePlayers(TArray<APRPlayerCharacter*>& OutPlayers) const
{
	OutPlayers.Reset();

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : ArenaInsidePlayers)
	{
		if (APRPlayerCharacter* Player = WeakPlayer.Get())
		{
			OutPlayers.Add(Player);
		}
	}
}

void APRFaerinEncounterBoundaryActor::ClearEncounterState()
{
	if (!HasAuthority())
	{
		return;
	}

	ArenaInsidePlayers.Reset();
	BoundaryMode = EFaerinBoundaryMode::PreIntro;
}

void APRFaerinEncounterBoundaryActor::SetBoundaryMode(EFaerinBoundaryMode Mode)
{
	if (!HasAuthority())
	{
		return;
	}

	BoundaryMode = Mode;
	CleanupInvalidPlayers();
}

bool APRFaerinEncounterBoundaryActor::IsPlayerInsideArena(APRPlayerCharacter* Player) const
{
	return ContainsPlayer(ArenaInsidePlayers, Player);
}

bool APRFaerinEncounterBoundaryActor::IsPlayerPhysicallyInsideArena(APRPlayerCharacter* Player) const
{
	if (!IsValid(Player) || !IsValid(ArenaVolume))
	{
		return false;
	}

	const FTransform VolumeTransform = ArenaVolume->GetComponentTransform();
	const FVector LocalPoint = VolumeTransform.InverseTransformPosition(Player->GetActorLocation());
	const FVector Extent = ArenaVolume->GetScaledBoxExtent();

	return FMath::Abs(LocalPoint.X) <= Extent.X
		&& FMath::Abs(LocalPoint.Y) <= Extent.Y
		&& FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

void APRFaerinEncounterBoundaryActor::HandleArenaBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	RegisterArenaEnter(ResolvePlayerCharacter(OtherActor));
}

void APRFaerinEncounterBoundaryActor::HandleArenaEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	RegisterArenaExit(ResolvePlayerCharacter(OtherActor));
}

APRPlayerCharacter* APRFaerinEncounterBoundaryActor::ResolvePlayerCharacter(AActor* Actor) const
{
	return Cast<APRPlayerCharacter>(Actor);
}

bool APRFaerinEncounterBoundaryActor::ContainsPlayer(
	const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return false;
	}

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : Players)
	{
		if (WeakPlayer.Get() == Player)
		{
			return true;
		}
	}

	return false;
}

void APRFaerinEncounterBoundaryActor::RemovePlayer(
	TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	for (auto It = Players.CreateIterator(); It; ++It)
	{
		if (It->Get() == Player)
		{
			It.RemoveCurrent();
			return;
		}
	}
}

void APRFaerinEncounterBoundaryActor::CleanupInvalidPlayers()
{
	for (auto It = ArenaInsidePlayers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}
