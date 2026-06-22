// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (세션 진입 시 로딩화면 연동 구현)
// Author: 배유찬 (온라인 세션 생성/검색/참여 및 전멸 리셋 제어 구현)
#include "PRSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Regex.h"
#include "Misc/PackageName.h"
#include "ProjectR/System/PRLoadingScreenSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRSession, Log, All);

namespace
{
	const FName ProjectRSessionName(TEXT("ProjectRSession"));
	constexpr int32 LocalUserNum = 0;
	constexpr float LoadingOverlayTravelDelay = 1.0f;

	FString NormalizeMapPackageName(const FString& MapPackageName)
	{
		// PIE 접두사와 오브젝트 이름 접미사를 제거한 비교용 패키지 이름
		FString NormalizedName = UWorld::RemovePIEPrefix(MapPackageName);
		int32 ObjectNameSeparatorIndex = INDEX_NONE;
		if (NormalizedName.FindChar(TEXT('.'), ObjectNameSeparatorIndex))
		{
			NormalizedName.LeftInline(ObjectNameSeparatorIndex);
		}

		return NormalizedName;
	}

	bool IsSameMapForRestart(const UWorld* World, const FString& TargetPackageName)
	{
		if (!IsValid(World) || TargetPackageName.IsEmpty())
		{
			return false;
		}

		const FString NormalizedTargetPackageName = NormalizeMapPackageName(TargetPackageName);
		const FString NormalizedCurrentPackageName = NormalizeMapPackageName(World->GetOutermost()->GetName());
		if (NormalizedCurrentPackageName == NormalizedTargetPackageName)
		{
			return true;
		}

		const FString NormalizedCurrentMapName = NormalizeMapPackageName(World->GetMapName());
		if (NormalizedCurrentMapName == NormalizedTargetPackageName)
		{
			return true;
		}

		// PIE와 Restart URL 경로에서 짧은 맵 이름만 남는 경우를 위한 보조 판정
		const FString CurrentShortName = FPackageName::GetShortName(NormalizedCurrentPackageName);
		const FString CurrentMapShortName = FPackageName::GetShortName(NormalizedCurrentMapName);
		const FString TargetShortName = FPackageName::GetShortName(NormalizedTargetPackageName);
		return CurrentShortName == TargetShortName || CurrentMapShortName == TargetShortName;
	}
}

// ===== 초기화 ===== 

void UPRSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 네트워크 실패 콜백 등록. ClientTravel 실패·연결 끊김을 여기서 포착
	if (GEngine)
	{
		GEngine->NetworkFailureEvent.AddUObject(this, &UPRSessionSubsystem::HandleNetworkFailure);
	}
}

void UPRSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	if (GEngine)
	{
		GEngine->NetworkFailureEvent.RemoveAll(this);
	}

	Super::Deinitialize();
}

// ===== 상태 전이 ===== 

void UPRSessionSubsystem::SetState(EPRSessionState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	OnSessionStateChanged.Broadcast(CurrentState);
}

void UPRSessionSubsystem::RunTravelAfterLoadingOverlay(FName TravelReason, const FString& MapName, TFunction<void()> TravelAction)
{
	if (!TravelAction)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
		{
			if (MapName.IsEmpty())
			{
				LoadingScreen->BeginTravel(TravelReason);
			}
			else
			{
				LoadingScreen->BeginTravelToMap(TravelReason, MapName);
			}
		}
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || LoadingOverlayTravelDelay <= 0.0f)
	{
		TravelAction();
		return;
	}

	World->GetTimerManager().ClearTimer(LoadingOverlayTravelTimerHandle);
	FTimerDelegate TravelDelegate = FTimerDelegate::CreateWeakLambda(this, [TravelAction = MoveTemp(TravelAction)]() mutable
	{
		TravelAction();
	});
	World->GetTimerManager().SetTimer(LoadingOverlayTravelTimerHandle, TravelDelegate, LoadingOverlayTravelDelay, false);
}

// ===== Host 경로 ===== 

