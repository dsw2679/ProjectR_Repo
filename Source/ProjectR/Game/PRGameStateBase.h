// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Game State 기본 구조 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"
#include "PRGameTypes.h"
#include "ProjectR/UI/WorldMarker/PRWorldMarkerTypes.h"
#include "PRGameStateBase.generated.h"

class APRPlayerCharacter;
class APlayerState;

// 현재 PlayerArray 기준 플레이어 수 변경 이벤트
DECLARE_MULTICAST_DELEGATE(FPRPlayerCountChangedSignature);

// Waypoint 활성화 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaypointActivatedSignature, FPRWaypointKey, WaypointKey);

// 마지막 방문 Waypoint 변경 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastVisitedWaypointChangedSignature, FPRWaypointKey, WaypointKey);

// 보스 처치 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossDefeatedSignature, FName, BossId);

// 월드 진행 상태의 복제 컨테이너. 서버가 권위를 가지며 모든 클라이언트에 동일 상태로 복제
UCLASS()
class PROJECTR_API APRGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ AGameStateBase Interface ~*/
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

public:
	// 마지막 방문 Waypoint 조회
	FPRWaypointKey GetLastVisitedWaypoint() const { return LastVisitedWaypoint; }

	// 마지막 활성화 Waypoint 조회
	FPRWaypointKey GetLastActivatedWaypoint() const { return LastActivatedWaypoint; }

	// 저장된 실제 스폰 위치 조회
	FPRSpawnPointKey GetSavedSpawnPoint() const { return SavedSpawnPoint; }

	// 활성화된 Waypoint 목록 조회
	const TArray<FPRUnlockedWaypointEntry>& GetUnlockedWaypoints() const { return UnlockedWaypoints; }

	// 지정 Waypoint 활성화 여부 조회
	bool IsWaypointUnlocked(const FPRWaypointKey& WaypointKey) const;

	// 지정 월드에 활성화된 Waypoint 존재 여부 조회
	bool HasUnlockedWaypointInWorld(FGameplayTag WorldId) const;

	// 보스 처치 여부 조회
	bool IsBossDefeated(FName BossId) const;

	// 현재 세션에 등록된 PlayerState의 Pawn을 PRPlayerCharacter로 캐스팅해 수집. 캐스팅 실패나 미possess 상태는 제외
	TArray<APRPlayerCharacter*> GetPlayerCharacters() const;

	// 현재 PlayerArray 기준 플레이어 수 조회
	int32 GetPlayerCount() const;

	// 서버 전용. 월드 세이브 스냅샷을 GameState에 주입. 호스트 맵 로드 직후 GameMode가 호출
	void InitializeFromWorldSave(const FPRWorldSaveData& WorldSave);

	// 현재 월드 진행 상태 저장 데이터
	FPRWorldSaveData MakeWorldSaveData() const;

	// 서버 전용. Waypoint 해금 반영
	void ActivateWaypoint(const FPRWaypointKey& WaypointKey);

	// 서버 전용. Waypoint 해금 목록 추가
	void UnlockWaypoint(const FPRWaypointKey& WaypointKey);

	// 서버 전용. 실제 Travel 확정 목적지를 마지막 방문 Waypoint로 기록
	void VisitWaypoint(const FPRWaypointKey& WaypointKey);

	// 서버 전용. 실제 플레이어 소환에 사용한 SpawnPoint 기록
	void RecordSpawnPoint(const FPRSpawnPointKey& SpawnPointKey);

	// 서버 전용. 보스 처치 반영
	void MarkBossDefeated(FName BossId);

	// 서버 전용. 웨이포인트·보스 처치 등 월드 진행 상태 초기화
	void ResetWorldProgress();

	// 서버 전용. 월드 마커 생성 요청 처리
	void ServerSubmitWorldMarker(const FPRWorldMarkerRequest& Request);

protected:
	// 현재 GameState PlayerArray 기준 플레이어 수 변경 이벤트 갱신
	void RefreshPlayerCount();

	// LastVisitedWaypoint 복제 콜백. 클라이언트에서 Travel UI 갱신 트리거
	UFUNCTION()
	void OnRep_LastVisitedWaypoint(FPRWaypointKey OldLastVisitedWaypoint);

	// 월드 마커 이벤트 전파
	UFUNCTION(NetMulticast, Reliable)
	void MulticastWorldMarkerEvent(const FPRWorldMarkerEventPayload& Payload);

	// 플레이어별 마커 개수 제한 적용
	void EnforceMarkerLimit(APlayerState* SourcePlayerState);

	// 월드 마커 제거
	void RemoveWorldMarker(FGuid MarkerId);

public:
	// Waypoint 활성화 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnWaypointActivatedSignature OnWaypointActivated;

	// 마지막 방문 Waypoint 변경 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnLastVisitedWaypointChangedSignature OnLastVisitedWaypointChanged;

	// 보스 처치 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnBossDefeatedSignature OnBossDefeated;

	// 현재 PlayerArray 기준 플레이어 수 변경 이벤트
	FPRPlayerCountChangedSignature OnPlayerCountChanged;

protected:
	// 플레이어별 최대 활성 핑 개수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|WorldMarker", meta = (ClampMin = "1"))
	int32 MaxActiveMarkersPerPlayer = 1;

	// 마지막 방문 Waypoint. 변경 시 OnRep으로 Travel UI 갱신
	UPROPERTY(ReplicatedUsing = OnRep_LastVisitedWaypoint)
	FPRWaypointKey LastVisitedWaypoint;

	// 마지막 활성화 Waypoint. 전멸 리스폰 기준 지점
	UPROPERTY(Replicated)
	FPRWaypointKey LastActivatedWaypoint;

	// 저장 시 시작 메뉴 이어하기 위치로 사용할 실제 스폰 위치
	UPROPERTY(Replicated)
	FPRSpawnPointKey SavedSpawnPoint;

	// 이번 세션에서 해금된 Waypoint 집합
	UPROPERTY(Replicated)
	TArray<FPRUnlockedWaypointEntry> UnlockedWaypoints;

	// 처치된 보스 ID. 재도전 트리거·컷신 분기용
	UPROPERTY(Replicated)
	TArray<FName> DefeatedBosses;

	// 호스트 기준 월드 세이브 버전. 디버그·불일치 진단용
	UPROPERTY(Replicated)
	EPRSaveVersion WorldSaveVersion = EPRSaveVersion::V1;

	// 플레이어별 활성 마커 목록
	TMap<TWeakObjectPtr<APlayerState>, FPRActiveWorldMarkerList> ActiveMarkerIdsByPlayer;

	// 활성 마커 데이터
	TMap<FGuid, FPRWorldMarkerData> ActiveWorldMarkers;

	// 마커 만료 타이머
	TMap<FGuid, FTimerHandle> WorldMarkerExpireTimers;

	// 마지막으로 이벤트 발행한 플레이어 수
	int32 LastBroadcastPlayerCount = 0;
};
