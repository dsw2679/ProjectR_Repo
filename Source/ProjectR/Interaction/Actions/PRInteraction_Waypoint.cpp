// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상호작용 시 로딩화면 프리웜 연동)
// Author: 배유찬 (웨이포인트 활성화, 리스폰 지점 등록 및 트래블 UI 연동 구현)
#include "PRInteraction_Waypoint.h"

#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRWorldRegistry.h"
#include "ProjectR/World/PRWaypointActor.h"
#include "UObject/ConstructorHelpers.h"

UPRInteraction_Waypoint::UPRInteraction_Waypoint()
{
	PartySyncCheckDelay = 2.1f;
}

void UPRInteraction_Waypoint::RequestWaypointTravel(APRPlayerController* RequestingController, const FPRWaypointKey& WaypointKey)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(RequestingController) || !RequestingController->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: invalid host controller"));
		return;
	}

	TSoftObjectPtr<UWorld> MapAsset;
	if (!ValidateWaypointTravelRequest(WaypointKey, MapAsset))
	{
		return;
	}

	NotifyLoadingScreenToAllPlayers(MapAsset.GetLongPackageName());

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: game state is not available"));
		return;
	}

	FPRWaypointKey InteractedWaypointKey;
	if (!ResolveInteractedWaypointKey(InteractedWaypointKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: interacted waypoint is not resolved"));
		return;
	}

	// 실제 Travel 확정 직전 마지막 상호작용 Waypoint 기록
	GameState->VisitWaypoint(InteractedWaypointKey);

	// UI 선택 대기 종료
	bWaitingForWaypointTravelSelection = false;
	bWaitingActivateFadeOut = false;
	if (IsValid(World))
	{
		// Travel UI 오픈 예약 정리
		World->GetTimerManager().ClearTimer(WaypointActivateTimerHandle);
	}
	RequestingController->ClearPendingWaypointTravelInteraction(this);
	UnlockPlayerInteraction();

	// 선택 노드 목적지 이동
	StartWaypointTravelWhenLoadingScreenReady(MapAsset, WaypointKey.WaypointId);
}

void UPRInteraction_Waypoint::CancelWaypointTravel(APRPlayerController* RequestingController)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(RequestingController) || !RequestingController->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel cancel rejected: invalid host controller"));
		return;
	}

	if (!bWaitingForWaypointTravelSelection)
	{
		return;
	}

	// UI 선택 대기 취소
	bWaitingForWaypointTravelSelection = false;
	bWaitingActivateFadeOut = false;
	if (UWorld* World = GetWorld())
	{
		// Travel UI 오픈 예약 정리
		World->GetTimerManager().ClearTimer(WaypointActivateTimerHandle);
	}
	RequestingController->ClearPendingWaypointTravelInteraction(this);
	BroadcastWaypointCancelEventToAllPlayers();
	NotifyMapTransitionToAllPlayers(EPRMapTransitionType::CancelTravel);
	ClearPartySyncWaitingMessages();
	UnlockPlayerInteraction();
}

bool UPRInteraction_Waypoint::CanInteract_Implementation(AActor* Interactor) const
{
	if (auto ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		return !ASC->HasMatchingGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	return Super::CanInteract_Implementation(Interactor);
}

void UPRInteraction_Waypoint::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	if (!bWaitingActivateFadeOut && !bWaitingForWaypointTravelSelection)
	{
		// UI 전환 외부 종료에 따른 잠금 해제
		UnlockPlayerInteraction();
	}

	Super::EndInteraction_Implementation(Interactor, bCanceled);
}

void UPRInteraction_Waypoint::HandlePartySyncConditionMet()
{
	if (bWaitingActivateFadeOut || bWaitingForWaypointTravelSelection)
	{
		return;
	}

	RecordWaypointActivation();
	RespawnWorldObjects();
	ClearPartySyncWaitingMessages();

	// 호스트 목적지 선택 대기
	LockPlayerInteraction();
	ScheduleWaypointTravelUI();
}