void UPRSessionSubsystem::StartHost(const FPRHostSessionParams& Params)
{
	if (Params.MapName.IsNone())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapName is empty"));
		return;
	}

	PendingHostParams = Params;
	SetState(EPRSessionState::Hosting);

	IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Online session interface invalid"));
		SetState(EPRSessionState::None);
		return;
	}

	if (SessionInterface->GetNamedSession(ProjectRSessionName))
	{
		// 기존 세션 제거 후 동일 파라미터로 재생성
		bCreateSessionAfterDestroy = true;
		DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
		if (!SessionInterface->DestroySession(ProjectRSessionName))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			bCreateSessionAfterDestroy = false;
			OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Destroy existing session failed"));
			SetState(EPRSessionState::None);
		}
		return;
	}

	PendingHostSettings = MakeShared<FOnlineSessionSettings>();
	PendingHostSettings->NumPublicConnections = FMath::Max(Params.MaxPlayers, 1);
	PendingHostSettings->NumPrivateConnections = 0;
	PendingHostSettings->bIsLANMatch = true;
	PendingHostSettings->bShouldAdvertise = !Params.bPrivate;
	PendingHostSettings->bAllowJoinInProgress = true;
	PendingHostSettings->bAllowJoinViaPresence = false;
	PendingHostSettings->bUsesPresence = false;
	PendingHostSettings->Set(SETTING_MAPNAME, Params.MapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	UE_LOG(LogPRSession, Log, TEXT("[StartHost] CreateSession Map=%s, MaxPlayers=%d"),
		*Params.MapName.ToString(), Params.MaxPlayers);

	if (!SessionInterface->CreateSession(LocalUserNum, ProjectRSessionName, *PendingHostSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		PendingHostSettings.Reset();
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("CreateSession request failed"));
		SetState(EPRSessionState::None);
	}
}

void UPRSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	PendingHostSettings.Reset();

	if (!bWasSuccessful)
	{
		UE_LOG(LogPRSession, Warning, TEXT("[HandleCreateSessionComplete] Failed Session=%s"), *SessionName.ToString());
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("CreateSession failed"));
		SetState(EPRSessionState::None);
		return;
	}

	// listen 옵션으로 리슨 서버 개시. SessionPort 명시로 PIE/Standalone 포트 통일
	const FString Options = FString::Printf(TEXT("listen?MaxPlayers=%d?Port=%d"), PendingHostParams.MaxPlayers, SessionPort);
	UE_LOG(LogPRSession, Log, TEXT("[HandleCreateSessionComplete] OpenLevel Map=%s, Options=%s"), *PendingHostParams.MapName.ToString(), *Options);
	const FName MapName = PendingHostParams.MapName;
	RunTravelAfterLoadingOverlay(TEXT("StartHost"), MapName.ToString(), [this, MapName, Options]()
	{
		UGameplayStatics::OpenLevel(this, MapName, true, Options);
	});

	// OpenLevel은 비동기이므로 Hosted 전이는 맵 로드 완료 시 GameMode에서 확정한다
	SetState(EPRSessionState::Hosted);
}

// ===== Join 경로 ===== 

bool UPRSessionSubsystem::ValidateAddress(const FString& Address) const
{
	// IPv4(:Port)? 포맷만 허용. 호스트네임은 현재 단계에서 미지원
	const FRegexPattern Pattern(TEXT("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(:\\d{1,5})?$"));
	FRegexMatcher Matcher(Pattern, Address);
	return Matcher.FindNext();
}

void UPRSessionSubsystem::StartJoin(const FPRJoinSessionParams& Params)
{
	const FString TrimmedAddress = Params.Address.TrimStartAndEnd();
	if (!TrimmedAddress.IsEmpty())
	{
		StartDirectJoin(TrimmedAddress);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("GameInstance invalid"));
		return;
	}

	UWorld* World = GetWorld();
	const ENetMode NetMode = IsValid(World) ? World->GetNetMode() : NM_Standalone;
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogPRSession, Warning, TEXT("[StartJoin] 호스트 프로세스에서 검색 참가 시도 차단 NetMode=%d"), static_cast<int32>(NetMode));
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Cannot join from a hosting instance"));
		return;
	}

	IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Online session interface invalid"));
		return;
	}

	PendingSessionSearch = MakeShared<FOnlineSessionSearch>();
	PendingSessionSearch->bIsLanQuery = true;
	PendingSessionSearch->MaxSearchResults = 20;
	PendingSessionSearch->PingBucketSize = 50;

	SetState(EPRSessionState::Finding);

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	UE_LOG(LogPRSession, Log, TEXT("[StartJoin] FindSessions"));
	if (!SessionInterface->FindSessions(LocalUserNum, PendingSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		PendingSessionSearch.Reset();
		OnSessionFailed.Broadcast(EPRSessionFailReason::SessionSearchFailed, TEXT("FindSessions request failed"));
		SetState(EPRSessionState::None);
	}
}

void UPRSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	const int32 ResultCount = PendingSessionSearch.IsValid() ? PendingSessionSearch->SearchResults.Num() : 0;
	UE_LOG(LogPRSession, Log, TEXT("[HandleFindSessionsComplete] Success=%d, Results=%d"), bWasSuccessful ? 1 : 0, ResultCount);

	if (!bWasSuccessful)
	{
		PendingSessionSearch.Reset();
		OnSessionFailed.Broadcast(EPRSessionFailReason::SessionSearchFailed, TEXT("FindSessions failed"));
		SetState(EPRSessionState::None);
		return;
	}

	if (ResultCount <= 0)
	{
		PendingSessionSearch.Reset();
		OnSessionFailed.Broadcast(EPRSessionFailReason::NoSessionFound, TEXT("No session found"));
		SetState(EPRSessionState::None);
		return;
	}

	if (!JoinFirstSearchResult())
	{
		PendingSessionSearch.Reset();
		OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, TEXT("JoinSession request failed"));
		SetState(EPRSessionState::None);
	}
}

bool UPRSessionSubsystem::JoinFirstSearchResult()
{
	IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
	if (!SessionInterface.IsValid() || !PendingSessionSearch.IsValid() || PendingSessionSearch->SearchResults.Num() <= 0)
	{
		return false;
	}

	SetState(EPRSessionState::Joining);

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	const FOnlineSessionSearchResult& SearchResult = PendingSessionSearch->SearchResults[0];
	if (!SessionInterface->JoinSession(LocalUserNum, ProjectRSessionName, SearchResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		return false;
	}

	return true;
}

void UPRSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	PendingSessionSearch.Reset();

	if (Result != EOnJoinSessionCompleteResult::Success || !SessionInterface.IsValid())
	{
		UE_LOG(LogPRSession, Warning, TEXT("[HandleJoinSessionComplete] Failed Session=%s, Result=%d"),
			*SessionName.ToString(), static_cast<int32>(Result));
		OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, TEXT("JoinSession failed"));
		SetState(EPRSessionState::None);
		return;
	}

	FString TravelAddress;
	if (!SessionInterface->GetResolvedConnectString(ProjectRSessionName, TravelAddress) || TravelAddress.IsEmpty())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, TEXT("Resolved connect string empty"));
		SetState(EPRSessionState::None);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PC = IsValid(GameInstance) ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (!IsValid(PC))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("PlayerController invalid"));
		SetState(EPRSessionState::None);
		return;
	}

	UE_LOG(LogPRSession, Log, TEXT("[HandleJoinSessionComplete] ClientTravel Address=%s PC=%s"),
		*TravelAddress, *PC->GetName());

	// OSS가 해석한 접속 문자열로 이동. 실제 Joined 전이는 새 맵에서 확정
	RunTravelAfterLoadingOverlay(TEXT("StartJoin"), FString(), [PC, TravelAddress]()
	{
		if (IsValid(PC))
		{
			PC->ClientTravel(TravelAddress, TRAVEL_Absolute);
		}
	});
}

void UPRSessionSubsystem::StartDirectJoin(const FString& Address)
{
	if (!ValidateAddress(Address))
	{
		UE_LOG(LogPRSession, Warning, TEXT("[StartDirectJoin] InvalidAddress=%s"), *Address);
		OnSessionFailed.Broadcast(EPRSessionFailReason::InvalidAddress, Address);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("GameInstance invalid"));
		return;
	}

	APlayerController* PC = GameInstance->GetFirstLocalPlayerController();
	if (!IsValid(PC))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("PlayerController invalid"));
		return;
	}

	// 같은 프로세스가 이미 리슨 서버이거나 호스트 상태이면 자기 자신으로 ClientTravel 불가
	UWorld* World = GetWorld();
	const ENetMode NetMode = IsValid(World) ? World->GetNetMode() : NM_Standalone;
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogPRSession, Warning, TEXT("[StartJoin] 호스트 프로세스에서 Join 시도 차단 NetMode=%d"), static_cast<int32>(NetMode));
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Cannot join from a hosting instance"));
		return;
	}

	// 메뉴에서 포트 미입력 시 SessionPort로 보정. 호스트와 동일 포트 강제
	FString TravelAddress = Address;
	if (!TravelAddress.Contains(TEXT(":")))
	{
		TravelAddress += FString::Printf(TEXT(":%d"), SessionPort);
	}

	UE_LOG(LogPRSession, Log, TEXT("[StartDirectJoin] ClientTravel Address=%s NetMode=%d PC=%s"),
		*TravelAddress, static_cast<int32>(NetMode), *PC->GetName());

	SetState(EPRSessionState::Joining);

	// 접속 시도. 실패는 HandleNetworkFailure로 통지됨
	// ClientTravel은 비동기. 실제 Joined 전이는 새 맵에서 PostLogin/WelcomeReceived 시점에 확정해야 함
	RunTravelAfterLoadingOverlay(TEXT("StartJoin"), FString(), [PC, TravelAddress]()
	{
		if (IsValid(PC))
		{
			PC->ClientTravel(TravelAddress, TRAVEL_Absolute);
		}
	});
}

// ===== ServerTravel =====

