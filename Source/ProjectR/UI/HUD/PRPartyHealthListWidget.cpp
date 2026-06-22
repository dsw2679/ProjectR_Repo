// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Party Health List UI 위젯 구현)
#include "ProjectR/UI/HUD/PRPartyHealthListWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/HUD/PRPartyMemberHealthWidget.h"
#include "TimerManager.h"

namespace
{
	// 파티원 목록 초기화 최대 재시도 횟수
	constexpr int32 PartyHealthRefreshRetryCount = 3;

	// 파티원 목록 초기화 재시도 간격
	constexpr float PartyHealthRefreshRetryInterval = 0.1f;
}

UPRPartyHealthListWidget::UPRPartyHealthListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRPartyHealthListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachePartyMemberSlots();
	BindPlayerCountChanged();
	RefreshPartyMembers();
}

void UPRPartyHealthListWidget::NativeDestruct()
{
	UnbindPlayerCountChanged();

	ApplyPartyMembers(TArray<APRPlayerState*>());

	Super::NativeDestruct();
}

/*~ Public API ~*/

void UPRPartyHealthListWidget::RefreshPartyMembers()
{
	RefreshPartyMembersInternal(PartyHealthRefreshRetryCount);
}

void UPRPartyHealthListWidget::RefreshPartyMembersInternal(int32 RemainingRetryCount)
{
	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	APRPlayerState* OwningPlayerState = GetOwningPRPlayerState();
	if (!IsValid(GameState) || !IsValid(OwningPlayerState))
	{
		BindPlayerCountChanged();
		ApplyPartyMembers(TArray<APRPlayerState*>());
		ScheduleRefreshRetry(RemainingRetryCount);
		return;
	}

	BindPlayerCountChanged();

	TArray<APRPlayerState*> PartyMembers;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || PRPlayerState == OwningPlayerState || !IsValid(PRPlayerState->GetAbilitySystemComponent()))
		{
			continue;
		}

		PartyMembers.Add(PRPlayerState);
	}

	PartyMembers.Sort([](const APRPlayerState& Left, const APRPlayerState& Right)
	{
		const int32 LeftPlayerIndex = Left.GetPRPlayerIndex();
		const int32 RightPlayerIndex = Right.GetPRPlayerIndex();
		if (LeftPlayerIndex != INDEX_NONE && RightPlayerIndex != INDEX_NONE)
		{
			return LeftPlayerIndex < RightPlayerIndex;
		}

		return Left.GetPlayerId() < Right.GetPlayerId();
	});

	if (PartyMembers.Num() > PartyMemberSlots.Num())
	{
		PartyMembers.SetNum(PartyMemberSlots.Num());
	}

	ApplyPartyMembers(PartyMembers);

	if (GameState->PlayerArray.Num() > 1 && PartyMembers.Num() == 0)
	{
		// PlayerArray 복제 이후 개별 PlayerState 준비 지연 대응
		ScheduleRefreshRetry(RemainingRetryCount);
	}
}

/*~ Party Members ~*/

void UPRPartyHealthListWidget::CachePartyMemberSlots()
{
	PartyMemberSlots.Reset();

	if (!IsValid(PartyMemberListPanel))
	{
		return;
	}

	const int32 ChildCount = PartyMemberListPanel->GetChildrenCount();
	PartyMemberSlots.Reserve(ChildCount);
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		UPRPartyMemberHealthWidget* PartyMemberSlot = Cast<UPRPartyMemberHealthWidget>(PartyMemberListPanel->GetChildAt(ChildIndex));
		if (!IsValid(PartyMemberSlot))
		{
			continue;
		}

		PartyMemberSlots.Add(PartyMemberSlot);
	}
}

void UPRPartyHealthListWidget::BindPlayerCountChanged()
{
	if (PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	PlayerCountChangedDelegateHandle = GameState->OnPlayerCountChanged.AddUObject(this, &ThisClass::HandlePlayerCountChanged);
}

void UPRPartyHealthListWidget::UnbindPlayerCountChanged()
{
	if (!PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (IsValid(GameState))
	{
		GameState->OnPlayerCountChanged.Remove(PlayerCountChangedDelegateHandle);
	}

	PlayerCountChangedDelegateHandle.Reset();
}

void UPRPartyHealthListWidget::HandlePlayerCountChanged()
{
	RefreshPartyMembers();
}

void UPRPartyHealthListWidget::ScheduleRefreshRetry(int32 RemainingRetryCount)
{
	if (RemainingRetryCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FTimerHandle RetryTimerHandle;
	TWeakObjectPtr<UPRPartyHealthListWidget> WeakThis(this);
	World->GetTimerManager().SetTimer(
		RetryTimerHandle,
		FTimerDelegate::CreateLambda([WeakThis, RemainingRetryCount]()
		{
			if (UPRPartyHealthListWidget* Widget = WeakThis.Get())
			{
				// 다음 재시도 잔여 횟수 차감
				Widget->RefreshPartyMembersInternal(RemainingRetryCount - 1);
			}
		}),
		PartyHealthRefreshRetryInterval,
		false);
}

APRPlayerState* UPRPartyHealthListWidget::GetOwningPRPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APRPlayerState>() : nullptr;
}

void UPRPartyHealthListWidget::ApplyPartyMembers(const TArray<APRPlayerState*>& PartyMembers)
{
	for (int32 Index = 0; Index < PartyMemberSlots.Num(); ++Index)
	{
		UPRPartyMemberHealthWidget* PartySlot = PartyMemberSlots[Index];
		if (!IsValid(PartySlot))
		{
			continue;
		}

		if (PartyMembers.IsValidIndex(Index))
		{
			PartySlot->SetPlayerState(PartyMembers[Index]);
		}
		else
		{
			PartySlot->ClearPlayerState();
		}
	}

	const bool bHasPartyMembers = PartyMembers.Num() > 0;
	SetVisibility(bHasPartyMembers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
