// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Load Actor 구현)
#include "PRTestLoadActor.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

APRTestLoadActor::APRTestLoadActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);
}

/*~ AActor Interface ~*/

void APRTestLoadActor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (MulticastIntervalSeconds > 0.f)
	{
		World->GetTimerManager().SetTimer(
			MulticastTimerHandle, this, &APRTestLoadActor::FireMulticast,
			MulticastIntervalSeconds, true);
	}

	if (PropertyTickIntervalSeconds > 0.f)
	{
		World->GetTimerManager().SetTimer(
			PropertyTimerHandle, this, &APRTestLoadActor::TickReplicatedProperty,
			PropertyTickIntervalSeconds, true);
	}
}

void APRTestLoadActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MulticastTimerHandle);
		World->GetTimerManager().ClearTimer(PropertyTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APRTestLoadActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRTestLoadActor, ReplicatedHealth);
	DOREPLIFETIME(APRTestLoadActor, ReplicatedStateCounter);
}

/*~ APRTestLoadActor Interface ~*/

void APRTestLoadActor::FireMulticast()
{
	MulticastDummyEvent(NextEventId++, GetActorLocation());
}

void APRTestLoadActor::TickReplicatedProperty()
{
	ReplicatedStateCounter++;

	// 0~100 사이로 진동시켜 매번 변경이 감지되도록 처리
	ReplicatedHealth = FMath::Fmod(ReplicatedHealth + 7.5f, 100.f);
}

void APRTestLoadActor::MulticastDummyEvent_Implementation(int32 EventId, const FVector_NetQuantize& Location)
{
	// 수신만 하고 별도 처리 없음. 채널 점유 측정 용도
}
