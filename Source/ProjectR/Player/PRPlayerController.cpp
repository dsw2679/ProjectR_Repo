// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (로딩 화면 프리웜, 픽업 알림 및 강화/성장 UI 호출 제어 구현)
// Author: 배유찬 (세션 매치메이킹 UI, 리스폰 흐름 및 패스트 트래블 UI, 플래시라이트 입력 제어 구현)
// Author: 이건주 (마우스 감도 설정 갱신 및 인벤토리/무기 상태 HUD 호출 제어 구현)
#include "PRPlayerController.h"

#include "Net/UnrealNetwork.h"
#include "ProjectR/Test/PRCheatHandler.h"
#include "ProjectR/Audio/PRBGMSubsystem.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/FX/PRFXNetworkComponent.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Input/PRInputConfigDataAsset.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Projectile/PRProjectileManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_Waypoint.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/Interaction/PRInteractionSensor.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/System/PRLoadingScreenSubsystem.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/System/PRWorldTickOptimizerReporterComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr float LoadingScreenFadeInAckDelay = 1.0f;

	void SetLocalBGMState(const UObject* WorldContextObject, EPRBGMState BGMState)
	{
		if (!IsValid(WorldContextObject))
		{
			return;
		}

		UWorld* World = WorldContextObject->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		UPRBGMSubsystem* BGMSubsystem = World->GetSubsystem<UPRBGMSubsystem>();
		if (!IsValid(BGMSubsystem))
		{
			return;
		}

		BGMSubsystem->SetBGMState(BGMState);
	}

	void RestoreLocalDefaultBGM(const UObject* WorldContextObject)
	{
		if (!IsValid(WorldContextObject))
		{
			return;
		}

		UWorld* World = WorldContextObject->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		UPRBGMSubsystem* BGMSubsystem = World->GetSubsystem<UPRBGMSubsystem>();
		if (!IsValid(BGMSubsystem))
		{
			return;
		}

		BGMSubsystem->RestoreDefaultLevelBGM();
	}
}

APRPlayerController::APRPlayerController()
{
	PlayerCameraManagerClass = APRCameraManager::StaticClass();

	// 등록 기반 SubObject 복제 시스템 사용. CheatHandler를 AddReplicatedSubObject로 등록 가능
	bReplicateUsingRegisteredSubObjectList = true;

	ProjectileManager = CreateDefaultSubobject<UPRProjectileManagerComponent>(TEXT("ProjectileManager"));
	FloatingTextManager = CreateDefaultSubobject<UPRFloatingTextManager>(TEXT("FloatingTextManager"));
	UIControllerComponent = CreateDefaultSubobject<UPRUIControllerComponent>(TEXT("UIControllerComponent"));
	InteractionSensor = CreateDefaultSubobject<UPRInteractionSensor>(TEXT("InteractionSensor"));
	InteractorComponent = CreateDefaultSubobject<UPRInteractorComponent>(TEXT("InteractorComponent"));
	
	// Player 소유 Client RPC와 owning client의 Server RPC 호출을 위한 FX 네트워크 컴포넌트
	FXNetworkComponent = CreateDefaultSubobject<UPRFXNetworkComponent>(TEXT("FXNetworkComponent"));

	// WorldTickOptimizer 렌더 상태 변경분을 owning client에서 서버로 전달하는 컴포넌트
	TickOptimizerReporterComponent = CreateDefaultSubobject<UPRWorldTickOptimizerReporterComponent>(TEXT("TickOptimizerReporterComponent"));
}

// =====  APlayerController Interface =====

void APRPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APRPlayerController, CheatHandler, COND_OwnerOnly);
}

void APRPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCompanionHighlight();
}

void APRPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	EnableCheats();

	// 서버 권위에서 CheatHandler 생성 후 ReplicatedSubObject로 등록. 본인 클라에 복제
	if (HasAuthority() && IsValid(CheatHandlerClass))
	{
		CheatHandler = NewObject<UPRCheatHandler>(this, CheatHandlerClass);
		AddReplicatedSubObject(CheatHandler);
	}
#endif

	// 캐릭터 세이브 제출은 ReceivedPlayer와 possession fallback 경로 처리
}

void APRPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController() && GetNetMode() == NM_Client)
	{
		// possession 이전 캐릭터 세이브 페이로드 조기 제출
		SubmitLocalCharacterToServer();
	}
}

void APRPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	ResetPlayer();

	// 새 폰 possession 시점에 폰 의존 UI를 재초기화. 초기 possession과 리스폰 양쪽에서 동작
	if (IsValid(UIControllerComponent))
	{
		UIControllerComponent->RefreshForPawn(InPawn);
	}
	
	// 게임 시작 or 맵 진입 후 FadeIn
	if (IsLocalController())
	{
		if (APRCameraManager* CM = Cast<APRCameraManager>(PlayerCameraManager))
		{
			CM->FadeIn(EPRFadeColorPreset::Black, FadeInDuration, false);
		}
	}

	if (IsLocalController() && GetNetMode() == NM_Client)
	{
		// 조기 제출 실패 또는 travel 타이밍 차이 대비 재시도
		SubmitLocalCharacterToServer();
	}
}

void APRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EIC))
	{
		return;
	}

	if (IsValid(InventoryAction))
	{
		EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &APRPlayerController::OnInventoryInputStarted);
	}
	
	if (IsValid(MouseSensitivityActionUp))
	{
		EIC->BindAction(MouseSensitivityActionUp, ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionUp);
	}
	
	if (IsValid(MouseSensitivityActionDown))
	{
		EIC->BindAction(MouseSensitivityActionDown, ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionDown);
	}
	
	if (IsValid(FlashlightAction))
	{
		EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &APRPlayerController::ToggleFlashlight);
	}

	if (IsValid(TraitWindowAction.Get()))
	{
		EIC->BindAction(TraitWindowAction.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnTraitWindowInputStarted);
	}

	if (IsValid(InGameMenuAction.Get()))
	{
		EIC->BindAction(InGameMenuAction.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnInGameMenuInputStarted);
	}
	
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotActions.Num(); ++SlotIndex)
	{
		if (!IsValid(QuickSlotActions[SlotIndex]))
		{
			continue;
		}

		EIC->BindAction(QuickSlotActions[SlotIndex], ETriggerEvent::Started, this, &APRPlayerController::OnQuickSlotInputStarted, SlotIndex);
	}

	if (IsValid(InputConfig))
	{
		// IA별로 Started/Completed에 InputTag 포함 콜백을 바인딩
		for (const FPRInputActionBinding& Binding : InputConfig->AbilityInputBindings)
		{
			if (!IsValid(Binding.InputAction.Get()) || !Binding.InputTag.IsValid())
			{
				continue;
			}

			EIC->BindAction(Binding.InputAction.Get(), ETriggerEvent::Started, this,
				&APRPlayerController::OnAbilityInputPressed, Binding.InputTag);
			EIC->BindAction(Binding.InputAction.Get(), ETriggerEvent::Completed, this,
				&APRPlayerController::OnAbilityInputReleased, Binding.InputTag);
		}
	}
}

void APRPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void APRPlayerController::ClientNotifyMapTransition_Implementation(float Delay, EPRMapTransitionType TransitionType)
{
	if (TransitionType == EPRMapTransitionType::None)
	{
		return;
	}

	ResetPlayer();
	
	// Handle UI Display
	if (IsValid(UIControllerComponent))
	{
		switch (TransitionType)
		{
		case EPRMapTransitionType::MapTravel:
			// 실제 맵 이동 대기 HUD 메시지 표시
			UIControllerComponent->NotifyHUDMessage(EPRHUDMessageType::MapTravelInProgress);
			break;
		case EPRMapTransitionType::WaypointTravelUI:
			break;
		case EPRMapTransitionType::Respawn:
			// 리스폰 전환에 따른 HUD 메시지 정리
			UIControllerComponent->NotifyHUDMessage(EPRHUDMessageType::None);
			break;
		case EPRMapTransitionType::CancelTravel:
			// 전환 취소에 따른 HUD 메시지 정리
			UIControllerComponent->NotifyHUDMessage(EPRHUDMessageType::None);
			// UI 정리
			UIControllerComponent->RefreshForPawn(GetPawn());
			break;
		case EPRMapTransitionType::RespawnComplete:
			HidePartyWipeWidget();

			// 리스폰 완료에 따른 HUD 메시지 정리
			UIControllerComponent->NotifyHUDMessage(EPRHUDMessageType::None);
			// 리스폰 완료 UI 복구
			UIControllerComponent->RefreshForPawn(GetPawn());
			break;
		default:
			break;
		}
	}
	
	// Handle Fade In/Out
	if (APRCameraManager* CM = Cast<APRCameraManager>(PlayerCameraManager))
	{
		switch (TransitionType)
		{
		case EPRMapTransitionType::MapTravel:
			// 맵 이동 페이드
			if (Delay <= 0.0f)
			{
				CM->FadeIn(EPRFadeColorPreset::White, 0.0f, false);
			}
			else
			{
				CM->FadeOut(EPRFadeColorPreset::White, Delay, false);
			}
			break;
		case EPRMapTransitionType::WaypointTravelUI:
			// Waypoint Travel UI 표시 전 페이드
			CM->FadeOut(EPRFadeColorPreset::White, Delay, false);
			break;
		case EPRMapTransitionType::Respawn:
			// 리스폰 페이드
			CM->FadeOut(EPRFadeColorPreset::Black, Delay, false);
			break;
		case EPRMapTransitionType::RespawnComplete:
			// 리스폰 완료 페이드
			CM->FadeIn(EPRFadeColorPreset::Black, Delay, false);
			break;
		case EPRMapTransitionType::CancelTravel:
			CM->FadeIn(EPRFadeColorPreset::White, Delay, false);
			break;
		default:
			break;
		}
	}

	if (TransitionType == EPRMapTransitionType::RespawnComplete && IsLocalController())
	{
		UWorld* World = GetWorld();
		if (IsValid(World))
		{
			if (UPRBGMSubsystem* BGMSubsystem = World->GetSubsystem<UPRBGMSubsystem>())
			{
				BGMSubsystem->ResetToLevelBGM(0.0f);
			}
		}
	}
}

