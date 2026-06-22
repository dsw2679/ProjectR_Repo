// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 다운/사망 상태 연동 및 게임오버 처리 구현)
// Author: 배유찬 (세션/멀티플레이 흐름 및 전멸 리스폰, 웨이포인트 이동 룰 구현)
#include "PRPlayGameMode.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "PRGameStateBase.h"
#include "TimerManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/World/PRSpawnPointTags.h"
#include "ProjectR/World/PRWorldRegistry.h"

APRPlayGameMode::APRPlayGameMode()
{
	PlayerControllerClass = APRPlayerController::StaticClass();
	PlayerStateClass      = APRPlayerState::StaticClass();
	GameStateClass        = APRGameStateBase::StaticClass();

	bUseSeamlessTravel = true;
}

void APRPlayGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 월드 세이브 로드는 세이브 시스템 구현 시 여기서 수행
	// 현재는 기본 초기값을 유지한다
	HostWorldSave.Version = EPRSaveVersion::V1;

	if (UPRGameInstance* GameInstance = GetGameInstance<UPRGameInstance>())
	{
		// 월드 진행 상태 소비
		GameInstance->ConsumePendingWorldSaveData(HostWorldSave);

		// ServerTravel 진입 SpawnPoint 소비
		TravelSpawnPointId = GameInstance->ConsumePendingTravelSpawnPointId();
	}
}

void APRPlayGameMode::InitGameState()
{
	Super::InitGameState();

	// 심리스 트래블 기존 플레이어도 공유할 월드 진행 상태 주입 위치
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		const bool bHasHostWorldSaveData =
			HostWorldSave.LastVisitedWaypoint.IsValid()
			|| HostWorldSave.LastActivatedWaypoint.IsValid()
			|| HostWorldSave.SavedSpawnPoint.IsValid()
			|| !HostWorldSave.UnlockedWaypoints.IsEmpty()
			|| !HostWorldSave.DefeatedBosses.IsEmpty();
		if (!GS->GetLastVisitedWaypoint().IsValid() && !GS->GetLastActivatedWaypoint().IsValid() && bHasHostWorldSaveData)
		{
			GS->InitializeFromWorldSave(HostWorldSave);
		}

		UnlockDefaultWaypoints();
	}
}

void APRPlayGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (APRPlayerState* NewPlayerState = IsValid(NewPlayer) ? NewPlayer->GetPlayerState<APRPlayerState>() : nullptr)
	{
		// 최초 스폰 전 PlayerStart 슬롯 할당
		AssignPRPlayerIndex(NewPlayerState);
	}

	APRPlayerController* PRPlayerController = Cast<APRPlayerController>(NewPlayer);
	if (IsValid(PRPlayerController)
		&& PRPlayerController->IsLocalController()
		&& (GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer))
	{
		if (UPRGameInstance* GameInstance = GetGameInstance<UPRGameInstance>())
		{
			// 호스트와 Standalone 직접 실행용 로컬 세이브 주입
			GameInstance->EnsureLocalCharacterReadyForSession();
			AcceptGuestCharacter(PRPlayerController, GameInstance->GetLocalCharacter());
		}
	}

	// 원격 클라이언트 페이로드는 ServerSubmitCharacter RPC 경로 수신
}

void APRPlayGameMode::Logout(AController* Exiting)
{
	// 보상은 즉시 지급 원칙이므로 여기서 Reliable RPC를 호출하지 않는다
	// (연결이 이미 해제 단계이므로 도달이 보장되지 않음)
	// 필요한 정리 작업(월드 오브젝트 소유권 이관 등)만 수행한다
	Super::Logout(Exiting);
}

AActor* APRPlayGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// SpawnPoint 우선 스폰 지점
	const FGameplayTag SpawnPointId = ResolvePlayerStartSpawnPointId();
	if (UWorld* World = GetWorld())
	{
		if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
		{
			if (AActor* ResolvedStartActor = RespawnSubsystem->ResolvePlayerStartActor(Player, SpawnPointId))
			{
				// 실제 플레이어 스폰 지점 기록
				RecordCurrentWorldSpawnPoint(SpawnPointId);
				return ResolvedStartActor;
			}
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void APRPlayGameMode::ReportWaypointActivated(const FPRWaypointKey& WaypointKey)
{
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		GS->ActivateWaypoint(WaypointKey);
	}
}

void APRPlayGameMode::ReportBossDefeated(FName BossId)
{
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		GS->MarkBossDefeated(BossId);
	}
}

