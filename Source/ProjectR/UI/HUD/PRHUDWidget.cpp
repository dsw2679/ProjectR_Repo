// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 HP/포이즈 게이지 및 픽업 보상 알림 UI 구현)
// Author: 배유찬 (Mod 게이지, 핑/마커 알림 및 홀드 상호작용 게이지 UI 구현)
// Author: 이건주 (무기 장착 상태 및 소모품 퀵슬롯 UI 구현)
#include "PRHUDWidget.h"

#include "PRInteractionProgressBar.h"
#include "EngineUtils.h"
#include "PRInteractionHintWidget.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairWidget.h"
#include "ProjectR/UI/HUD/PRBossHealthBarWidget.h"
#include "ProjectR/UI/HUD/PRHealthBarWidget.h"
#include "ProjectR/UI/HUD/PRPartyHealthListWidget.h"
#include "ProjectR/UI/HUD/PRPickupNotificationListWidget.h"
#include "ProjectR/UI/HUD/PRStaminaBarWidget.h"
#include "ProjectR/UI/HUD/PRQuickSlotWidget.h"
#include "ProjectR/UI/Growth/PRLevelUpPopupWidget.h"
#include "ProjectR/UI/Growth/PRTraitNotiTextWidget.h"
#include "ProjectR/UI/WeaponStatusHUD/PRWeaponHUDWidget.h"

UPRHUDWidget::UPRHUDWidget()
{
	// HUD 컨테이너는 HUD 레이어에 위치하며 입력에 영향을 주지 않는다
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ Public API ~*/

void UPRHUDWidget::BindBossHealthBar(APRBossBaseCharacter* InBoss)
{
	if (!IsValid(BossHealthBar))
	{
		return;
	}

	BossHealthBar->BindBoss(InBoss);
}

void UPRHUDWidget::ClearBossHealthBar()
{
	if (!IsValid(BossHealthBar))
	{
		return;
	}

	BossHealthBar->ClearBoss();
}

void UPRHUDWidget::ShowLevelUpPopup(int32 PreviousLevel, int32 CurrentLevel)
{
	if (CurrentLevel <= PreviousLevel || !IsValid(LevelUpPopupWidget))
	{
		return;
	}

	LevelUpPopupWidget->ShowLevelUp(PreviousLevel, CurrentLevel);
}

void UPRHUDWidget::ShowPickupRewardNotification(const FPRPickupNotificationPayload& Payload)
{
	if (!IsValid(PickupNotificationListWidget))
	{
		return;
	}

	PickupNotificationListWidget->AddNotification(Payload);
}

void UPRHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UPREventManagerSubsystem* EventMgr = GetEventManager();
	if (!IsValid(EventMgr))
	{
		return;
	}
	
	bool bPlayerReady = false;
	if (APlayerController* OwningPlayerController = GetOwningPlayer())
	{
		if (APRPlayerState* PS = OwningPlayerController->GetPlayerState<APRPlayerState>())
		{
			bPlayerReady = true; 
		}
	}
	
	if (bPlayerReady)
	{
		OnPlayerReady();
	}
	else
	{
		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Player_Ready,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandlePlayerReady)));
	}

	// 크로스헤어가 BP 레이아웃에 포함된 경우에만 에임 이벤트를 구독한다
	if (IsValid(CrosshairWidget))
	{
		// 초기 상태는 숨김. Aim.Start 신호를 받기 전까지 보이지 않는다
		SetCrosshairVisible(false);

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_Start,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimStart)));

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_End,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimEnd)));
	}

	// 상호작용 HUD 가 BP 레이아웃에 포함된 경우에만 Hold 이벤트를 구독한다
	if (IsValid(InteractionProgressBar))
	{
		InteractionProgressBar->SetProgressBarVisible(false);

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Interaction_Hold,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleInteractionHold)));
	}
	
	if (IsValid(InteractionHint))
	{
		InteractionHint->SetHintVisible(false);
		
		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Interactable,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleInteractableChanged)));
	}

	// 보스 HP 바가 BP 레이아웃에 포함된 경우에만 보스 조우 이벤트를 구독한다
	if (IsValid(BossHealthBar))
	{
		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Boss_Encounter_Begin,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleBossEncounterBegin)));

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Boss_Encounter_End,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleBossEncounterEnd)));

		// 위젯 생성 이전에 이미 스폰된 보스가 있을 수 있으므로 즉시 디스커버리
		DiscoverActiveBossInWorld();
	}
}

void UPRHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(BossHealthBar) && !BossHealthBar->IsBossBound())
	{
		DiscoverActiveBossInWorld();
	}
}

void UPRHUDWidget::NativeDestruct()
{
	UnbindFromEventManager();
	Super::NativeDestruct();
}

void UPRHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Hold 진행 중일 때 진행도 보간. HoldDuration 이 0 이하인 경우는 Start 즉시 완료로 간주
	if (bIsHoldActive && IsValid(InteractionProgressBar))
	{
		HoldElapsed += InDeltaTime;
		const float Percent = HoldDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(HoldElapsed / HoldDuration, 0.f, 1.f)
			: 1.f;
		InteractionProgressBar->SetProgress(Percent);
	}
}

void UPRHUDWidget::OnPlayerReady()
{
	if (IsValid(PlayerHealthBar))
	{
		PlayerHealthBar->RefreshHealthFromOwner();
	}

	if (IsValid(PlayerStaminaBar))
	{
		PlayerStaminaBar->RefreshStaminaFromOwner();
	}

	if (IsValid(TraitNotiTextWidget))
	{
		TraitNotiTextWidget->RefreshTraitPointFromOwner();
	}

	if (IsValid(PartyHealthList))
	{
		PartyHealthList->RefreshPartyMembers();
	}
	
	if (IsValid(WeaponHUD))
	{
		WeaponHUD->InitializeWeaponHUD();
	}
	if (IsValid(QuickSlotHUD))
	{
		QuickSlotHUD->InitializeQuickSlotHUD();
	}
}

void UPRHUDWidget::HandlePlayerReady(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	OnPlayerReady();
}

void UPRHUDWidget::HandleAimStart(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(true);
}

void UPRHUDWidget::HandleAimEnd(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(false);
}

void UPRHUDWidget::HandleInteractionHold(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!IsValid(InteractionProgressBar))
	{
		return;
	}

	const FPRInteractionHoldEventPayload* Data = Payload.GetPtr<FPRInteractionHoldEventPayload>();
	if (Data == nullptr)
	{
		return;
	}

	switch (Data->Phase)
	{
	case EPRInteractionHoldPhase::Start:
		HoldDuration = Data->HoldDuration;
		HoldElapsed = 0.f;
		bIsHoldActive = true;
		InteractionProgressBar->SetProgressBarVisible(true);
		
		if (InteractionHint)
		{
			InteractionHint->SetHintVisible(false);
		}
		
		InteractionProgressBar->SetProgress(0.f);
		InteractionProgressBar->SetActionNameText(Data->ActionName);
		break;

	case EPRInteractionHoldPhase::Canceled:
		bIsHoldActive = false;
		HoldElapsed = 0.f;
		HoldDuration = 0.f;
		InteractionProgressBar->SetProgressBarVisible(false);
		break;

	case EPRInteractionHoldPhase::Finished:
		bIsHoldActive = false;
		// 완료 펄스를 위해 한 프레임은 100% 로 채운 뒤 숨김 처리
		InteractionProgressBar->SetProgress(1.f);
		InteractionProgressBar->SetProgressBarVisible(false);
		HoldElapsed = 0.f;
		HoldDuration = 0.f;
		break;
	}
}

void UPRHUDWidget::HandleInteractableChanged(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!IsValid(InteractionHint))
	{
		return;
	}

	const FPRInteractableEventPayload* Data = Payload.GetPtr<FPRInteractableEventPayload>();
	if (Data == nullptr)
	{
		return;
	}
	
	if (Data->bShowPrompt)
	{
		InteractionHint->SetHintVisible(true);
		InteractionHint->SetInteractionHintText(Data->ActionHintText);
		InteractionHint->SetCanInteract(Data->bCanInteract);
	}
	else
	{
		InteractionHint->SetHintVisible(false);
	}
}

void UPRHUDWidget::HandleBossEncounterBegin(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!IsValid(BossHealthBar))
	{
		return;
	}

	const FPRBossEncounterEventPayload* Data = Payload.GetPtr<FPRBossEncounterEventPayload>();
	if (Data == nullptr || !IsValid(Data->Boss))
	{
		return;
	}

	BindBossHealthBar(Data->Boss);
}

void UPRHUDWidget::HandleBossEncounterEnd(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	ClearBossHealthBar();
}

void UPRHUDWidget::DiscoverActiveBossInWorld()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 월드에 이미 존재하는 첫 번째 보스를 바인딩. 후속 보스 등장은 Event_Boss_Encounter_Begin 이벤트로 갱신됨
	for (TActorIterator<APRBossBaseCharacter> It(World); It; ++It)
	{
		APRBossBaseCharacter* Boss = *It;
		if (!IsValid(Boss))
		{
			continue;
		}

		if (APRFaerinCharacter* Faerin = Cast<APRFaerinCharacter>(Boss))
		{
			if (!Faerin->IsBossEncounterActive())
			{
				continue;
			}
		}

		if (!BossHealthBar->IsBossBound())
		{
			BindBossHealthBar(Boss);
		}
		return;
	}
}

void UPRHUDWidget::UnbindFromEventManager()
{
	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		for (auto& Handle : EventHandles)
		{
			EventMgr->UnlistenAll(Handle);
		}
	}
	EventHandles.Reset();
}

void UPRHUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (!IsValid(CrosshairWidget))
	{
		return;
	}
	CrosshairWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