void APRPlayerController::ClientBeginMapLoadingScreen_Implementation(const FString& MapName)
{
	if (MapName.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
	{
		LoadingScreen->BeginTravelToMap(TEXT("WaypointTravel"), MapName);
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || LoadingScreenFadeInAckDelay <= 0.0f)
	{
		if (HasAuthority())
		{
			LastAcknowledgedLoadingScreenMapName = MapName;
		}
		else
		{
			ServerAcknowledgeMapLoadingScreen(MapName);
		}

		return;
	}

	World->GetTimerManager().ClearTimer(LoadingScreenAckTimerHandle);
	FTimerDelegate AckDelegate = FTimerDelegate::CreateWeakLambda(this, [this, MapName]()
	{
		if (HasAuthority())
		{
			LastAcknowledgedLoadingScreenMapName = MapName;
		}
		else
		{
			ServerAcknowledgeMapLoadingScreen(MapName);
		}
	});
	World->GetTimerManager().SetTimer(LoadingScreenAckTimerHandle, AckDelegate, LoadingScreenFadeInAckDelay, false);
}

bool APRPlayerController::HasAcknowledgedMapLoadingScreen(const FString& MapName) const
{
	return !MapName.IsEmpty() && LastAcknowledgedLoadingScreenMapName == MapName;
}

void APRPlayerController::ResetAcknowledgedMapLoadingScreen()
{
	LastAcknowledgedLoadingScreenMapName.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadingScreenAckTimerHandle);
	}
}

void APRPlayerController::OnMouseSensitivityActionUp()
{
	APRPlayerState* PS = GetPlayerState<APRPlayerState>();
	
	if (IsValid(PS))
	{
		float NewSensitivity = PS->GetCameraSensitivity() + 0.05;
		PS->SetCameraSensitivity(NewSensitivity);
	}
}

void APRPlayerController::OnMouseSensitivityActionDown()
{
	APRPlayerState* PS = GetPlayerState<APRPlayerState>();
	
	if (IsValid(PS))
	{
		float NewSensitivity = PS->GetCameraSensitivity() - 0.05;
		PS->SetCameraSensitivity(NewSensitivity);
	}
}

void APRPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->AbilityInputPressed(InputTag);
	}
}

void APRPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->AbilityInputReleased(InputTag);
	}
}

void APRPlayerController::ToggleFlashlight(const FInputActionValue& Value)
{
	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	
	PlayerCharacter->SetFlashlightEnabled(!PlayerCharacter->IsFlashlightEnabled());
}

UPRAbilitySystemComponent* APRPlayerController::GetPRASC() const
{
	if (CachedASC.IsValid())
	{
		return CachedASC.Get();
	}

	if (APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>())
	{
		if (UPRAbilitySystemComponent* ASC = PRPlayerState->GetPRAbilitySystemComponent())
		{
			CachedASC = ASC;
			return ASC;
		}
	}
	
	if (APawn* LocalPawn = GetPawn())
	{
		if (UPRAbilitySystemComponent* ASC =  Cast<UPRAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(LocalPawn)))
		{
			CachedASC = ASC;
			return ASC;
		}
	}
	return nullptr;
}

