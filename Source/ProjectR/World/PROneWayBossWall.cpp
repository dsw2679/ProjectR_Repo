// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 One Way Boss Wall 및 관련 시스템 구현)
#include "PROneWayBossWall.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Game/PRGameStateBase.h"

APROneWayBossWall::APROneWayBossWall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	SetRootComponent(BlockingCollision);
	BlockingCollision->SetBoxExtent(FVector(25.0f, 180.0f, 160.0f));
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);

	EntrySideTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntrySideTrigger"));
	EntrySideTrigger->SetupAttachment(RootComponent);
	EntrySideTrigger->SetRelativeLocation(FVector(-80.0f, 0.0f, 0.0f));
	EntrySideTrigger->SetBoxExtent(FVector(60.0f, 200.0f, 180.0f));
	EntrySideTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EntrySideTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	EntrySideTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EntrySideTrigger->SetGenerateOverlapEvents(true);

	ExitSideTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitSideTrigger"));
	ExitSideTrigger->SetupAttachment(RootComponent);
	ExitSideTrigger->SetRelativeLocation(FVector(80.0f, 0.0f, 0.0f));
	ExitSideTrigger->SetBoxExtent(FVector(60.0f, 200.0f, 180.0f));
	ExitSideTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ExitSideTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExitSideTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ExitSideTrigger->SetGenerateOverlapEvents(true);
}

void APROneWayBossWall::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(EntrySideTrigger))
	{
		EntrySideTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnEntryTriggerBeginOverlap);
	}

	if (IsValid(ExitSideTrigger))
	{
		ExitSideTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnExitTriggerEndOverlap);
	}

	if (HasAuthority())
	{
		if (APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
		{
			GameState->OnBossDefeated.AddDynamic(this, &ThisClass::HandleBossDefeated);
		}

		RefreshBarrierState();
	}
	else
	{
		ApplyBarrierState();
	}
}

void APROneWayBossWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearIgnoredPawns();

	Super::EndPlay(EndPlayReason);
}

void APROneWayBossWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APROneWayBossWall, bBarrierActive);
}

void APROneWayBossWall::HandleBossDefeated(FName DefeatedBossId)
{
	if (!HasAuthority() || !bDeactivateWhenBossDefeated || BossId.IsNone() || DefeatedBossId != BossId)
	{
		UE_LOG(LogTemp,Warning,TEXT("Boss Defeated, but invalid ID"));
		return;
	}

	UE_LOG(LogTemp,Warning,TEXT("Boss Defeated"));
	bBarrierActive = false;
	ApplyBarrierState();
}

void APROneWayBossWall::OnEntryTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bBarrierActive)
	{
		return;
	}

	SetPawnIgnoreWall(OtherActor, true);
}

void APROneWayBossWall::OnExitTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bBarrierActive)
	{
		return;
	}

	if (IsValid(EntrySideTrigger) && EntrySideTrigger->IsOverlappingActor(OtherActor))
	{
		return;
	}

	SetPawnIgnoreWall(OtherActor, false);
}

void APROneWayBossWall::OnRep_BarrierActive()
{
	ApplyBarrierState();
}

void APROneWayBossWall::RefreshBarrierState()
{
	if (!HasAuthority())
	{
		return;
	}

	bool bShouldBeActive = true;
	if (bDeactivateWhenBossDefeated && !BossId.IsNone())
	{
		if (const APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
		{
			// 저장된 보스 처치 상태 반영
			bShouldBeActive = !GameState->IsBossDefeated(BossId);
		}
	}

	bBarrierActive = bShouldBeActive;
	ApplyBarrierState();
}

void APROneWayBossWall::ApplyBarrierState()
{
	const ECollisionEnabled::Type BarrierCollisionState = bBarrierActive
		? ECollisionEnabled::QueryAndPhysics
		: ECollisionEnabled::NoCollision;
	const ECollisionEnabled::Type TriggerCollisionState = bBarrierActive
		? ECollisionEnabled::QueryOnly
		: ECollisionEnabled::NoCollision;

	if (IsValid(BlockingCollision))
	{
		BlockingCollision->SetCollisionEnabled(BarrierCollisionState);
	}

	if (IsValid(EntrySideTrigger))
	{
		EntrySideTrigger->SetCollisionEnabled(TriggerCollisionState);
		EntrySideTrigger->SetGenerateOverlapEvents(bBarrierActive);
	}

	if (IsValid(ExitSideTrigger))
	{
		ExitSideTrigger->SetCollisionEnabled(TriggerCollisionState);
		ExitSideTrigger->SetGenerateOverlapEvents(bBarrierActive);
	}

	if (!bBarrierActive)
	{
		ClearIgnoredPawns();
	}
}

void APROneWayBossWall::SetPawnIgnoreWall(AActor* Actor, bool bIgnore)
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (!IsValid(Pawn) || !IsValid(BlockingCollision))
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Pawn->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent))
		{
			// EntrySideTrigger와 ExitSideTrigger의 중첩 이벤트를 유지하기 위한 BlockingCollision 한정 이동 무시 상태 적용
			PrimitiveComponent->IgnoreComponentWhenMoving(BlockingCollision, bIgnore);
		}
	}

	const TWeakObjectPtr<AActor> PawnKey(Pawn);
	if (bIgnore)
	{
		IgnoredPawns.AddUnique(PawnKey);
	}
	else
	{
		IgnoredPawns.Remove(PawnKey);
	}
}

void APROneWayBossWall::ClearIgnoredPawns()
{
	for (int32 PawnIndex = IgnoredPawns.Num() - 1; PawnIndex >= 0; --PawnIndex)
	{
		AActor* Actor = IgnoredPawns[PawnIndex].Get();
		if (IsValid(Actor))
		{
			SetPawnIgnoreWall(Actor, false);
		}
		else
		{
			IgnoredPawns.RemoveAt(PawnIndex);
		}
	}

	IgnoredPawns.Reset();
}
