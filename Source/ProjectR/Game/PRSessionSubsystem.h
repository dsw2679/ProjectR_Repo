// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (세션 진입 시 로딩화면 연동 구현)
// Author: 배유찬 (온라인 세션 생성/검색/참여 및 전멸 리셋 제어 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/NetworkDelegates.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "TimerManager.h"
#include "PRGameTypes.h"
#include "PRSessionSubsystem.generated.h"

// 세션 상태 변화 통지. UI가 Bind하여 로비/로딩 화면을 갱신
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChangedSignature, EPRSessionState, NewState);

// 세션 실패 통지. Reason은 사용자 표시용, Detail은 로그·디버그용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionFailedSignature, EPRSessionFailReason, Reason, FString, Detail);

// 세션 Host/Join의 플랫폼별 구현을 격리. 상위는 동일 인터페이스로 호출
// 현재 구현: OnlineSubsystem Null 기반 LAN 세션 검색과 참가
UCLASS()
class PROJECTR_API UPRSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/*~ UGameInstanceSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// 호스트 개시. 세션 등록 성공 시 리슨 서버로 OpenLevel, 실패 시 OnSessionFailed 발행
	UFUNCTION(BlueprintCallable)
	void StartHost(const FPRHostSessionParams& Params);

	// 참가 개시. 주소가 비어 있으면 OSS 세션 검색 후 첫 번째 세션 참가
	UFUNCTION(BlueprintCallable)
	void StartJoin(const FPRJoinSessionParams& Params);

	// 현재 세션 종료 처리. 호스트/게스트 공통 진입점
	void EndSession();

	// 소프트 참조된 맵으로 ServerTravel을 수행한다. 호스트(리슨/데디케이티드/스탠드얼론)에서만 동작
	// bAbsolute=true 이면 URL 옵션을 보존하지 않고 새 맵으로 절대 이동
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	bool ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute = true);

	// 현재 세션 상태 조회
	EPRSessionState GetState() const { return CurrentState; }

protected:
	// 상태 전이 + 델리게이트 발행
	void SetState(EPRSessionState NewState);

	// 네트워크 레이어 실패 핸들러. ClientTravel 실패·연결 끊김 포착
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// 주소 포맷 검증. IPv4(:Port)? 만 허용
	bool ValidateAddress(const FString& Address) const;

	// OnlineSubsystem 세션 인터페이스 조회
	IOnlineSessionPtr GetOnlineSessionInterface() const;

	// OSS 세션 생성 완료 처리
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	// OSS 세션 검색 완료 처리
	void HandleFindSessionsComplete(bool bWasSuccessful);

	// OSS 세션 참가 완료 처리
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// OSS 세션 종료 완료 처리
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// 검색된 세션 결과 참가 요청
	bool JoinFirstSearchResult();

	// IP 직접 접속 호환 경로
	void StartDirectJoin(const FString& Address);

	// 메뉴 맵 복귀 실행
	void TravelToMenuMap();

	// 로딩 오버레이 표시 이후 Travel 실행 예약
	void RunTravelAfterLoadingOverlay(FName TravelReason, const FString& MapName, TFunction<void()> TravelAction);

public:
	// 세션 상태 변화 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnSessionStateChangedSignature OnSessionStateChanged;

	// 세션 실패 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnSessionFailedSignature OnSessionFailed;

protected:
	// 현재 세션 진행 단계
	EPRSessionState CurrentState = EPRSessionState::None;

	// 호스트 개시 시 보관된 파라미터. 맵 전이 이후 로직에서 참조
	FPRHostSessionParams PendingHostParams;

	// 세션 생성 설정 보관. CreateSession 비동기 완료까지 수명 유지
	TSharedPtr<FOnlineSessionSettings> PendingHostSettings;

	// 세션 검색 결과 보관. JoinSession 비동기 완료까지 수명 유지
	TSharedPtr<FOnlineSessionSearch> PendingSessionSearch;

	// 세션 생성 완료 델리게이트 핸들
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	// 세션 검색 완료 델리게이트 핸들
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	// 세션 참가 완료 델리게이트 핸들
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	// 세션 종료 완료 델리게이트 핸들
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	// 기존 세션 제거 후 호스트 재시도 여부
	bool bCreateSessionAfterDestroy = false;

	// 로딩 오버레이 표시 후 Travel 실행 타이머
	FTimerHandle LoadingOverlayTravelTimerHandle;

	// 메뉴 복귀 시 사용할 기본 메뉴 맵
	FName MenuMapName = TEXT("/Game/1_Maps/L_Menu");

	// 호스트 리슨 포트와 클라 접속 포트 통일값. 프로젝트 단일 진입점 고정
	static constexpr int32 SessionPort = 7777;
};
