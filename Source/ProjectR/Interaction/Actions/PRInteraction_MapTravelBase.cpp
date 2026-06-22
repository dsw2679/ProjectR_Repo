// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Map Travel 기본 구조 상호작용 액션 실행 로직 구현)
#include "PRInteraction_MapTravelBase.h"

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/World/PRSpawnPointTags.h"

namespace
{
	// 플레이어 상태 기준 PR 플레이어 컨트롤러 조회
	APRPlayerController* ResolveMapTravelPlayerController(const APlayerState* PlayerState)
	{
		if (!IsValid(PlayerState))
		{
			return nullptr;
		}

		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
		{
			Controller = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		return Controller;
	}
}

UPRInteraction_MapTravelBase::UPRInteraction_MapTravelBase()
{
	// 맵 이동 실행 기본 설정
}

bool UPRInteraction_MapTravelBase::IsPartySyncActionLocked() const
{
	return bTravelInProgress;
}

void UPRInteraction_MapTravelBase::StartTravelToSpawnPoint(TSoftObjectPtr<UWorld> MapToTravel, FGameplayTag SpawnPointId, float TransitionDelay)
{
	UWorld* World = GetWorld();
	// 목적지 데이터는 EntranceGate 또는 Waypoint 같은 하위 클래스 소유
	if (bTravelInProgress || MapToTravel.IsNull() || !IsValid(World))
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(TravelDelayTimerHandle))
	{
		return;
	}

	const float ResolvedTransitionDelay = FMath::Max(0.0f, TransitionDelay);
	const FGameplayTag ResolvedSpawnPointId = SpawnPointId.IsValid() ? SpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
	bTravelInProgress = true;
	// 이동 확정 이후 파티 대기 판정과 안내 메시지 정리
	ClearPartySyncCheckTimer();
	ClearPartySyncWaitingMessages();
	// 실제 맵 이동 전환 알림
	NotifyMapTransitionToAllPlayers(ResolvedTransitionDelay, EPRMapTransitionType::MapTravel);

	// ServerTravel 시작 예약
	FTimerDelegate TravelDelegate = FTimerDelegate::CreateWeakLambda(this, [this, MapToTravel, ResolvedSpawnPointId, ResolvedTransitionDelay]()
	{
		UWorld* TimerWorld = GetWorld();
		if (!IsValid(TimerWorld))
		{
			// 월드 소멸에 따른 맵 전환 취소 알림
			NotifyMapTransitionToAllPlayers(ResolvedTransitionDelay, EPRMapTransitionType::CancelTravel);
			bTravelInProgress = false;
			return;
		}

		if (UPRGameInstance* GameInstance = TimerWorld->GetGameInstance<UPRGameInstance>())
		{
			FPRWorldSaveData PendingWorldSaveData;
			bool bHasPendingWorldSaveData = false;
			if (const APRGameStateBase* GameState = TimerWorld->GetGameState<APRGameStateBase>())
			{
				// GameInstance를 통한 ServerTravel 직후 저장 예약용 월드 진행 상태 생성
				PendingWorldSaveData = GameState->MakeWorldSaveData();
				bHasPendingWorldSaveData = true;
			}

			if (!GameInstance->ServerTravelToMap(MapToTravel, false))
			{
				// ServerTravel 실패에 따른 맵 전환 취소 알림
				NotifyMapTransitionToAllPlayers(ResolvedTransitionDelay, EPRMapTransitionType::CancelTravel);
				bTravelInProgress = false;
				return;
			}

			if (bHasPendingWorldSaveData)
			{
				// 다음 월드 초기화 단계에서 적용할 진행 상태 예약
				GameInstance->SetPendingWorldSaveData(PendingWorldSaveData);
			}

			// 다음 월드 SpawnPoint 선택 단계에서 사용할 목적지 태그 예약
			GameInstance->SetPendingTravelSpawnPointId(ResolvedSpawnPointId);
			return;
		}

		// GameInstance 조회 실패에 따른 맵 전환 취소 알림
		NotifyMapTransitionToAllPlayers(ResolvedTransitionDelay, EPRMapTransitionType::CancelTravel);
		bTravelInProgress = false;
	});

	if (ResolvedTransitionDelay <= 0.0f)
	{
		// 지연 시간이 없을 때 즉시 ServerTravel 실행
		TravelDelegate.ExecuteIfBound();
		return;
	}

	World->GetTimerManager().SetTimer(TravelDelayTimerHandle, TravelDelegate, ResolvedTransitionDelay, false);
}

void UPRInteraction_MapTravelBase::NotifyMapTransitionToAllPlayers(float Delay, EPRMapTransitionType TransitionType) const
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		APRPlayerController* Controller = ResolveMapTravelPlayerController(PlayerState);
		if (IsValid(Controller))
		{
			// 로컬 맵 전환 페이드와 HUD 메시지 처리를 PlayerController에 위임
			Controller->ClientNotifyMapTransition(Delay, TransitionType);
		}
	}
}