bool UPRSessionSubsystem::ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute)
{
	if (MapAsset.IsNull())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapAsset is null"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("World invalid"));
		return false;
	}

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode != NM_ListenServer && NetMode != NM_DedicatedServer && NetMode != NM_Standalone)
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("ServerTravel requires server authority"));
		return false;
	}

	if (World->IsInSeamlessTravel())
	{
		// 이미 진행 중인 SeamlessTravel에 대한 중복 요청 차단
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("ServerTravel already in progress"));
		return false;
	}

	// 소프트 참조의 LongPackageName을 ServerTravel URL로 사용 (예: /Game/.../L_MyMap)
	const FString PackageName = MapAsset.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapAsset package name empty"));
		return false;
	}

	const FString CurrentPackageName = NormalizeMapPackageName(World->GetOutermost()->GetName());
	if (IsSameMapForRestart(World, PackageName))
	{
		// 현재 맵 재진입은 플레이어 리스폰과 동일한 Restart URL 사용
		RunTravelAfterLoadingOverlay(TEXT("ServerTravelRestart"), CurrentPackageName, [this, CurrentPackageName, PackageName]()
		{
			UWorld* TravelWorld = GetWorld();
			if (!IsValid(TravelWorld) || !TravelWorld->ServerTravel(TEXT("?Restart"), false))
			{
				UE_LOG(LogPRSession, Warning, TEXT("[ServerTravelToMap] Restart failed. Current=%s, Target=%s"),
					*CurrentPackageName,
					*PackageName);
				OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("?Restart"));
			}
		});

		return true;
	}

	const FString TravelURL = PackageName;
	const bool bResolvedAbsolute = bAbsolute;

	// 다른 맵 이동은 소프트 참조 패키지 이름으로 ServerTravel 실행
	RunTravelAfterLoadingOverlay(TEXT("ServerTravelToMap"), PackageName, [this, TravelURL, CurrentPackageName, bResolvedAbsolute]()
	{
		UWorld* TravelWorld = GetWorld();
		if (!IsValid(TravelWorld) || !TravelWorld->ServerTravel(TravelURL, bResolvedAbsolute))
		{
			UE_LOG(LogPRSession, Warning, TEXT("[ServerTravelToMap] ServerTravel failed. URL=%s, Current=%s"),
				*TravelURL,
				*CurrentPackageName);
			OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TravelURL);
		}
	});

	return true;
}

// ===== 종료 =====

void UPRSessionSubsystem::EndSession()
{
	SetState(EPRSessionState::Leaving);

	IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(ProjectRSessionName))
	{
		DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
		if (SessionInterface->DestroySession(ProjectRSessionName))
		{
			return;
		}

		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	TravelToMenuMap();
	SetState(EPRSessionState::None);
}

void UPRSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	UE_LOG(LogPRSession, Log, TEXT("[HandleDestroySessionComplete] Session=%s, Success=%d"),
		*SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (bCreateSessionAfterDestroy)
	{
		// 기존 세션 제거 후 Host 요청 재개
		bCreateSessionAfterDestroy = false;
		StartHost(PendingHostParams);
		return;
	}

	TravelToMenuMap();
	SetState(EPRSessionState::None);
}

void UPRSessionSubsystem::TravelToMenuMap()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 호스트는 ServerTravel(모든 클라 동반 이동), 게스트는 ClientTravel
	if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
	{
		RunTravelAfterLoadingOverlay(TEXT("EndSessionHost"), MenuMapName.ToString(), [this]()
		{
			UWorld* TravelWorld = GetWorld();
			if (IsValid(TravelWorld))
			{
				TravelWorld->ServerTravel(MenuMapName.ToString(), true);
			}
		});
	}
	else
	{
		UGameInstance* GameInstance = GetGameInstance();
		if (!IsValid(GameInstance))
		{
			return;
		}

		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			RunTravelAfterLoadingOverlay(TEXT("EndSessionClient"), MenuMapName.ToString(), [this, PC]()
			{
				if (IsValid(PC))
				{
					PC->ClientTravel(MenuMapName.ToString(), TRAVEL_Absolute);
				}
			});
		}
	}
}

// ===== 네트워크 실패 ===== 

void UPRSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogPRSession, Warning, TEXT("[HandleNetworkFailure] FailureType=%d, Error=%s"),
		static_cast<int32>(FailureType), *ErrorString);

	OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, ErrorString);
	SetState(EPRSessionState::None);
}

IOnlineSessionPtr UPRSessionSubsystem::GetOnlineSessionInterface() const
{
	// PIE의 월드별 OnlineSubsystem 컨텍스트를 반영한 세션 인터페이스 조회
	return Online::GetSessionInterface(GetWorld());
}