void APRPlayerController::ResetPlayer()
{
	CachedASC.Reset();
	if (APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>())
	{
		// PlayerState 소유 ASC 캐시 갱신
		CachedASC = PRPlayerState->GetPRAbilitySystemComponent();
	}

	ResetLocalInteractionVisualState();
}

void APRPlayerController::ResetLocalInteractionVisualState()
{
	if (!IsLocalController())
	{
		return;
	}

	if (IsValid(InteractorComponent))
	{
		// 전환 전 로컬 포커스 정리
		InteractorComponent->ClearFocus();
	}

	APRPlayerCharacter* LocalCharacter = Cast<APRPlayerCharacter>(GetPawn());
	UPRInteractableComponent* Interactable = IsValid(LocalCharacter) ? LocalCharacter->GetInteractableComponent() : nullptr;
	if (IsValid(Interactable) && Interactable->IsDepthStencilApplied())
	{
		// 본인 캐릭터 외곽선 잔상 제거
		Interactable->ResetDepthStencilValues();
	}
}

void APRPlayerController::UpdateCompanionHighlight()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	APRGameStateBase* GS = World->GetGameState<APRGameStateBase>();
	if (!IsValid(GS))
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	APawn* MyPawn = GetPawn();

	// 본인 제외 모든 플레이어 캐릭터 순회. 카메라 뷰포인트에서 캐릭터 위치까지 라인 트레이스로 차폐 여부 판정
	for (APRPlayerCharacter* OtherCharacter : GS->GetPlayerCharacters())
	{
		if (OtherCharacter == MyPawn)
		{
			UPRInteractableComponent* OwnInteractable = OtherCharacter->GetInteractableComponent();
			if (IsValid(OwnInteractable) && OwnInteractable->IsDepthStencilApplied())
			{
				// 본인 캐릭터 외곽선 방어 정리
				OwnInteractable->ResetDepthStencilValues();
			}
			continue;
		}
		
		UPRInteractableComponent* Interactable = OtherCharacter->GetInteractableComponent();
		const bool bFocusedByInteractor = IsValid(InteractorComponent)
			&& InteractorComponent->GetFocusedComponent() == Interactable;
		if (bFocusedByInteractor)
		{
			continue;
		}
		
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PRPlayerVisibility), false, this);
		Params.AddIgnoredActor(OtherCharacter);
		if (IsValid(MyPawn))
		{
			Params.AddIgnoredActor(MyPawn);
		}

		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit, ViewLocation, OtherCharacter->GetActorLocation(), ECC_Visibility, Params);
		const bool bVisible = !bBlocked;
		
		if (Interactable->IsDepthStencilApplied())
		{
			// 보이는 경우 하이라이트 해제
			if (bVisible)
			{
				Interactable->ResetDepthStencilValues();
			}
		}
		// 벽에 가려진 경우 하이라이트 적용
		else if (!bVisible)
		{
			Interactable->ApplyDepthStencilValues(false);
		}
	}
}

void APRPlayerController::HidePartyWipeWidget()
{
	if (IsValid(PartyWipeWidget))
	{
		PartyWipeWidget->RemoveFromParent();
	}

	PartyWipeWidget = nullptr;
}

void APRPlayerController::SubmitLocalCharacterToServer()
{
	if (bCharacterSubmitted)
	{
		return;
	}

	UPRGameInstance* GI = GetGameInstance<UPRGameInstance>();
	if (!IsValid(GI))
	{
		return;
	}

	if (!GI->EnsureLocalCharacterReadyForSession())
	{
		return;
	}

	bCharacterSubmitted = true;
	ServerSubmitCharacter(GI->GetLocalCharacter());
}

bool APRPlayerController::ServerSubmitCharacter_Validate(const FPRCharacterSaveData& Payload)
{
	// RPC 단계에서는 포맷만 확인. 상세 검증은 GameMode에서 수행
	return true;
}

void APRPlayerController::ServerSubmitCharacter_Implementation(const FPRCharacterSaveData& Payload)
{
	APRPlayGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(GM))
	{
		ClientCharacterAccepted(false, TEXT("Server GameMode unavailable"));
		return;
	}

	const bool bAccepted = GM->AcceptGuestCharacter(this, Payload);
	if (!bAccepted)
	{
		ClientCharacterAccepted(false, TEXT("Payload rejected"));
	}
	else
	{
		ClientCharacterAccepted(true, FString());
	}
}

// =====  서버 -> 클라 통지 =====

void APRPlayerController::ClientCharacterAccepted_Implementation(bool bAccepted, const FString& Detail)
{
	if (!bAccepted)
	{
		// 거부 시 세션 퇴장
		if (UPRGameInstance* GI = GetGameInstance<UPRGameInstance>())
		{
			GI->LeaveSession();
		}
	}
}