bool UPRInteraction_Waypoint::IsPartySyncActionLocked() const
{
	// Travel UI 전환 중 상호작용 입력 해제에 따른 중복 종료 피드백 차단
	return Super::IsPartySyncActionLocked() || bWaitingActivateFadeOut || bWaitingForWaypointTravelSelection;
}

void UPRInteraction_Waypoint::RecordWaypointActivation()
{
	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	FPRWaypointKey ActivatedWaypointKey;
	if (!ResolveInteractedWaypointKey(ActivatedWaypointKey))
	{
		return;
	}

	// Waypoint 해금 상태 기록
	GameState->ActivateWaypoint(ActivatedWaypointKey);
}

void UPRInteraction_Waypoint::RespawnWorldObjects()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
	{
		// Waypoint 활성화 월드 오브젝트 복구
		RespawnSubsystem->RespawnWorldObjects();
	}
}

void UPRInteraction_Waypoint::NotifyPartySyncInteractionStarted(AActor* Interactor)
{
	Super::NotifyPartySyncInteractionStarted(Interactor);
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// Waypoint 활성화 시작 이벤트 전송
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_Start);
	}
}

void UPRInteraction_Waypoint::NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled)
{
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// Waypoint 상호작용 종료 이벤트 전송
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
	}
	Super::NotifyPartySyncInteractionEnded(Interactor, bCanceled);
}

void UPRInteraction_Waypoint::LockPlayerInteraction()
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	bInteractionLocked = true;
}

void UPRInteraction_Waypoint::UnlockPlayerInteraction()
{
	if (!bInteractionLocked)
	{
		return;
	}

	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	bInteractionLocked = false;
}

void UPRInteraction_Waypoint::ScheduleWaypointTravelUI()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		bWaitingActivateFadeOut = false;
		UnlockPlayerInteraction();
		return;
	}

	bWaitingActivateFadeOut = true;

	// 전원 Travel UI 진입 FadeOut 알림
	NotifyMapTransitionToAllPlayers(EPRMapTransitionType::WaypointTravelUI);

	World->GetTimerManager().ClearTimer(WaypointActivateTimerHandle);
	if (TravelUIFadeDuration <= 0.0f)
	{
		OnWaypointActivate();
		return;
	}

	// FadeOut 종료 후 호스트 UI 표시 예약
	World->GetTimerManager().SetTimer(
		WaypointActivateTimerHandle,
		this,
		&UPRInteraction_Waypoint::OnWaypointActivate,
		TravelUIFadeDuration,
		false);
}

void UPRInteraction_Waypoint::OnWaypointActivate()
{
	if (!bWaitingActivateFadeOut)
	{
		return;
	}

	bWaitingActivateFadeOut = false;
	
	// 기본 아이템 충전
	GiveStartUpItems();
	
	// GE 적용
	ApplyWaypointGameplayEffectToAllPlayers();
	
	// 호스트 UI 오픈
	bWaitingForWaypointTravelSelection = OpenWaypointTravelUI();
	if (!bWaitingForWaypointTravelSelection)
	{
		// UI 오픈 실패 후 Fade 복구
		NotifyMapTransitionToAllPlayers(EPRMapTransitionType::CancelTravel);
		UnlockPlayerInteraction();
	}
}

bool UPRInteraction_Waypoint::OpenWaypointTravelUI()
{
	APRPlayerController* HostController = FindHostPlayerController();
	if (!IsValid(HostController))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel UI open failed: host controller not found"));
		return false;
	}

	// ActiveAction 해제 이후 서버 RPC 대상 복구용 참조 등록
	HostController->SetPendingWaypointTravelInteraction(this);

	// 호스트 로컬 클라이언트 UI 열기
	HostController->ClientOpenWaypointTravelUI(ShouldShowWorldResetButton());
	return true;
}

void UPRInteraction_Waypoint::GiveStartUpItems() const
{
	for (TObjectPtr<APlayerState> Player : GetPlayerArray())
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(Player.Get()))
		{
			PS->GiveStartUpItems();
		}
	}
}

