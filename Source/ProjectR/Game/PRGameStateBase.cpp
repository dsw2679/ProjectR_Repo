// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Game State 기본 구조 구현)
#include "PRGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"


void APRGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRGameStateBase, LastVisitedWaypoint);
	DOREPLIFETIME(APRGameStateBase, LastActivatedWaypoint);
	DOREPLIFETIME(APRGameStateBase, SavedSpawnPoint);
	DOREPLIFETIME(APRGameStateBase, UnlockedWaypoints);
	DOREPLIFETIME(APRGameStateBase, DefeatedBosses);
	DOREPLIFETIME(APRGameStateBase, WorldSaveVersion);
}

void APRGameStateBase::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	// PlayerArray 추가 이후 참가자 수 이벤트 갱신
	RefreshPlayerCount();
}

void APRGameStateBase::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	// PlayerArray 제거 이후 참가자 수 이벤트 갱신
	RefreshPlayerCount();
}

bool APRGameStateBase::IsWaypointUnlocked(const FPRWaypointKey& WaypointKey) const
{
	if (!WaypointKey.IsValid())
	{
		return false;
	}

	return UnlockedWaypoints.ContainsByPredicate([&WaypointKey](const FPRUnlockedWaypointEntry& Entry)
	{
		return Entry.WaypointKey == WaypointKey;
	});
}

bool APRGameStateBase::HasUnlockedWaypointInWorld(FGameplayTag WorldId) const
{
	if (!WorldId.IsValid())
	{
		return false;
	}

	return UnlockedWaypoints.ContainsByPredicate([WorldId](const FPRUnlockedWaypointEntry& Entry)
	{
		return Entry.WaypointKey.WorldId.MatchesTagExact(WorldId);
	});
}

bool APRGameStateBase::IsBossDefeated(FName BossId) const
{
	return DefeatedBosses.Contains(BossId);
}

TArray<APRPlayerCharacter*> APRGameStateBase::GetPlayerCharacters() const
{
	TArray<APRPlayerCharacter*> OutCharacters;
	OutCharacters.Reserve(PlayerArray.Num());

	for (APlayerState* PS : PlayerArray)
	{
		if (!IsValid(PS))
		{
			continue;
		}

		APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(PS->GetPawn());
		if (!IsValid(PlayerCharacter))
		{
			continue;
		}

		OutCharacters.Add(PlayerCharacter);
	}

	return OutCharacters;
}

int32 APRGameStateBase::GetPlayerCount() const
{
	int32 NewPlayerCount = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		++NewPlayerCount;
	}

	return NewPlayerCount;
}

void APRGameStateBase::RefreshPlayerCount()
{
	const int32 NewPlayerCount = GetPlayerCount();
	if (NewPlayerCount == LastBroadcastPlayerCount)
	{
		return;
	}

	// PlayerArray 변경 이벤트 전파
	LastBroadcastPlayerCount = NewPlayerCount;
	OnPlayerCountChanged.Broadcast();
}

void APRGameStateBase::InitializeFromWorldSave(const FPRWorldSaveData& WorldSave)
{
	if (!HasAuthority())
	{
		return;
	}

	WorldSaveVersion = WorldSave.Version;
	LastVisitedWaypoint = WorldSave.LastVisitedWaypoint;
	LastActivatedWaypoint = WorldSave.LastActivatedWaypoint.IsValid()
		? WorldSave.LastActivatedWaypoint
		: WorldSave.LastVisitedWaypoint;
	SavedSpawnPoint = WorldSave.SavedSpawnPoint;
	UnlockedWaypoints = WorldSave.UnlockedWaypoints;
	DefeatedBosses = WorldSave.DefeatedBosses;
}

FPRWorldSaveData APRGameStateBase::MakeWorldSaveData() const
{
	FPRWorldSaveData SaveData;
	SaveData.Version = WorldSaveVersion;
	SaveData.LastVisitedWaypoint = LastVisitedWaypoint;
	SaveData.LastActivatedWaypoint = LastActivatedWaypoint;
	SaveData.SavedSpawnPoint = SavedSpawnPoint;
	SaveData.UnlockedWaypoints = UnlockedWaypoints;
	SaveData.DefeatedBosses = DefeatedBosses;
	return SaveData;
}

void APRGameStateBase::ActivateWaypoint(const FPRWaypointKey& WaypointKey)
{
	if (!HasAuthority() || !WaypointKey.IsValid())
	{
		return;
	}

	UnlockWaypoint(WaypointKey);
	LastActivatedWaypoint = WaypointKey;

	// 서버 로컬 호스트 활성화 이벤트
	OnWaypointActivated.Broadcast(WaypointKey);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Waypoint unlocked: World=%s Waypoint=%s"),
		*WaypointKey.WorldId.ToString(),
		*WaypointKey.WaypointId.ToString());
}