void APRPlayerController::ClientGrantReward_Implementation(const FPRRewardGrant& Grant)
{
	UPRGameInstance* GI = GetGameInstance<UPRGameInstance>();
	if (!IsValid(GI))
	{
		return;
	}

	GI->ApplyRewardGrant(Grant);
}

void APRPlayerController::ClientDispatchSurvivalGameplayEvent_Implementation(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = ControlledPawn;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		ControlledPawn,
		EventTag,
		Payload);
}

void APRPlayerController::ClientPlayPartyWipeSound_Implementation(USoundBase* Sound)
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		if (UPRBGMSubsystem* BGMSubsystem = World->GetSubsystem<UPRBGMSubsystem>())
		{
			BGMSubsystem->StopBGM(0.0f);
		}
	}

	if (!IsValid(Sound))
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
}

void APRPlayerController::ClientShowPartyWipeWidget_Implementation(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!IsLocalController())
	{
		return;
	}

	HidePartyWipeWidget();

	if (!IsValid(WidgetClass.Get()))
	{
		return;
	}

	PartyWipeWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (IsValid(PartyWipeWidget))
	{
		PartyWipeWidget->AddToViewport(10000);
	}
}

void APRPlayerController::SetEncounterInputLockLocal(bool bLock)
{
	SetIgnoreMoveInput(bLock);
	SetIgnoreLookInput(bLock);
}

void APRPlayerController::ClientSetEncounterInputLock_Implementation(bool bLock)
{
	SetEncounterInputLockLocal(bLock);
}

void APRPlayerController::RestoreFaerinEncounterViewTargetLocal(float BlendTime)
{
	if (!IsLocalController())
	{
		return;
	}

	APawn* LocalPawn = GetPawn();
	if (!IsValid(LocalPawn))
	{
		return;
	}

	SetViewTargetWithBlend(LocalPawn, FMath::Max(BlendTime, 0.0f), VTBlend_Cubic, 2.0f, false);
}

void APRPlayerController::ClientRestoreFaerinEncounterViewTarget_Implementation(float BlendTime)
{
	RestoreFaerinEncounterViewTargetLocal(BlendTime);
}

void APRPlayerController::SetFaerinEncounterHUDVisibleLocal(bool bVisible)
{
	if (!IsLocalController() || !IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->SetHUDVisible(bVisible);
}

void APRPlayerController::ClientSetFaerinEncounterHUDVisible_Implementation(bool bVisible)
{
	SetFaerinEncounterHUDVisibleLocal(bVisible);
}

void APRPlayerController::ClientNotifyWeaponUpgradeResult_Implementation(const FPRWeaponUpgradeResult& Result)
{
	OnWeaponUpgradeResult.Broadcast(Result);
}

void APRPlayerController::ClientOpenWeaponUpgradeUI_Implementation(UPRWeaponUpgradeComponent* UpgradeComponent)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->OpenWeaponUpgrade(UpgradeComponent);
}

void APRPlayerController::ClientOpenShopUI_Implementation(UPRShopComponent* ShopComponent)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->OpenShop(ShopComponent);
}

void APRPlayerController::ClientOpenWaypointTravelUI_Implementation(bool bShowWorldResetButton)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	// 호스트 로컬 UI 표시
	UIControllerComponent->OpenWaypointTravel(bShowWorldResetButton);
}

void APRPlayerController::ClientOpenFaerinEncounterChoice_Implementation(APRFaerinEncounterDirector* Director)
{
	if (!IsLocalController() || !IsValid(UIControllerComponent))
	{
		return;
	}

	SetLocalBGMState(this, EPRBGMState::BossDialogue);
	SetFaerinEncounterHUDVisibleLocal(false);
	UIControllerComponent->OpenFaerinEncounterChoice(Director);
}

void APRPlayerController::ClientCloseFaerinEncounterChoice_Implementation(bool bRestoreDefaultBGM)
{
	if (!IsLocalController() || !IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->CloseFaerinEncounterChoice();
	if (bRestoreDefaultBGM)
	{
		SetFaerinEncounterHUDVisibleLocal(true);
		RestoreLocalDefaultBGM(this);
	}
}

void APRPlayerController::ShowFaerinSubtitleLocal(APRFaerinEncounterDirector* Director, FName DialogueNodeId)
{
	if (!IsLocalController() || !IsValid(UIControllerComponent) || !IsValid(Director))
	{
		return;
	}

	FText SpeakerText;
	FText BodyText;
	if (!Director->ResolveDialogueSubtitleText(DialogueNodeId, SpeakerText, BodyText))
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] Client resolve failed node=%s -> hide"), *DialogueNodeId.ToString());
		UIControllerComponent->HideFaerinEncounterSubtitle();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] Client show node=%s body=%s"), *DialogueNodeId.ToString(), *BodyText.ToString());
	UIControllerComponent->ShowFaerinEncounterSubtitle(SpeakerText, BodyText);
}

