// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Fire Game Mode 구현)
#include "PRTestFireGameMode.h"

#include "PRTestFireShooter.h"
#include "PRTestLoadActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

APRTestFireGameMode::APRTestFireGameMode()
{
	ShooterClass = APRTestFireShooter::StaticClass();
	LoadActorClass = APRTestLoadActor::StaticClass();
}

/*~ AGameModeBase Interface ~*/

void APRTestFireGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const double ServerTime = FPlatformTime::Seconds();
	const float ExpectedRecvPerSec = ShooterFireRateRPM > 0.f ? (ShooterFireRateRPM / 60.f) : 0.f;
	UE_LOG(LogPRTestFire, Log,
		TEXT("[Config] ServerTime=%.3f FireRateRPM=%.1f ExpectedRecvPerSec=%.3f AutoStart=%s Reliable=%s TestTime=%.1f ReportInterval=1.0"),
		ServerTime,
		ShooterFireRateRPM,
		ExpectedRecvPerSec,
		bShooterAutoStart ? TEXT("true") : TEXT("false"),
		bShooterUseReliable ? TEXT("true") : TEXT("false"),
		TestTime);

	SpawnLoadActors();
}

void APRTestFireGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer) || !IsValid(ShooterClass))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	const FTransform SpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	APRTestFireShooter* Shooter = World->SpawnActorDeferred<APRTestFireShooter>(
		ShooterClass, SpawnTransform, NewPlayer, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (IsValid(Shooter))
	{
		Shooter->SetFireTestConfig(ShooterFireRateRPM, bShooterAutoStart, bShooterUseReliable, TestTime);
		Shooter->SetOwner(NewPlayer);
		Shooter->FinishSpawning(SpawnTransform);
	}
}

/*~ APRTestFireGameMode Interface ~*/

void APRTestFireGameMode::SpawnLoadActors()
{
	if (!IsValid(LoadActorClass) || LoadActorCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < LoadActorCount; ++Index)
	{
		APRTestLoadActor* LoadActor = World->SpawnActor<APRTestLoadActor>(
			LoadActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

		if (IsValid(LoadActor))
		{
			LoadActor->MulticastIntervalSeconds = LoadMulticastIntervalSeconds;
			LoadActor->PropertyTickIntervalSeconds = LoadPropertyTickIntervalSeconds;
		}
	}
}