void APRGameStateBase::UnlockWaypoint(const FPRWaypointKey& WaypointKey)
{
	if (!HasAuthority() || !WaypointKey.IsValid())
	{
		return;
	}

	FPRUnlockedWaypointEntry UnlockedEntry;
	UnlockedEntry.WaypointKey = WaypointKey;
	UnlockedWaypoints.AddUnique(UnlockedEntry);
}

void APRGameStateBase::VisitWaypoint(const FPRWaypointKey& WaypointKey)
{
	if (!HasAuthority() || !WaypointKey.IsValid())
	{
		return;
	}

	if (LastVisitedWaypoint == WaypointKey)
	{
		return;
	}

	// 실제 Travel 확정 목적지 기록
	LastVisitedWaypoint = WaypointKey;
	OnLastVisitedWaypointChanged.Broadcast(WaypointKey);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("LastVisitedWaypoint updated: World=%s Waypoint=%s"),
		*WaypointKey.WorldId.ToString(),
		*WaypointKey.WaypointId.ToString());
}

void APRGameStateBase::RecordSpawnPoint(const FPRSpawnPointKey& SpawnPointKey)
{
	if (!HasAuthority() || !SpawnPointKey.IsValid())
	{
		return;
	}

	if (SavedSpawnPoint == SpawnPointKey)
	{
		return;
	}

	// 실제 소환 완료 위치 기록
	SavedSpawnPoint = SpawnPointKey;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("SavedSpawnPoint updated: World=%s SpawnPoint=%s"),
		*SpawnPointKey.WorldId.ToString(),
		*SpawnPointKey.SpawnPointId.ToString());
}

void APRGameStateBase::MarkBossDefeated(FName BossId)
{
	if (!HasAuthority())
	{
		return;
	}

	if (DefeatedBosses.Contains(BossId))
	{
		return;
	}

	DefeatedBosses.Add(BossId);
	OnBossDefeated.Broadcast(BossId);
}

void APRGameStateBase::ResetWorldProgress()
{
	if (!HasAuthority())
	{
		return;
	}

	// 월드 진행 상태 기본값 복귀
	LastVisitedWaypoint = FPRWaypointKey();
	LastActivatedWaypoint = FPRWaypointKey();
	// 실제 시작 위치 유지
	UnlockedWaypoints.Reset();
	DefeatedBosses.Reset();
	WorldSaveVersion = EPRSaveVersion::V1;

	// 서버 로컬 UI 갱신 이벤트
	OnLastVisitedWaypointChanged.Broadcast(LastVisitedWaypoint);
}

void APRGameStateBase::ServerSubmitWorldMarker(const FPRWorldMarkerRequest& Request)
{
	// 서버 권위 확인
	if (!HasAuthority())
	{
		return;
	}

	// 요청자 확인
	APlayerController* RequestingController = Request.RequestingController;
	if (!IsValid(RequestingController))
	{
		return;
	}

	// 플레이어 식별자 확인
	APlayerState* SourcePlayerState = RequestingController->PlayerState;
	if (!IsValid(SourcePlayerState))
	{
		return;
	}

	// 기본 마커 데이터
	FPRWorldMarkerData MarkerData;
	MarkerData.MarkerId = FGuid::NewGuid();
	MarkerData.SourcePlayerState = SourcePlayerState;
	MarkerData.WorldLocation = Request.WorldLocation;
	MarkerData.ServerWorldTime = IsValid(GetWorld()) ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (const UPRDeveloperSettings* const Settings = GetDefault<UPRDeveloperSettings>())
	{
		MarkerData.Duration = Settings->DefaultWorldMarkerDuration;
		MarkerData.VisualData = Settings->GetWorldMarkerPreset(EPRWorldMarkerPreset::Default);
	}

	AActor* TargetActor = Request.TargetActor;
	// 인터페이스 대상 검증
	const bool bValidInterfaceTarget =
		IsValid(TargetActor) &&
		TargetActor->GetIsReplicated() &&
		TargetActor->GetClass()->ImplementsInterface(UPRPingMarkerTargetInterface::StaticClass()) &&
		IPRPingMarkerTargetInterface::Execute_CanCreatePingMarker(TargetActor, RequestingController);

	if (bValidInterfaceTarget)
	{
		// 추적형 마커로 페이로드 구성
		MarkerData.LocationMode = EPRWorldMarkerLocationMode::InterfaceTarget;
		MarkerData.TargetActor = TargetActor;
		MarkerData.WorldLocation = IPRPingMarkerTargetInterface::Execute_GetPingMarkerWorldLocation(TargetActor);
		MarkerData.VisualData = IPRPingMarkerTargetInterface::Execute_GetPingMarkerVisualData(TargetActor);
	}
	else
	{
		// 고정 위치 마커로 페이로드 구성
		MarkerData.LocationMode = EPRWorldMarkerLocationMode::FixedWorldLocation;
		MarkerData.TargetActor = nullptr;
	}

	// 서버 활성 목록 등록
	ActiveWorldMarkers.Add(MarkerData.MarkerId, MarkerData);
	const TWeakObjectPtr<APlayerState> SourcePlayerKey(SourcePlayerState);
	ActiveMarkerIdsByPlayer.FindOrAdd(SourcePlayerKey).MarkerIds.Add(MarkerData.MarkerId);

	// 마커 추가 이벤트 전파
	FPRWorldMarkerEventPayload AddedPayload;
	AddedPayload.EventType = EPRWorldMarkerEventType::Added;
	AddedPayload.MarkerData = MarkerData;
	AddedPayload.MarkerId = MarkerData.MarkerId;
	MulticastWorldMarkerEvent(AddedPayload);

	if (MarkerData.Duration > 0.0f)
	{
		// 만료 타이머 등록
		FTimerDelegate ExpireDelegate;
		ExpireDelegate.BindUObject(this, &ThisClass::RemoveWorldMarker, MarkerData.MarkerId);

		FTimerHandle& TimerHandle = WorldMarkerExpireTimers.FindOrAdd(MarkerData.MarkerId);
		GetWorldTimerManager().SetTimer(TimerHandle, ExpireDelegate, MarkerData.Duration, false);
	}

	// 인당 개수 제한
	EnforceMarkerLimit(SourcePlayerState);
}