void APRPlayerController::ShowFaerinSubtitleTextLocal(const FText& SpeakerText, const FText& BodyText)
{
	if (!IsLocalController() || !IsValid(UIControllerComponent))
	{
		return;
	}

	if (BodyText.IsEmpty())
	{
		UIControllerComponent->HideFaerinEncounterSubtitle();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] Client show direct body=%s"), *BodyText.ToString());
	UIControllerComponent->ShowFaerinEncounterSubtitle(SpeakerText, BodyText);
}

void APRPlayerController::ClientShowFaerinSubtitle_Implementation(APRFaerinEncounterDirector* Director, FName DialogueNodeId)
{
	ShowFaerinSubtitleLocal(Director, DialogueNodeId);
}

void APRPlayerController::HideFaerinSubtitleLocal()
{
	if (!IsLocalController() || !IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->HideFaerinEncounterSubtitle();
}

void APRPlayerController::ClientHideFaerinSubtitle_Implementation()
{
	HideFaerinSubtitleLocal();
}

void APRPlayerController::PlayFaerinEncounterSequenceLocal(APRFaerinEncounterDirector* Director, EFaerinEncounterSequence SequenceType)
{
	if (!IsLocalController() || !IsValid(Director))
	{
		return;
	}

	switch (SequenceType)
	{
	case EFaerinEncounterSequence::Intro:
		SetLocalBGMState(this, EPRBGMState::BossIntroCutscene);
		break;
	case EFaerinEncounterSequence::FightStart:
		SetLocalBGMState(this, EPRBGMState::BossFightStartCutscene);
		break;
	default:
		break;
	}

	SetFaerinEncounterHUDVisibleLocal(false);
	Director->PlayEncounterSequenceForLocalAudience(SequenceType);
}

void APRPlayerController::ClientPlayFaerinEncounterSequence_Implementation(APRFaerinEncounterDirector* Director, EFaerinEncounterSequence SequenceType)
{
	PlayFaerinEncounterSequenceLocal(Director, SequenceType);
}

void APRPlayerController::StopFaerinEncounterSequenceLocal(APRFaerinEncounterDirector* Director, FName Reason)
{
	if (!IsLocalController() || !IsValid(Director))
	{
		return;
	}

	Director->StopEncounterSequenceForLocalAudience(Reason);
	RestoreFaerinEncounterViewTargetLocal(0.0f);
	SetEncounterInputLockLocal(false);
	SetFaerinEncounterHUDVisibleLocal(true);
	HideFaerinSubtitleLocal();
	RestoreLocalDefaultBGM(this);
}

void APRPlayerController::ClientStopFaerinEncounterSequence_Implementation(APRFaerinEncounterDirector* Director, FName Reason)
{
	StopFaerinEncounterSequenceLocal(Director, Reason);
}

void APRPlayerController::ServerNotifyFaerinDialogueNodePresented_Implementation(APRFaerinEncounterDirector* Director, FName DialogueNodeId)
{
	if (!IsValid(Director))
	{
		return;
	}

	Director->NotifyDialogueNodePresentedFromClient(this, DialogueNodeId);
}

void APRPlayerController::ClientNotifyShopBuyResult_Implementation(const FPRShopBuyResult& Result)
{
	OnShopBuyResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyShopSellResult_Implementation(const FPRShopSellResult& Result)
{
	OnShopSellResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyTraitInvestmentResult_Implementation(const FPRTraitInvestmentResult& Result)
{
	OnTraitInvestmentResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyPlayerLevelUp_Implementation(int32 PreviousLevel, int32 CurrentLevel)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ShowLevelUpPopup(PreviousLevel, CurrentLevel);
}

void APRPlayerController::ClientNotifyPickupReward_Implementation(const FPRPickupNotificationPayload& Payload)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ShowPickupRewardNotification(Payload);
}

void APRPlayerController::ClientNotifyHUDMessage_Implementation(EPRHUDMessageType MessageType)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	// 네트워크 수신 후 로컬 UI 계층에 HUD 메시지 처리 위임
	UIControllerComponent->NotifyHUDMessage(MessageType);
}

void APRPlayerController::RequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(UpgradeComponent) || !IsValid(WeaponItem))
	{
		return;
	}

	ServerRequestUpgradeWeapon(UpgradeComponent, WeaponItem);
}

