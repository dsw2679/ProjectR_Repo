// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (상호작용 시 로딩화면 프리웜 연동)
// Author: 배유찬 (웨이포인트 활성화, 리스폰 지점 등록 및 트래블 UI 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRInteraction_MapTravelBase.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "TimerManager.h"
#include "PRInteraction_Waypoint.generated.h"

class APRPlayerController;
class UGameplayEffect;
class UPRWorldRegistry;
enum class EPRMapTransitionType : uint8;

// 호스트 UI 선택 결과로 목적지가 정해지는 웨이포인트 상호작용
UCLASS()
class PROJECTR_API UPRInteraction_Waypoint : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	UPRInteraction_Waypoint();

	// 호스트 선택 웨이포인트 목적지 이동 요청
	void RequestWaypointTravel(APRPlayerController* RequestingController, const FPRWaypointKey& WaypointKey);

	// 호스트 웨이포인트 Travel UI 닫힘 처리
	void CancelWaypointTravel(APRPlayerController* RequestingController);

	// 목적지 선택 UI 입력 대기 여부 반환
	bool IsWaitingForWaypointTravelSelection() const { return bWaitingForWaypointTravelSelection; }

	// 이 Waypoint Travel UI의 월드 진행도 리셋 버튼 표시 여부 반환
	bool ShouldShowWorldResetButton() const { return bShowWorldResetButton; }

protected:
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;

	/*~ UPRInteraction_PartySyncBase Interface ~*/
	// Waypoint 파티 동기화 조건 충족 시 호스트 Travel UI 표시
	virtual void HandlePartySyncConditionMet() override;

	// Travel UI 대기 또는 맵 이동 중 중복 파티 동기화 차단 여부
	virtual bool IsPartySyncActionLocked() const override;

	// Waypoint 상호작용 시작 이벤트 전송
	virtual void NotifyPartySyncInteractionStarted(AActor* Interactor) override;

	// Waypoint 상호작용 종료 이벤트 전송
	virtual void NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled) override;

	// 전체 플레이어 상호작용 잠금
	void LockPlayerInteraction();

	// 전체 플레이어 상호작용 잠금 해제
	void UnlockPlayerInteraction();

private:
	// Waypoint 해금 상태 갱신
	void RecordWaypointActivation();

	// Waypoint 활성화에 따른 월드 오브젝트 복구
	void RespawnWorldObjects();

	// 전원 FadeOut 이후 호스트 Travel UI 표시 예약
	void ScheduleWaypointTravelUI();

	// FadeOut 종료 후 호스트 Travel UI 표시
	void OnWaypointActivate();

	// 호스트 Travel UI 열기
	bool OpenWaypointTravelUI();

	// 기본 아이템 충전	
	void GiveStartUpItems() const; 
	
	// FadeOut 완료 후 모든 플레이어 Waypoint 효과 적용
	void ApplyWaypointGameplayEffectToAllPlayers() const;

	// 모든 플레이어 맵 전환 연출 알림
	void NotifyMapTransitionToAllPlayers(EPRMapTransitionType TransitionType) const;

	// 모든 플레이어 로딩 화면 선표시 요청
	void NotifyLoadingScreenToAllPlayers(const FString& MapName) const;

	// 로딩 오버레이 준비 후 목적지 이동 예약
	void StartWaypointTravelWhenLoadingScreenReady(TSoftObjectPtr<UWorld> MapAsset, FGameplayTag SpawnPointId);

	// 로딩 오버레이 준비 대기 갱신
	void PollWaypointLoadingScreenReady();

	// 모든 플레이어 로딩 오버레이 표시 완료 여부
	bool AreLoadingScreensAcknowledged(const FString& MapName) const;

	// 대기 중인 목적지 이동 실행
	void ExecutePendingWaypointTravel();

	// 서버 월드에서 호스트 컨트롤러 조회
	APRPlayerController* FindHostPlayerController() const;

	// 목적지 WorldId와 웨이포인트 ID 검증
	bool ValidateWaypointTravelRequest(const FPRWaypointKey& WaypointKey, TSoftObjectPtr<UWorld>& OutMapAsset) const;

	// 프로젝트 월드 Registry 조회
	const UPRWorldRegistry* GetWorldRegistry() const;

	// 현재 상호작용 중인 Waypoint 키 생성
	bool ResolveInteractedWaypointKey(FPRWaypointKey& OutWaypointKey) const;

	// 모든 플레이어 Waypoint 취소 처리
	void BroadcastWaypointCancelEventToAllPlayers() const;

private:
	// 호스트 UI 목적지 선택 대기 상태
	bool bWaitingForWaypointTravelSelection = false;

	// 호스트 Travel UI 표시 전 FadeOut 대기 상태
	bool bWaitingActivateFadeOut = false;

	// 전체 플레이어 상호작용 차단 태그 적용 상태
	bool bInteractionLocked = false;

	// 호스트 Travel UI 표시 예약 타이머
	FTimerHandle WaypointActivateTimerHandle;

	// 로딩 오버레이 준비 확인 타이머
	FTimerHandle LoadingScreenReadyTimerHandle;

	// 로딩 오버레이 준비 대기 시작 시간
	double LoadingScreenReadyWaitStartSeconds = 0.0;

	// 로딩 오버레이 준비 이후 이동할 맵
	TSoftObjectPtr<UWorld> PendingWaypointTravelMap;

	// 로딩 오버레이 준비 이후 이동할 SpawnPoint 태그
	FGameplayTag PendingWaypointTravelSpawnPointId;

	// Waypoint 이동 직전 자원 충전 또는 회복 GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> WaypointGameplayEffect;

	// 호스트 Travel UI 표시 전 FadeOut 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TravelUIFadeDuration = 1.6f;

	// 로딩 오버레이 표시 ack 최대 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LoadingScreenReadyTimeout = 1.25f;

	// 로딩 오버레이 표시 ack 확인 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float LoadingScreenReadyPollInterval = 0.05f;

	// 이 Waypoint에서 열린 Travel UI의 월드 진행도 리셋 버튼 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true"))
	bool bShowWorldResetButton = false;
};