void APRGameStateBase::OnRep_LastVisitedWaypoint(FPRWaypointKey OldLastVisitedWaypoint)
{
	if (LastVisitedWaypoint == OldLastVisitedWaypoint)
	{
		return;
	}

	OnLastVisitedWaypointChanged.Broadcast(LastVisitedWaypoint);
}

void APRGameStateBase::MulticastWorldMarkerEvent_Implementation(const FPRWorldMarkerEventPayload& Payload)
{
	// 로컬 이벤트 버스 전달
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventManager))
	{
		return;
	}

	EventManager->BroadcastTyped(PRGameplayTags::Event_Player_WorldMarker, Payload);
}

void APRGameStateBase::EnforceMarkerLimit(APlayerState* SourcePlayerState)
{
	// 서버 권위 확인
	if (!HasAuthority() || !IsValid(SourcePlayerState))
	{
		return;
	}

	// 제한값 보정
	const int32 MarkerLimit = FMath::Max(1, MaxActiveMarkersPerPlayer);
	const TWeakObjectPtr<APlayerState> SourcePlayerKey(SourcePlayerState);
	FPRActiveWorldMarkerList* MarkerList = ActiveMarkerIdsByPlayer.Find(SourcePlayerKey);
	if (!MarkerList)
	{
		return;
	}

	while (MarkerList->MarkerIds.Num() > MarkerLimit)
	{
		// 오래된 마커 제거
		const FGuid OldestMarkerId = MarkerList->MarkerIds[0];
		MarkerList->MarkerIds.RemoveAt(0);

		// 활성 데이터가 남아있는 경우만 제거 이벤트 전파
		if (ActiveWorldMarkers.Contains(OldestMarkerId))
		{
			RemoveWorldMarker(OldestMarkerId);
		}
	}
}

void APRGameStateBase::RemoveWorldMarker(FGuid MarkerId)
{
	// 서버 권위 확인
	if (!HasAuthority() || !MarkerId.IsValid())
	{
		return;
	}

	if (!ActiveWorldMarkers.Contains(MarkerId))
	{
		return;
	}

	ActiveWorldMarkers.Remove(MarkerId);

	// 플레이어별 목록 정리
	for (TPair<TWeakObjectPtr<APlayerState>, FPRActiveWorldMarkerList>& Pair : ActiveMarkerIdsByPlayer)
	{
		Pair.Value.MarkerIds.Remove(MarkerId);
	}

	// 만료 타이머 정리
	if (FTimerHandle* TimerHandle = WorldMarkerExpireTimers.Find(MarkerId))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		WorldMarkerExpireTimers.Remove(MarkerId);
	}

	// 제거 이벤트 전파
	FPRWorldMarkerEventPayload RemovedPayload;
	RemovedPayload.EventType = EPRWorldMarkerEventType::Removed;
	RemovedPayload.MarkerId = MarkerId;
	MulticastWorldMarkerEvent(RemovedPayload);
}
