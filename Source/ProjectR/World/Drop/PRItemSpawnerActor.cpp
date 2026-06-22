// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Item Spawner Actor 및 관련 시스템 구현)
#include "PRItemSpawnerActor.h"

#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "ProjectR/ItemSystem/Data/PRAmmoDataAsset.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/World/Drop/PRItemDropManagerSubsystem.h"
#include "ProjectR/World/Pickable/PRRewardPickupActor.h"

APRItemSpawnerActor::APRItemSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SpawnBounds->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->SetGenerateOverlapEvents(false);
	SetRootComponent(SpawnBounds);
}

void APRItemSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		BindActivateEvents();
		BindPrepareRespawnEvent();

		if (bAutoStart)
		{
			// 서버 자동 스폰 타이머 시작
			StartSpawning();
		}
	}
}

void APRItemSpawnerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		// 서버 스폰 타이머 정리
		StopSpawning();
		UnbindActivateEvents();
		UnbindPrepareRespawnEvent();
	}

	Super::EndPlay(EndPlayReason);
}

void APRItemSpawnerActor::StartSpawning()
{
	if (!HasAuthority() || !bSpawnEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float SafeSpawnInterval = FMath::Max(SpawnInterval, 0.1f);
	const float SafeInitialSpawnDelay = FMath::Max(InitialSpawnDelay, 0.0f);

	// 기존 타이머 중복 등록 방지
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	if (SafeInitialSpawnDelay <= KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &APRItemSpawnerActor::HandleSpawnTimerElapsed);
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &APRItemSpawnerActor::HandleSpawnTimerElapsed, SafeSpawnInterval, true, SafeSpawnInterval);
		return;
	}

	World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &APRItemSpawnerActor::HandleSpawnTimerElapsed, SafeSpawnInterval, true, SafeInitialSpawnDelay);
}

void APRItemSpawnerActor::StopSpawning()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 반복 스폰 타이머 정지
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

bool APRItemSpawnerActor::SpawnItemOnce()
{
	if (!HasAuthority())
	{
		return false;
	}

	PruneAlivePickups();
	if (!CanSpawnItem())
	{
		return false;
	}

	UPRAmmoDataAsset* AmmoData = PickAmmoData();
	if (!IsValid(AmmoData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner][Server] 스폰 가능한 탄약 데이터 없음. Spawner = %s"), *GetNameSafe(this));
		return false;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!FindSpawnLocation(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner][Server] NavMesh 스폰 위치 선택 실패. Spawner = %s"), *GetNameSafe(this));
		return false;
	}

	UWorld* World = GetWorld();
	UPRItemDropManagerSubsystem* DropManager = IsValid(World) ? World->GetSubsystem<UPRItemDropManagerSubsystem>() : nullptr;
	if (!IsValid(DropManager))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner][Server] 드롭 매니저 없음. Spawner = %s"), *GetNameSafe(this));
		return false;
	}

	// 기존 드롭 보상 픽업 경로 재사용
	const FPRResolvedDropReward Reward = BuildAmmoReward(AmmoData);
	APRRewardPickupActor* PickupActor = DropManager->SpawnResolvedRewardPickup(Reward, SpawnLocation, this);
	if (!IsValid(PickupActor))
	{
		return false;
	}

	AlivePickups.Add(PickupActor);
	return true;
}

void APRItemSpawnerActor::BindActivateEvents()
{
	if (!HasAuthority() || ActivateEventTags.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	UPREventManagerSubsystem* EventManager = IsValid(World) ? World->GetSubsystem<UPREventManagerSubsystem>() : nullptr;
	if (!IsValid(EventManager))
	{
		return;
	}

	TSet<FGameplayTag> UniqueTags;
	for (const FGameplayTag& ActivateEventTag : ActivateEventTags)
	{
		if (!ActivateEventTag.IsValid() || UniqueTags.Contains(ActivateEventTag) || ActivateEventHandles.Contains(ActivateEventTag))
		{
			continue;
		}

		// 동일 태그 중복 등록 방지
		UniqueTags.Add(ActivateEventTag);

		FDelegateHandle EventHandle = EventManager->Listen(
			ActivateEventTag,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleActivateEvent));
		if (EventHandle.IsValid())
		{
			// EndPlay와 리스폰 준비 단계 정리를 위한 핸들 보관
			ActivateEventHandles.Add(ActivateEventTag, EventHandle);
		}
	}
}

void APRItemSpawnerActor::UnbindActivateEvents()
{
	UWorld* World = GetWorld();
	UPREventManagerSubsystem* EventManager = IsValid(World) ? World->GetSubsystem<UPREventManagerSubsystem>() : nullptr;
	if (IsValid(EventManager))
	{
		for (TPair<FGameplayTag, FDelegateHandle>& EventHandlePair : ActivateEventHandles)
		{
			// EventManager 태그별 수신 핸들 해제
			EventManager->Unlisten(EventHandlePair.Key, EventHandlePair.Value);
		}
	}

	ActivateEventHandles.Reset();
}