void UPRInteraction_Waypoint::ApplyWaypointGameplayEffectToAllPlayers() const
{
	if (!IsValid(WaypointGameplayEffect))
	{
		return;
	}

	for (APlayerState* PlayerState : GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(PlayerState);
		if (!IsValid(ASC))
		{
			continue;
		}

		// FadeOut 완료 후 Travel UI 표시 시점 효과 적용
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(WaypointGameplayEffect, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState))
			{
				// Waypoint GE 적용 이후 SeamlessTravel 복원 데이터 갱신
				PRPlayerState->SnapshotCurrentSaveData();
			}
		}
	}
}

void UPRInteraction_Waypoint::NotifyMapTransitionToAllPlayers(EPRMapTransitionType TransitionType) const
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
		{
			Controller = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (!IsValid(Controller))
		{
			continue;
		}

		// 로컬 맵 전환 페이드 알림
		Controller->ClientNotifyMapTransition(TravelUIFadeDuration, TransitionType);
	}
}

void UPRInteraction_Waypoint::NotifyLoadingScreenToAllPlayers(const FString& MapName) const
{
	if (MapName.IsEmpty())
	{
		return;
	}

	for (APlayerState* PlayerState : GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
		{
			Controller = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (!IsValid(Controller))
		{
			continue;
		}

		Controller->ResetAcknowledgedMapLoadingScreen();
		Controller->ClientBeginMapLoadingScreen(MapName);
	}
}

void UPRInteraction_Waypoint::StartWaypointTravelWhenLoadingScreenReady(TSoftObjectPtr<UWorld> MapAsset, FGameplayTag SpawnPointId)
{
	UWorld* World = GetWorld();
	if (MapAsset.IsNull() || !IsValid(World))
	{
		return;
	}

	PendingWaypointTravelMap = MapAsset;
	PendingWaypointTravelSpawnPointId = SpawnPointId;
	LoadingScreenReadyWaitStartSeconds = FPlatformTime::Seconds();

	World->GetTimerManager().ClearTimer(LoadingScreenReadyTimerHandle);
	if (AreLoadingScreensAcknowledged(MapAsset.GetLongPackageName()) || LoadingScreenReadyTimeout <= 0.0f)
	{
		ExecutePendingWaypointTravel();
		return;
	}

	World->GetTimerManager().SetTimer(
		LoadingScreenReadyTimerHandle,
		this,
		&UPRInteraction_Waypoint::PollWaypointLoadingScreenReady,
		FMath::Max(0.01f, LoadingScreenReadyPollInterval),
		true);
}

void UPRInteraction_Waypoint::PollWaypointLoadingScreenReady()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || PendingWaypointTravelMap.IsNull())
	{
		return;
	}

	const FString PendingMapName = PendingWaypointTravelMap.GetLongPackageName();
	const double ElapsedSeconds = FPlatformTime::Seconds() - LoadingScreenReadyWaitStartSeconds;
	if (AreLoadingScreensAcknowledged(PendingMapName) || ElapsedSeconds >= LoadingScreenReadyTimeout)
	{
		ExecutePendingWaypointTravel();
	}
}

bool UPRInteraction_Waypoint::AreLoadingScreensAcknowledged(const FString& MapName) const
{
	if (MapName.IsEmpty())
	{
		return false;
	}

	bool bFoundController = false;
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
		{
			Controller = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (!IsValid(Controller))
		{
			continue;
		}

		bFoundController = true;
		if (!Controller->HasAcknowledgedMapLoadingScreen(MapName))
		{
			return false;
		}
	}

	return bFoundController;
}

void UPRInteraction_Waypoint::ExecutePendingWaypointTravel()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(LoadingScreenReadyTimerHandle);
	}

	TSoftObjectPtr<UWorld> MapToTravel = PendingWaypointTravelMap;
	const FGameplayTag SpawnPointId = PendingWaypointTravelSpawnPointId;
	PendingWaypointTravelMap.Reset();
	PendingWaypointTravelSpawnPointId = FGameplayTag();
	LoadingScreenReadyWaitStartSeconds = 0.0;

	StartTravelToSpawnPoint(MapToTravel, SpawnPointId, 0.0f);
}

