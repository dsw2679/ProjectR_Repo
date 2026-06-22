// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Boss Spawner 및 관련 시스템 구현)
#include "PRBossSpawner.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"

APRBossSpawner::APRBossSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Capsule = CreateDefaultSubobject<UCapsuleComponent>("Capsule");
	SetRootComponent(Capsule);
	
	Arrow = CreateDefaultSubobject<UArrowComponent>("Arrow");
	Arrow->SetupAttachment(Capsule);
	
	SetActorEnableCollision(false);
}

APRBossBaseCharacter* APRBossSpawner::SpawnBossCharacter()
{
	if (!HasAuthority())
	{
		return nullptr;
	}
	
	if (!IsValid(BossCharacterClass))
	{
		return nullptr;
	}
	
	const APRBossBaseCharacter* BossCDO = BossCharacterClass->GetDefaultObject<APRBossBaseCharacter>();
	if (!IsValid(BossCDO))
	{
		return nullptr;
	}

	if (SpawnedCharacter.IsValid())
	{
		SpawnedCharacter->Destroy();
		SpawnedCharacter = nullptr;
	}
	
	const FName BossId = BossCDO->GetMonsterId();
	if (APRGameStateBase* GS = GetWorld()->GetGameState<APRGameStateBase>())
	{
		// 이미 처치된 상태면 스폰 x
		if (GS->IsBossDefeated(BossId))
		{
			return nullptr;
		}
	}
	
	FActorSpawnParameters BossSpawnParameters;
	BossSpawnParameters.Owner = this;
	BossSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FTransform SpawnTransform = GetActorTransform();
	SpawnedCharacter = GetWorld()->SpawnActor<APRBossBaseCharacter>(BossCharacterClass, SpawnTransform, BossSpawnParameters);
	
	return SpawnedCharacter.Get();
}

AActor* APRBossSpawner::SpawnBossForEncounter_Implementation()
{
	return SpawnBossCharacter();
}

void APRBossSpawner::ResetBossForEncounter_Implementation(AActor* SpawnedBoss)
{
	AActor* ActorToDestroy = IsValid(SpawnedBoss) ? SpawnedBoss : SpawnedCharacter.Get();
	if (IsValid(ActorToDestroy))
	{
		ActorToDestroy->Destroy();
	}

	if (!SpawnedCharacter.IsValid() || SpawnedCharacter.Get() == ActorToDestroy)
	{
		SpawnedCharacter = nullptr;
	}
}

void APRBossSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorHiddenInGame(true);
	
	if (HasAuthority())
	{
		if (bListenForGlobalBossSpawnEvent)
		{
			if (UPREventManagerSubsystem* EventMgr = GetWorld()->GetSubsystem<UPREventManagerSubsystem>())
			{
				// 서버 월드의 보스 스폰 요청 수신
				BossSpawnEventHandle = EventMgr->Listen(
					PRGameplayTags::Event_Boss_Spawn,
					FPREventMulticast::FDelegate::CreateUObject(this, &APRBossSpawner::HandleBossSpawnEvent));
			}
		}
		
		if (bAutoSpawn)
		{
			SpawnBossCharacter();	
		}
	}
}

void APRBossSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			// 스포너 소멸 시 EventManager 구독 정리
			EventMgr->UnlistenAll(BossSpawnEventHandle);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APRBossSpawner::HandleBossSpawnEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	// BroadcastEmpty 기반 보스 스폰 요청 처리
	SpawnBossCharacter();
}