bool APRPlayGameMode::ValidateCharacterPayload(const FPRCharacterSaveData& Payload, FString& OutReason) const
{
	// 포맷 버전 체크
	if (Payload.Version != EPRSaveVersion::V1)
	{
		OutReason = TEXT("Save version mismatch");
		return false;
	}

	// 표시명 길이 체크. 빈 이름은 신규 세이브 기본값으로 허용
	if (Payload.DisplayName.Len() > MaxDisplayNameLength)
	{
		OutReason = TEXT("Invalid display name length");
		return false;
	}

	// 레벨 상한 체크
	if (Payload.Level < 1 || Payload.Level > MaxCharacterLevel)
	{
		OutReason = TEXT("Level out of range");
		return false;
	}

	// 경험치 음수 방지
	if (Payload.Experience < 0)
	{
		OutReason = TEXT("Negative experience");
		return false;
	}

	return true;
}

bool APRPlayGameMode::AcceptGuestCharacter(APRPlayerController* From, const FPRCharacterSaveData& Payload)
{
	if (!IsValid(From))
	{
		return false;
	}

	APRPlayerState* PS = From->GetPlayerState<APRPlayerState>();
	if (!IsValid(PS))
	{
		return false;
	}

	if (PS->HasAcceptedCharacterPayload())
	{
		// ReceivedPlayer 재호출로 전달된 클라이언트 로컬 세이브 재적용 방지
		AssignPRPlayerIndex(PS);
		return true;
	}

	FString Reason;
	if (!ValidateCharacterPayload(Payload, Reason))
	{
		return false;
	}

	// PlayerState에 주입. 이후 복제로 모든 클라에 전파
	AssignPRPlayerIndex(PS);
	PS->MarkCharacterPayloadAccepted();
	PS->QueueSaveDataApply(Payload, true);
	if (IsValid(PS->GetPawn()))
	{
		// 게스트 페이로드와 기본 지급 아이템 즉시 복원
		PS->ApplyPendingSaveData();
	}

	return true;
}

void APRPlayGameMode::AssignPRPlayerIndex(APRPlayerState* PlayerState) const
{
	if (!HasAuthority() || !IsValid(PlayerState))
	{
		return;
	}

	if (PlayerState->GetPRPlayerIndex() != INDEX_NONE)
	{
		return;
	}

	const AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return;
	}

	TSet<int32> UsedPlayerIndices;
	for (APlayerState* ExistingPlayerState : CurrentGameState->PlayerArray)
	{
		const APRPlayerState* ExistingPRPlayerState = Cast<APRPlayerState>(ExistingPlayerState);
		if (!IsValid(ExistingPRPlayerState) || ExistingPRPlayerState == PlayerState)
		{
			continue;
		}

		const int32 ExistingPlayerIndex = ExistingPRPlayerState->GetPRPlayerIndex();
		if (ExistingPlayerIndex != INDEX_NONE)
		{
			UsedPlayerIndices.Add(ExistingPlayerIndex);
		}
	}

	int32 NewPlayerIndex = 0;
	while (UsedPlayerIndices.Contains(NewPlayerIndex))
	{
		++NewPlayerIndex;
	}

	PlayerState->SetPRPlayerIndex(NewPlayerIndex);
}

void APRPlayGameMode::GrantRewardTo(APRPlayerController* Target, const FPRRewardGrant& Grant)
{
	if (!IsValid(Target))
	{
		return;
	}

	// 오너 클라에게만 푸시. 수신 측에서 GameInstance에 즉시 반영
	Target->ClientGrantReward(Grant);
}

void APRPlayGameMode::NotifyPlayerSurvivalStateChanged(APRPlayerState* PlayerState)
{
	if (!HasAuthority() || !IsValid(PlayerState) || !PlayerState->IsCombatParticipant())
	{
		return;
	}

	EvaluatePartyWipe();
}

void APRPlayGameMode::UnlockDefaultWaypoints()
{
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		for (FPRWaypointKey& WaypointKey : DefaultUnlockedWaypoints)
		{
			GS->UnlockWaypoint(WaypointKey);
		}
	}
}

void APRPlayGameMode::EvaluatePartyWipe()
{
	if (!HasAuthority() || bPartyWipeInProgress)
	{
		return;
	}

	const AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return;
	}

	int32 ParticipantCount = 0;
	int32 OutOfFightCount = 0;
	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		++ParticipantCount;
		if (PRPlayerState->IsOutOfFight())
		{
			++OutOfFightCount;
		}
	}

	if (ParticipantCount > 0 && ParticipantCount == OutOfFightCount)
	{
		ConfirmPartyWipe();
	}
}