APRPlayerController* UPRInteraction_Waypoint::FindHostPlayerController() const
{
	for (APlayerState* PlayerState :GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(PlayerController) && IsValid(PlayerState->GetPawn()))
		{
			PlayerController = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (IsValid(PlayerController) && PlayerController->IsLocalController())
		{
			// 리슨 서버 호스트 컨트롤러
			return PlayerController;
		}
	}

	return nullptr;
}

bool UPRInteraction_Waypoint::ValidateWaypointTravelRequest(const FPRWaypointKey& WaypointKey, TSoftObjectPtr<UWorld>& OutMapAsset) const
{
	OutMapAsset.Reset();

	if (!bWaitingForWaypointTravelSelection)
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: UI selection is not pending"));
		return false;
	}

	if (!WaypointKey.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: invalid destination data"));
		return false;
	}

	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(WorldRegistry))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: world registry is not configured"));
		return false;
	}

	const bool bRegisteredWorld = WorldRegistry->IsWorldIdRegistered(WaypointKey.WorldId);
	const bool bRegisteredDevWorld = WorldRegistry->IsDevWorldIdRegistered(WaypointKey.WorldId);
	if (!bRegisteredWorld)
	{
		if (!bRegisteredDevWorld)
		{
			UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: world id is not registered %s"), *WaypointKey.WorldId.ToString());
			return false;
		}

		if (!UPRWorldRegistry::IsDevTravelEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: dev world travel is disabled %s"), *WaypointKey.WorldId.ToString());
			return false;
		}
	}

	if (!WorldRegistry->HasWaypoint(WaypointKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: waypoint node not found %s"), *WaypointKey.WaypointId.ToString());
		return false;
	}

	if (bRegisteredWorld)
	{
		const APRGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<APRGameStateBase>() : nullptr;
		if (!IsValid(GameState) || !GameState->IsWaypointUnlocked(WaypointKey))
		{
			UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: waypoint is locked %s"), *WaypointKey.WaypointId.ToString());
			return false;
		}
	}

	if (!WorldRegistry->ResolveMapAsset(WaypointKey.WorldId, OutMapAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: target map is null %s"), *WaypointKey.WorldId.ToString());
		return false;
	}

	return !OutMapAsset.IsNull();
}

const UPRWorldRegistry* UPRInteraction_Waypoint::GetWorldRegistry() const
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(Settings))
	{
		return nullptr;
	}

	return Settings->GetWorldRegistrySync();
}

bool UPRInteraction_Waypoint::ResolveInteractedWaypointKey(FPRWaypointKey& OutWaypointKey) const
{
	OutWaypointKey = FPRWaypointKey();

	UWorld* World = GetWorld();
	const APRWaypointActor* WaypointActor = Cast<APRWaypointActor>(GetOwner());
	const UPRWorldRegistry* WorldRegistry = GetWorldRegistry();
	if (!IsValid(World) || !IsValid(WaypointActor) || !IsValid(WorldRegistry))
	{
		UE_LOG(LogTemp, Warning, TEXT("Waypoint key resolve failed: invalid owner or registry"));
		return false;
	}

	FGameplayTag CurrentWorldId;
	if (!WorldRegistry->FindWorldIdByMap(World, CurrentWorldId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Waypoint key resolve failed: current world is not registered"));
		return false;
	}

	OutWaypointKey.WorldId = CurrentWorldId;
	OutWaypointKey.WaypointId = WaypointActor->GetSpawnPointId();
	return OutWaypointKey.IsValid();
}

void UPRInteraction_Waypoint::BroadcastWaypointCancelEventToAllPlayers() const
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		// 상호작용 어빌리티 취소
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
		ASC->CancelAbilities(&CancelTags);

		// Waypoint 종료 이벤트 전파
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
	}
}