void APRItemSpawnerActor::BindPrepareRespawnEvent()
{
	if (!HasAuthority() || PrepareRespawnHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UPRRespawnSubsystem* RespawnSubsystem = IsValid(World) ? World->GetSubsystem<UPRRespawnSubsystem>() : nullptr;
	if (!IsValid(RespawnSubsystem))
	{
		return;
	}

	// 리스폰 준비 단계의 스포너 상태 정리 예약
	PrepareRespawnHandle = RespawnSubsystem->OnPrepareRespawn.AddUObject(this, &ThisClass::HandlePrepareRespawn);
}

void APRItemSpawnerActor::UnbindPrepareRespawnEvent()
{
	if (!PrepareRespawnHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UPRRespawnSubsystem* RespawnSubsystem = IsValid(World) ? World->GetSubsystem<UPRRespawnSubsystem>() : nullptr;
	if (IsValid(RespawnSubsystem))
	{
		// RespawnSubsystem 리스폰 준비 이벤트 수신 핸들 해제
		RespawnSubsystem->OnPrepareRespawn.Remove(PrepareRespawnHandle);
	}

	PrepareRespawnHandle.Reset();
}

void APRItemSpawnerActor::HandleActivateEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!HasAuthority())
	{
		return;
	}

	// 이벤트 페이로드와 태그 내용은 스포너별 필터 없이 구독 태그 자체로만 판정
	StartSpawning();
}

void APRItemSpawnerActor::HandlePrepareRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	// 리스폰 준비 단계에서 기존 이벤트 수신 경로와 반복 스폰 정리
	StopSpawning();
	bSpawnEnabled = false;
}

void APRItemSpawnerActor::HandleSpawnTimerElapsed()
{
	// 타이머 기반 단일 스폰 요청
	SpawnItemOnce();
}

void APRItemSpawnerActor::PruneAlivePickups()
{
	for (int32 Index = AlivePickups.Num() - 1; Index >= 0; --Index)
	{
		APRRewardPickupActor* PickupActor = AlivePickups[Index].Get();
		if (!IsValid(PickupActor) || PickupActor->IsClaimed())
		{
			// 파괴 또는 Claim 완료 픽업 참조 제거
			AlivePickups.RemoveAtSwap(Index);
		}
	}
}

bool APRItemSpawnerActor::CanSpawnItem() const
{
	if (!bSpawnEnabled || MaxAlivePickups <= 0)
	{
		return false;
	}

	if (AlivePickups.Num() >= MaxAlivePickups)
	{
		return false;
	}

	return !SpawnableAmmoItems.IsEmpty();
}

UPRAmmoDataAsset* APRItemSpawnerActor::PickAmmoData() const
{
	TArray<UPRAmmoDataAsset*> ValidAmmoItems;
	for (const TObjectPtr<UPRAmmoDataAsset>& AmmoData : SpawnableAmmoItems)
	{
		if (IsValid(AmmoData.Get()))
		{
			// 유효 탄약 데이터 후보 수집
			ValidAmmoItems.Add(AmmoData.Get());
		}
	}

	if (ValidAmmoItems.IsEmpty())
	{
		return nullptr;
	}

	const int32 PickedIndex = FMath::RandRange(0, ValidAmmoItems.Num() - 1);
	return ValidAmmoItems[PickedIndex];
}

FPRResolvedDropReward APRItemSpawnerActor::BuildAmmoReward(UPRAmmoDataAsset* AmmoData) const
{
	const int32 SafeMinQuantity = FMath::Max(MinQuantity, 1);
	const int32 SafeMaxQuantity = FMath::Max(MaxQuantity, SafeMinQuantity);

	FPRResolvedDropReward Reward;
	Reward.RewardType = EPRRewardType::Ammo;
	Reward.DistributionRule = DistributionRule;
	Reward.ItemData = AmmoData;
	Reward.ItemAssetId = IsValid(AmmoData) ? AmmoData->GetPrimaryAssetId() : FPrimaryAssetId();
	Reward.Quantity = FMath::RandRange(SafeMinQuantity, SafeMaxQuantity);
	Reward.bSpawnPickup = true;
	return Reward;
}

bool APRItemSpawnerActor::FindSpawnLocation(FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(SpawnBounds))
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!IsValid(NavigationSystem))
	{
		return false;
	}

	const FVector BoxExtent = SpawnBounds->GetUnscaledBoxExtent();
	if (BoxExtent.X <= 0.0f || BoxExtent.Y <= 0.0f || BoxExtent.Z <= 0.0f)
	{
		return false;
	}

	const FVector NavProjectExtent = SpawnBounds->GetScaledBoxExtent();
	const int32 AttemptCount = FMath::Max(SpawnLocationMaxAttempts, 1);

	for (int32 AttemptIndex = 0; AttemptIndex < AttemptCount; ++AttemptIndex)
	{
		const FVector LocalCandidate(
			FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
			FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
			0.0f);
		const FVector WorldCandidate = SpawnBounds->GetComponentTransform().TransformPosition(LocalCandidate);

		FNavLocation ProjectedLocation;
		if (!NavigationSystem->ProjectPointToNavigation(WorldCandidate, ProjectedLocation, NavProjectExtent))
		{
			continue;
		}

		if (!IsInsideSpawnBounds(ProjectedLocation.Location))
		{
			continue;
		}

		// 박스 내부 NavMesh 투영 위치 확정
		OutLocation = ProjectedLocation.Location;
		return !OutLocation.ContainsNaN();
	}

	return false;
}

bool APRItemSpawnerActor::IsInsideSpawnBounds(const FVector& WorldLocation) const
{
	if (!IsValid(SpawnBounds))
	{
		return false;
	}

	const FVector LocalLocation = SpawnBounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector BoxExtent = SpawnBounds->GetUnscaledBoxExtent();
	constexpr float BoundsTolerance = 1.0f;

	return FMath::Abs(LocalLocation.X) <= BoxExtent.X + BoundsTolerance
		&& FMath::Abs(LocalLocation.Y) <= BoxExtent.Y + BoundsTolerance
		&& FMath::Abs(LocalLocation.Z) <= BoxExtent.Z + BoundsTolerance;
}