void APRPlayGameMode::ConfirmPartyWipe()
{
	if (!HasAuthority() || bPartyWipeInProgress)
	{
		return;
	}

	AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return;
	}

	bPartyWipeInProgress = true;
	OnPartyWipeConfirmed.Broadcast();
	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		if (!PRPlayerState->IsDead())
		{
			PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_Death);
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(PRPlayerState->GetOwner());
		if (IsValid(PlayerController))
		{
			PlayerController->ClientPlayPartyWipeSound(PartyWipeSound);

			// 리스폰 전환 연출
			PlayerController->ClientNotifyMapTransition(PartyWipeRespawnDelay, EPRMapTransitionType::Respawn);
			PlayerController->ClientShowPartyWipeWidget(PartyWipeWidgetClass);
		}

		// if (PRPlayerState->IsDown())
		// {
		// 	PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_PlayerDeathConfirmed);
		// }
		// else if (!PRPlayerState->IsDead())
		// {
		// 	PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_Death);
		// }
	}

	if (UWorld* World = GetWorld())
	{
		// 사망 연출 대기
		World->GetTimerManager().SetTimer(
			PartyWipeRespawnTimerHandle,
			this,
			&ThisClass::RespawnParty,
			PartyWipeRespawnDelay,
			false);
	}
	else
	{
		RespawnParty();
	}
}

void APRPlayGameMode::RespawnParty()
{
	// 서버 권위 리스폰
	if (!HasAuthority())
	{
		return;
	}

	const FGameplayTag RespawnSpawnPointId = ResolvePartyRespawnSpawnPointId();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		bPartyWipeInProgress = false;
		OnPartyRespawned.Broadcast();
		return;
	}

	bool bRespawned = false;
	if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
	{
		// 월드 오브젝트와 플레이어 복구
		bRespawned = RespawnSubsystem->RespawnWorldAndPlayers(RespawnSpawnPointId);
	}

	if (!bRespawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("Party respawn failed."));
	}
	else
	{
		// 전멸 리스폰 완료 지점 기록
		RecordCurrentWorldSpawnPoint(RespawnSpawnPointId);
	}

	bPartyWipeInProgress = false;
	OnPartyRespawned.Broadcast();
}

FGameplayTag APRPlayGameMode::ResolvePlayerStartSpawnPointId() const
{
	const APRGameStateBase* PRGameState = Cast<APRGameStateBase>(GameState);

	// 맵 이동 진입 지점 우선
	if (TravelSpawnPointId.IsValid())
	{
		return TravelSpawnPointId;
	}

	if (IsValid(PRGameState) && PRGameState->GetSavedSpawnPoint().IsValid())
	{
		return PRGameState->GetSavedSpawnPoint().SpawnPointId;
	}

	if (IsValid(PRGameState) && PRGameState->GetLastVisitedWaypoint().IsValid())
	{
		return PRGameState->GetLastVisitedWaypoint().WaypointId;
	}

	return PRSpawnPointTags::SpawnPoint_Default;
}

FGameplayTag APRPlayGameMode::ResolvePartyRespawnSpawnPointId() const
{
	// 전멸 리스폰 지점 우선
	const APRGameStateBase* PRGameState = Cast<APRGameStateBase>(GameState);
	if (IsValid(PRGameState) && PRGameState->GetLastActivatedWaypoint().IsValid())
	{
		const FGameplayTag RespawnSpawnPointId = PRGameState->GetLastActivatedWaypoint().WaypointId;
		UE_LOG(LogTemp, Log, TEXT("Party respawn spawn point resolved from LastActivatedWaypoint: %s"), *RespawnSpawnPointId.ToString());
		return RespawnSpawnPointId;
	}

	if (IsValid(PRGameState) && PRGameState->GetLastVisitedWaypoint().IsValid())
	{
		const FGameplayTag RespawnSpawnPointId = PRGameState->GetLastVisitedWaypoint().WaypointId;
		UE_LOG(LogTemp, Log, TEXT("Party respawn spawn point fallback from LastVisitedWaypoint: %s"), *RespawnSpawnPointId.ToString());
		return RespawnSpawnPointId;
	}

	UE_LOG(LogTemp, Log, TEXT("Party respawn spawn point fallback to default"));
	return PRSpawnPointTags::SpawnPoint_Default;
}

void APRPlayGameMode::RecordCurrentWorldSpawnPoint(FGameplayTag SpawnPointId) const
{
	if (!SpawnPointId.IsValid())
	{
		return;
	}

	APRGameStateBase* PRGameState = Cast<APRGameStateBase>(GameState);
	if (!IsValid(PRGameState))
	{
		return;
	}

	FGameplayTag CurrentWorldId;
	if (!ResolveCurrentWorldId(CurrentWorldId))
	{
		return;
	}

	FPRSpawnPointKey SpawnPointKey;
	SpawnPointKey.WorldId = CurrentWorldId;
	SpawnPointKey.SpawnPointId = SpawnPointId;
	PRGameState->RecordSpawnPoint(SpawnPointKey);
}

bool APRPlayGameMode::ResolveCurrentWorldId(FGameplayTag& OutWorldId) const
{
	OutWorldId = FGameplayTag();

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	const UPRWorldRegistry* WorldRegistry = IsValid(Settings) ? Settings->GetWorldRegistrySync() : nullptr;
	UWorld* World = GetWorld();
	if (!IsValid(WorldRegistry) || !IsValid(World))
	{
		return false;
	}

	return WorldRegistry->FindWorldIdByMap(World, OutWorldId);
}