void APRPlayerController::RequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || EntryId.IsNone() || Quantity <= 0)
	{
		return;
	}

	ServerRequestBuyShopItem(ShopComponent, EntryId, Quantity);
}

void APRPlayerController::RequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || EntryId.IsNone() || Quantity <= 0)
	{
		return;
	}

	ServerRequestSellShopItem(ShopComponent, EntryId, Quantity);
}

void APRPlayerController::RequestWaypointTravel(const FPRWaypointKey& WaypointKey)
{
	if (!WaypointKey.IsValid())
	{
		return;
	}

	// 서버 목적지 검증 요청
	ServerRequestWaypointTravel(WaypointKey);
}

void APRPlayerController::RequestCancelWaypointTravel()
{
	// 서버 취소 이벤트 요청
	ServerRequestCancelWaypointTravel();
}

void APRPlayerController::SetPendingWaypointTravelInteraction(UPRInteraction_Waypoint* WaypointInteraction)
{
	if (!HasAuthority() || !IsValid(WaypointInteraction))
	{
		return;
	}

	// UI 입력 대기 중 ActiveAction 해제 대비 서버 참조 보관
	PendingWaypointTravelInteraction = WaypointInteraction;
}

void APRPlayerController::ClearPendingWaypointTravelInteraction(const UPRInteraction_Waypoint* WaypointInteraction)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!PendingWaypointTravelInteraction.IsValid() || PendingWaypointTravelInteraction.Get() == WaypointInteraction)
	{
		// 완료 또는 취소된 UI 대기 참조 정리
		PendingWaypointTravelInteraction.Reset();
	}
}

void APRPlayerController::RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	ServerRequestConfirmTraitInvestment(DesiredInvestment);
}

void APRPlayerController::RequestResetTraitInvestment()
{
	ServerRequestResetTraitInvestment();
}

void APRPlayerController::ServerChooseFaerinEncounterFight_Implementation(APRFaerinEncounterDirector* Director)
{
	APRPlayerCharacter* RequestingPlayer = Cast<APRPlayerCharacter>(GetPawn());
	if (!IsValid(Director) || !IsValid(RequestingPlayer))
	{
		return;
	}

	Director->ChooseFight(RequestingPlayer);
}

void APRPlayerController::ServerChooseFaerinEncounterDecline_Implementation(APRFaerinEncounterDirector* Director)
{
	APRPlayerCharacter* RequestingPlayer = Cast<APRPlayerCharacter>(GetPawn());
	if (!IsValid(Director) || !IsValid(RequestingPlayer))
	{
		return;
	}

	Director->ChooseDecline(RequestingPlayer);
}

void APRPlayerController::ServerRequestUpgradeWeapon_Implementation(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(UpgradeComponent) || !IsValid(UpgradeComponent->GetOwner()))
	{
		FPRWeaponUpgradeResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRWeaponUpgradeFailReason::InvalidUpgradeStation;
		Result.UpgradeComponent = UpgradeComponent;
		Result.WeaponItem = WeaponItem;
		Result.UpgradeLevel = IsValid(WeaponItem) ? WeaponItem->GetUpgradeLevel() : 0;
		ClientNotifyWeaponUpgradeResult(Result);
		return;
	}

	UpgradeComponent->RequestUpgradeWeapon(this, WeaponItem);
}

void APRPlayerController::ServerRequestBuyShopItem_Implementation(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || !IsValid(ShopComponent->GetOwner()))
	{
		FPRShopBuyResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRShopBuyFailReason::InvalidShopData;
		Result.ShopComponent = ShopComponent;
		Result.EntryId = EntryId;
		Result.Quantity = Quantity;
		ClientNotifyShopBuyResult(Result);
		return;
	}

	ShopComponent->RequestBuyItem(this, EntryId, Quantity);
}

void APRPlayerController::ServerRequestSellShopItem_Implementation(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || !IsValid(ShopComponent->GetOwner()))
	{
		FPRShopSellResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRShopSellFailReason::InvalidShopData;
		Result.ShopComponent = ShopComponent;
		Result.EntryId = EntryId;
		Result.Quantity = Quantity;
		ClientNotifyShopSellResult(Result);
		return;
	}

	ShopComponent->RequestSellItem(this, EntryId, Quantity);
}

