// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Party Sync 기본 구조 상호작용 액션 실행 로직 구현)
#include "PRInteraction_PartySyncBase.h"

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"

UPRInteraction_PartySyncBase::UPRInteraction_PartySyncBase()
{
	// 파티 동기화 상호작용 기본 설정
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_PartySyncBase::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	// 완료 이후 상태에서 재실행 차단
	if (IsPartySyncActionLocked())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	NotifyPartySyncInteractionStarted(Interactor);

	if (World->GetTimerManager().IsTimerActive(PartySyncCheckTimerHandle))
	{
		return;
	}

	if (PartySyncCheckDelay <= 0.0f)
	{
		// 즉시 파티 동기화 조건 검사
		CheckPartySyncCondition();
		return;
	}

	// 지연 후 파티 동기화 조건 검사
	World->GetTimerManager().SetTimer(
		PartySyncCheckTimerHandle,
		this,
		&UPRInteraction_PartySyncBase::CheckPartySyncCondition,
		PartySyncCheckDelay,
		false);
}

void UPRInteraction_PartySyncBase::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	Super::EndInteraction_Implementation(Interactor, bCanceled);

	// ActiveInteractors 정리 이후 완료 상태 피드백 중복 차단
	if (IsPartySyncActionLocked())
	{
		return;
	}

	ClearPartySyncCheckTimer();
	NotifyPartySyncInteractionEnded(Interactor, bCanceled);
}

TArray<TObjectPtr<APlayerState>> UPRInteraction_PartySyncBase::GetPlayerArray() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APRGameStateBase* GameState = World->GetGameState<APRGameStateBase>())
		{
			return GameState->PlayerArray;
		}	
	}
	return TArray<TObjectPtr<APlayerState>>();
}

void UPRInteraction_PartySyncBase::HandlePartySyncConditionMet()
{
	// 기본 처리 없음
}

void UPRInteraction_PartySyncBase::NotifyPartySyncInteractionStarted(AActor* Interactor)
{
	if (IsValid(Interactor))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Party Sync Interaction Started"), *Interactor->GetName());
	}

	if (UWorld* World = GetWorld())
	{
		// 여러 플레이어가 순차 진입할 때 최종 인원 기준 메시지 계산 예약
		World->GetTimerManager().ClearTimer(PartySyncMessageTimerHandle);
		World->GetTimerManager().SetTimer(
			PartySyncMessageTimerHandle,
			this,
			&UPRInteraction_PartySyncBase::RefreshPartySyncWaitingMessages,
			PartySyncMessageDelay,
			false);
	}
}

void UPRInteraction_PartySyncBase::NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled)
{
	if (IsValid(Interactor))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Party Sync Interaction Ended"), *Interactor->GetName());
	}

	if (UWorld* World = GetWorld())
	{
		// 이탈 직후 ActiveInteractionInfo 정리 반영을 기다리는 메시지 계산 예약
		World->GetTimerManager().ClearTimer(PartySyncMessageTimerHandle);
		World->GetTimerManager().SetTimer(
			PartySyncMessageTimerHandle,
			this,
			&UPRInteraction_PartySyncBase::RefreshPartySyncWaitingMessages,
			PartySyncMessageDelay,
			false);
	}
}

bool UPRInteraction_PartySyncBase::IsPartySyncActionLocked() const
{
	return false;
}

void UPRInteraction_PartySyncBase::ClearPartySyncCheckTimer()
{
	if (UWorld* World = GetWorld())
	{
		// 파티 동기화 판정 대기 취소
		World->GetTimerManager().ClearTimer(PartySyncCheckTimerHandle);
	}
}

void UPRInteraction_PartySyncBase::ClearPartySyncWaitingMessages()
{
	if (UWorld* World = GetWorld())
	{
		// 예약된 대기 HUD 메시지 갱신 취소
		World->GetTimerManager().ClearTimer(PartySyncMessageTimerHandle);
	}

	for (APlayerState* PlayerState :GetPlayerArray())
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		APRPlayerController* Controller = ResolvePlayerController(PRPlayerState);
		if (IsValid(Controller))
		{
			// 동기화 완료 또는 대기 종료에 따른 로컬 안내 메시지 정리
			Controller->ClientNotifyHUDMessage(EPRHUDMessageType::None);
		}
	}
}

void UPRInteraction_PartySyncBase::CheckPartySyncCondition()
{
	if (IsPartySyncActionLocked())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 행동 가능한 인원 전체의 상호작용 유지 여부 검사
	const int32 FightCapablePlayerCount = CountFightCapablePlayers();
	if (FightCapablePlayerCount > 0 && CountInteractingPlayers() == FightCapablePlayerCount)
	{
		HandlePartySyncConditionMet();
	}
}

int32 UPRInteraction_PartySyncBase::CountInteractingPlayers() const
{
	int32 InteractingPlayerCount = 0;
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		if (IsPlayerInteracting(PRPlayerState))
		{
			++InteractingPlayerCount;
		}
	}

	return InteractingPlayerCount;
}

int32 UPRInteraction_PartySyncBase::CountFightCapablePlayers() const
{
	int32 FightCapablePlayerCount = 0;
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		++FightCapablePlayerCount;
	}

	return FightCapablePlayerCount;
}

void UPRInteraction_PartySyncBase::RefreshPartySyncWaitingMessages()
{
	const int32 FightCapablePlayerCount = CountFightCapablePlayers();
	const int32 InteractingPlayerCount = CountInteractingPlayers();
	// 파티 일부만 상호작용 중일 때만 대기 안내 표시
	const bool bShouldShowWaitingMessage =
		FightCapablePlayerCount > 1
		&& InteractingPlayerCount > 0
		&& InteractingPlayerCount < FightCapablePlayerCount;

	for (APlayerState* PlayerState : GetPlayerArray())
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		APRPlayerController* Controller = ResolvePlayerController(PRPlayerState);
		if (!IsValid(Controller))
		{
			continue;
		}

		if (!bShouldShowWaitingMessage
			|| !IsValid(PRPlayerState)
			|| !PRPlayerState->IsCombatParticipant()
			|| PRPlayerState->IsOutOfFight())
		{
			Controller->ClientNotifyHUDMessage(EPRHUDMessageType::None);
			continue;
		}

		// 상호작용 참여 여부에 따른 플레이어별 안내 메시지 분기
		const EPRHUDMessageType MessageType = IsPlayerInteracting(PRPlayerState)
			? EPRHUDMessageType::WaitingForOtherPlayers
			: EPRHUDMessageType::OtherPlayersWaiting;
		Controller->ClientNotifyHUDMessage(MessageType);
	}

	if (UWorld* MutableWorld = GetWorld())
	{
		MutableWorld->GetTimerManager().ClearTimer(PartySyncMessageTimerHandle);
	}
}

bool UPRInteraction_PartySyncBase::IsPlayerInteracting(const APRPlayerState* PlayerState) const
{
	const APRPlayerController* Controller = ResolvePlayerController(PlayerState);
	if (!IsValid(Controller))
	{
		return false;
	}

	const UPRInteractorComponent* InteractorComponent = Controller->FindComponentByClass<UPRInteractorComponent>();
	// InteractorComponent가 서버에서 추적 중인 유지형 Action과의 일치 여부 확인
	return IsValid(InteractorComponent) && InteractorComponent->GetActiveAction() == this;
}

APRPlayerController* UPRInteraction_PartySyncBase::ResolvePlayerController(const APRPlayerState* PlayerState) const
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