void APRPlayerController::ServerRequestWaypointTravel_Implementation(FPRWaypointKey WaypointKey)
{
	if (!IsHostControllerForWaypointTravel())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: requester is not host"));
		return;
	}

	UPRInteraction_Waypoint* WaypointInteraction = ResolveWaypointTravelInteraction();
	if (!IsValid(WaypointInteraction))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: waypoint interaction not found"));
		return;
	}

	// 웨이포인트 상호작용에 이동 위임
	WaypointInteraction->RequestWaypointTravel(this, WaypointKey);
}

void APRPlayerController::ServerRequestCancelWaypointTravel_Implementation()
{
	if (!IsHostControllerForWaypointTravel())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel cancel rejected: requester is not host"));
		return;
	}

	UPRInteraction_Waypoint* WaypointInteraction = ResolveWaypointTravelInteraction();
	if (!IsValid(WaypointInteraction))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel cancel rejected: waypoint interaction not found"));
		return;
	}

	// 웨이포인트 상호작용에 취소 위임
	WaypointInteraction->CancelWaypointTravel(this);
}

void APRPlayerController::ServerAcknowledgeMapLoadingScreen_Implementation(const FString& MapName)
{
	if (MapName.IsEmpty())
	{
		return;
	}

	LastAcknowledgedLoadingScreenMapName = MapName;
}

void APRPlayerController::ServerRequestConfirmTraitInvestment_Implementation(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	UPRPlayerGrowthComponent* GrowthComponent = IsValid(PRPlayerState) ? PRPlayerState->GetGrowthComponent() : nullptr;
	FPRTraitInvestmentResult Result;
	if (!IsValid(GrowthComponent))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidGrowthComponent;
		ClientNotifyTraitInvestmentResult(Result);
		return;
	}

	Result = GrowthComponent->ConfirmTraitInvestment(DesiredInvestment);
	ClientNotifyTraitInvestmentResult(Result);
}

void APRPlayerController::ServerRequestResetTraitInvestment_Implementation()
{
	APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	UPRPlayerGrowthComponent* GrowthComponent = IsValid(PRPlayerState) ? PRPlayerState->GetGrowthComponent() : nullptr;
	FPRTraitInvestmentResult Result;
	if (!IsValid(GrowthComponent))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidGrowthComponent;
		ClientNotifyTraitInvestmentResult(Result);
		return;
	}

	Result = GrowthComponent->ResetTraitInvestment();
	ClientNotifyTraitInvestmentResult(Result);
}

// ===== UI =====
void APRPlayerController::OnInventoryInputStarted()
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}
    // 인벤토리 UI 토글 
	UIControllerComponent->ToggleInventory();
}

void APRPlayerController::OnTraitWindowInputStarted()
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ToggleTraitWindow();
}

void APRPlayerController::OnInGameMenuInputStarted()
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ToggleInGameMenu();
}

void APRPlayerController::OnQuickSlotInputStarted(int32 SlotIndex)
{
	if (UPRQuickSlotComponent* QuickSlotComponent = GetQuickSlotComponent())
	{
		QuickSlotComponent->RequestUseQuickSlot(SlotIndex);
	}
}

UPRQuickSlotComponent* APRPlayerController::GetQuickSlotComponent() const
{
	const APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetQuickSlotComponent();
}

UPRInteraction_Waypoint* APRPlayerController::ResolveWaypointTravelInteraction() const
{
	const UPRInteractorComponent* ActiveInteractorComponent = IsValid(InteractorComponent)
		? InteractorComponent.Get()
		: FindComponentByClass<UPRInteractorComponent>();
	if (IsValid(ActiveInteractorComponent))
	{
		if (UPRInteraction_Waypoint* ActiveWaypointInteraction = Cast<UPRInteraction_Waypoint>(ActiveInteractorComponent->GetActiveAction()))
		{
			// 입력 유지 중인 활성 상호작용 우선 사용
			return ActiveWaypointInteraction;
		}
	}

	UPRInteraction_Waypoint* PendingWaypointInteraction = PendingWaypointTravelInteraction.Get();
	if (IsValid(PendingWaypointInteraction) && PendingWaypointInteraction->IsWaitingForWaypointTravelSelection())
	{
		// UI 표시 이후 입력 해제에 따른 ActiveAction 소실 대비
		return PendingWaypointInteraction;
	}

	return nullptr;
}

bool APRPlayerController::IsHostControllerForWaypointTravel() const
{
	// 리슨 서버 호스트 또는 스탠드얼론 호스트 판정
	return HasAuthority() && IsLocalController();
}
