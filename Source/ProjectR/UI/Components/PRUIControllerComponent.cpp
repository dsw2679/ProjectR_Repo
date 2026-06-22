// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (레벨업, 픽업 알림 및 피격 피플래시 UI 호출 관리 구현)
// Author: 배유찬 (온라인 세션, 패스트 트래블 지도 및 테스트 UI 호출 관리 구현)
// Author: 손승우 (보스전 체력바 UI 호출 연동 구현)
// Author: 이건주 (인벤토리 메인 위젯 및 캐릭터 3D 프리뷰 호출 구현)
#include "PRUIControllerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/UI/HUD/PRHUDWidget.h"
#include "ProjectR/UI/InGameMenu/PRInGameMenuWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryWidget.h"
#include "ProjectR/UI/Inventory/PRItemTooltipWidget.h"
#include "ProjectR/UI/Inventory/PRItemTooltipViewDataBuilder.h"
#include "ProjectR/UI/Faerin/PRFaerinEncounterChoiceWidget.h"
#include "ProjectR/UI/Faerin/PRFaerinEncounterSubtitleWidget.h"
#include "ProjectR/UI/PlayerMenu/PRPlayerMenu.h"
#include "ProjectR/UI/Growth/PRTraitWindowWidget.h"
#include "ProjectR/UI/Option/PROptionWidget.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"
#include "ProjectR/UI/Shop/PRShopWidget.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelWidget.h"
#include "ProjectR/UI/WeaponUpgrade/PRWeaponUpgradeWidget.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/System/PRAssetManager.h"
#include "Sound/SoundBase.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	// HUD 메시지 타입별 표시 문구 해석
	FText ResolveHUDMessageText(EPRHUDMessageType MessageType)
	{
		switch (MessageType)
		{
		case EPRHUDMessageType::WaitingForOtherPlayers:
			return NSLOCTEXT("ProjectR", "HUDMessage_WaitingForOtherPlayers", "다른 플레이어를 기다리는 중");
		case EPRHUDMessageType::OtherPlayersWaiting:
			return NSLOCTEXT("ProjectR", "HUDMessage_OtherPlayersWaiting", "다른 플레이어들이 기다리는 중");
		case EPRHUDMessageType::MapTravelInProgress:
			return NSLOCTEXT("ProjectR", "HUDMessage_MapTravelInProgress", "이동 중");
		default:
			return FText::GetEmpty();
		}
	}

	constexpr int32 DamageFXZOrder = 9000;
	const FName DamageFXCreateFunctionName(TEXT("CreateDamageFX"));
	const FName DamageFXColor1ParameterName(TEXT("DamageColor1"));
	const FName DamageFXColor2ParameterName(TEXT("DamageColor2"));
	const FName DamageFXFadeInDurationParameterName(TEXT("FadeInDuration"));
	const FName DamageFXFadeOutDurationParameterName(TEXT("FadeOutDuration"));
	const FName DamageFXForceRemoveParameterName(TEXT("ForceRemove"));
	const FName DamageFXPulseLoopParameterName(TEXT("PulseLoop"));
	const FName DamageFXPulseOpacityMinMaxParameterName(TEXT("PulseOpacityMinMax"));

	FProperty* FindDamageFXParameter(UFunction* Function, FName ParameterName)
	{
		if (!IsValid(Function))
		{
			return nullptr;
		}

		FProperty* ParameterProperty = Function->FindPropertyByName(ParameterName);
		if (ParameterProperty == nullptr || !ParameterProperty->HasAnyPropertyFlags(CPF_Parm))
		{
			return nullptr;
		}

		return ParameterProperty;
	}

	void SetDamageFXScalarParameter(UFunction* Function, void* Parameters, FName ParameterName, double Value)
	{
		FProperty* ParameterProperty = FindDamageFXParameter(Function, ParameterName);
		if (ParameterProperty == nullptr || Parameters == nullptr)
		{
			return;
		}

		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(ParameterProperty))
		{
			FloatProperty->SetPropertyValue_InContainer(Parameters, static_cast<float>(Value));
			return;
		}

		if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(ParameterProperty))
		{
			DoubleProperty->SetPropertyValue_InContainer(Parameters, Value);
			return;
		}

		if (FIntProperty* IntProperty = CastField<FIntProperty>(ParameterProperty))
		{
			IntProperty->SetPropertyValue_InContainer(Parameters, static_cast<int32>(Value));
			return;
		}

		if (FByteProperty* ByteProperty = CastField<FByteProperty>(ParameterProperty))
		{
			ByteProperty->SetPropertyValue_InContainer(Parameters, static_cast<uint8>(Value));
		}
	}

	void SetDamageFXBoolParameter(UFunction* Function, void* Parameters, FName ParameterName, bool bValue)
	{
		FProperty* ParameterProperty = FindDamageFXParameter(Function, ParameterName);
		if (ParameterProperty == nullptr || Parameters == nullptr)
		{
			return;
		}

		if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(ParameterProperty))
		{
			BoolProperty->SetPropertyValue_InContainer(Parameters, bValue);
		}
	}

	void SetDamageFXLinearColorParameter(UFunction* Function, void* Parameters, FName ParameterName, const FLinearColor& Value)
	{
		FProperty* ParameterProperty = FindDamageFXParameter(Function, ParameterName);
		if (ParameterProperty == nullptr || Parameters == nullptr)
		{
			return;
		}

		FStructProperty* StructProperty = CastField<FStructProperty>(ParameterProperty);
		if (StructProperty == nullptr || StructProperty->Struct != TBaseStructure<FLinearColor>::Get())
		{
			return;
		}

		*StructProperty->ContainerPtrToValuePtr<FLinearColor>(Parameters) = Value;
	}

	void SetDamageFXVector2DParameter(UFunction* Function, void* Parameters, FName ParameterName, const FVector2D& Value)
	{
		FProperty* ParameterProperty = FindDamageFXParameter(Function, ParameterName);
		if (ParameterProperty == nullptr || Parameters == nullptr)
		{
			return;
		}

		FStructProperty* StructProperty = CastField<FStructProperty>(ParameterProperty);
		if (StructProperty == nullptr || StructProperty->Struct != TBaseStructure<FVector2D>::Get())
		{
			return;
		}

		*StructProperty->ContainerPtrToValuePtr<FVector2D>(Parameters) = Value;
	}

	void ApplyDamageFXParameters(UFunction* Function, void* Parameters, const FPRDamageFXSettings& DamageFXSettings)
	{
		SetDamageFXLinearColorParameter(Function, Parameters, DamageFXColor1ParameterName, DamageFXSettings.DamageColor1);
		SetDamageFXLinearColorParameter(Function, Parameters, DamageFXColor2ParameterName, DamageFXSettings.DamageColor2);
		SetDamageFXScalarParameter(Function, Parameters, DamageFXFadeInDurationParameterName, DamageFXSettings.FadeInDuration);
		SetDamageFXScalarParameter(Function, Parameters, DamageFXFadeOutDurationParameterName, DamageFXSettings.FadeOutDuration);
		SetDamageFXBoolParameter(Function, Parameters, DamageFXForceRemoveParameterName, DamageFXSettings.bForceRemove);
		SetDamageFXBoolParameter(Function, Parameters, DamageFXPulseLoopParameterName, DamageFXSettings.bPulseLoop);
		SetDamageFXVector2DParameter(Function, Parameters, DamageFXPulseOpacityMinMaxParameterName, DamageFXSettings.PulseOpacityMinMax);
	}
}

UPRUIControllerComponent::UPRUIControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	WeakDamageFXSettings.PulseOpacityMinMax = FVector2D(0.15f, 0.55f);
	WeakDamageFXSettings.FadeOutDuration = 0.25f;

	StrongDamageFXSettings.PulseOpacityMinMax = FVector2D(0.25f, 0.85f);
	StrongDamageFXSettings.FadeInDuration = 0.05f;
	StrongDamageFXSettings.FadeOutDuration = 0.35f;

	DownDamageFXSettings.PulseOpacityMinMax = FVector2D(0.35f, 1.0f);
	DownDamageFXSettings.FadeInDuration = 0.08f;
	DownDamageFXSettings.FadeOutDuration = 0.55f;
}

void UPRUIControllerComponent::ToggleInventory()
{
	// 플레이어 메뉴 인벤토리 탭 위임
	TogglePlayerMenu(EPRPlayerMenuTabType::Inventory);
}

void UPRUIControllerComponent::ToggleBag()
{
	// 플레이어 메뉴 가방 탭 토글
	TogglePlayerMenu(EPRPlayerMenuTabType::Bag);
}

void UPRUIControllerComponent::CloseInventory()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	HideItemTooltip();

	if (IsValid(PlayerMenuWidget) && PlayerMenuWidget->IsInViewport()
		&& PlayerMenuWidget->GetSelectedRuntimeTabType() == EPRPlayerMenuTabType::Inventory)
	{
		// 플레이어 메뉴 인벤토리 탭 닫기
		ClosePlayerMenu();
	}

	if (!IsValid(InventoryWidget) || !InventoryWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(InventoryWidget);
	}
	else
	{
		InventoryWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::CloseBag()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	HideItemTooltip();

	if (IsValid(PlayerMenuWidget) && PlayerMenuWidget->IsInViewport()
		&& PlayerMenuWidget->GetSelectedRuntimeTabType() == EPRPlayerMenuTabType::Bag)
	{
		// 플레이어 메뉴 가방 탭 닫기
		ClosePlayerMenu();
	}
}

void UPRUIControllerComponent::ToggleTraitWindow()
{
	// 플레이어 메뉴 특성 탭 위임
	TogglePlayerMenu(EPRPlayerMenuTabType::Trait);
}

void UPRUIControllerComponent::CloseTraitWindow()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (IsValid(PlayerMenuWidget) && PlayerMenuWidget->IsInViewport()
		&& PlayerMenuWidget->GetSelectedRuntimeTabType() == EPRPlayerMenuTabType::Trait)
	{
		// 플레이어 메뉴 특성 탭 닫기
		ClosePlayerMenu();
	}

	if (!IsValid(TraitWindowWidget) || !TraitWindowWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(TraitWindowWidget);
	}
	else
	{
		TraitWindowWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ToggleInGameMenu()
{
	// 플레이어 메뉴 인게임 메뉴 탭 위임
	TogglePlayerMenu(EPRPlayerMenuTabType::InGameMenu);
}

void UPRUIControllerComponent::TogglePlayerMenu(EPRPlayerMenuTabType TargetTabType)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	if (IsValid(PlayerMenuWidget) && PlayerMenuWidget->IsInViewport())
	{
		if (PlayerMenuWidget->GetSelectedRuntimeTabType() == TargetTabType)
		{
			// 동일 탭 토글 닫기
			UIManager->PopUI(PlayerMenuWidget);
			return;
		}

		// 요청 탭 전환
		PlayerMenuWidget->SelectRuntimeTab(TargetTabType);
		return;
	}

	UPRPlayerMenu* CreatedPlayerMenuWidget = GetOrCreatePlayerMenuWidget();
	if (!IsValid(CreatedPlayerMenuWidget))
	{
		return;
	}

	UPRInventoryComponent* InventoryComponent = GetInventoryComponent();
	UPRWeaponManagerComponent* WeaponManagerComponent = GetWeaponManagerComponent();
	UPRQuickSlotComponent* QuickSlotComponent = GetQuickSlotComponent();
	UPREquipmentManagerComponent* EquipmentManagerComponent = GetEquipmentManagerComponent();
	APlayerController* PlayerController = GetOwningPlayerController();
	APRPlayerState* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<APRPlayerState>() : nullptr;
	if ((TargetTabType == EPRPlayerMenuTabType::Inventory || TargetTabType == EPRPlayerMenuTabType::Bag)
		&& (!IsValid(InventoryComponent) || !IsValid(QuickSlotComponent)))
	{
		// 인벤토리와 가방 공통 소스 검증
		return;
	}

	if (TargetTabType == EPRPlayerMenuTabType::Inventory
		&& (!IsValid(WeaponManagerComponent) || !IsValid(EquipmentManagerComponent)))
	{
		// 인벤토리 장비 편성 소스 검증
		return;
	}

	if (TargetTabType == EPRPlayerMenuTabType::Trait && !IsValid(PlayerState))
	{
		// 특성 탭 소스 검증
		return;
	}

	// 플레이어 메뉴 소스 전달
	CreatedPlayerMenuWidget->SetMenuSources(InventoryComponent, WeaponManagerComponent, QuickSlotComponent, EquipmentManagerComponent, PlayerState);
	if (!CreatedPlayerMenuWidget->RefreshRuntimeTabs(TargetTabType))
	{
		return;
	}

	UIManager->PushUIInstance(CreatedPlayerMenuWidget);
}

void UPRUIControllerComponent::CloseInGameMenu()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (IsValid(PlayerMenuWidget) && PlayerMenuWidget->IsInViewport()
		&& PlayerMenuWidget->GetSelectedRuntimeTabType() == EPRPlayerMenuTabType::InGameMenu)
	{
		// 플레이어 메뉴 인게임 메뉴 탭 닫기
		ClosePlayerMenu();
	}

	if (!IsValid(InGameMenuWidget) || !InGameMenuWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(InGameMenuWidget);
	}
	else
	{
		InGameMenuWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::OpenOption()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	if (IsValid(OptionWidget) && OptionWidget->IsInViewport())
	{
		return;
	}

	UPROptionWidget* CreatedOptionWidget = GetOrCreateOptionWidget();
	if (!IsValid(CreatedOptionWidget))
	{
		return;
	}

	UIManager->PushUIInstance(CreatedOptionWidget);
}

void UPRUIControllerComponent::CloseOption()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(OptionWidget) || !OptionWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(OptionWidget);
	}
	else
	{
		OptionWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ClosePlayerMenu()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(PlayerMenuWidget) || !PlayerMenuWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(PlayerMenuWidget);
	}
}

void UPRUIControllerComponent::OpenWeaponUpgrade(UPRWeaponUpgradeComponent* UpgradeComponent)
{
	if (!IsLocalPlayer() || !IsValid(UpgradeComponent))
	{
		return;
	}

	HideItemTooltip();
	CloseShop();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRWeaponUpgradeWidget* CreatedWeaponUpgradeWidget = GetOrCreateWeaponUpgradeWidget();
	if (!IsValid(CreatedWeaponUpgradeWidget))
	{
		return;
	}

	CreatedWeaponUpgradeWidget->SetUpgradeContext(UpgradeComponent);
	UIManager->PushUIInstance(CreatedWeaponUpgradeWidget);
}

void UPRUIControllerComponent::OpenShop(UPRShopComponent* ShopComponent)
{
	if (!IsLocalPlayer() || !IsValid(ShopComponent))
	{
		return;
	}

	HideItemTooltip();
	CloseWeaponUpgrade();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRShopWidget* CreatedShopWidget = GetOrCreateShopWidget();
	if (!IsValid(CreatedShopWidget))
	{
		return;
	}

	CreatedShopWidget->SetShopContext(ShopComponent);
	CreatedShopWidget->SetVisibility(ESlateVisibility::Visible);
	UIManager->PushUIInstance(CreatedShopWidget);
}

void UPRUIControllerComponent::PrewarmShopUI(const TArray<UPRShopComponent*>& ShopComponents, bool bRenderPrewarm, FSimpleDelegate OnComplete)
{
	if (!IsLocalPlayer())
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	UPRShopComponent* RepresentativeShopComponent = nullptr;
	for (UPRShopComponent* ShopComponent : ShopComponents)
	{
		if (IsValid(ShopComponent))
		{
			RepresentativeShopComponent = ShopComponent;
			break;
		}
	}

	if (!IsValid(RepresentativeShopComponent))
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	UPRShopWidget* CreatedShopWidget = GetOrCreateShopWidget();
	if (!IsValid(CreatedShopWidget))
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	CreatedShopWidget->SetShopContext(RepresentativeShopComponent);
	if (!bRenderPrewarm)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	const ESlateVisibility PreviousVisibility = CreatedShopWidget->GetVisibility();
	CreatedShopWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	CreatedShopWidget->AddToViewport(-1000);

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		CreatedShopWidget->RemoveFromParent();
		CreatedShopWidget->SetVisibility(PreviousVisibility);
		OnComplete.ExecuteIfBound();
		return;
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [WeakWidget = TWeakObjectPtr<UPRShopWidget>(CreatedShopWidget), PreviousVisibility, OnComplete]()
	{
		if (UPRShopWidget* ShopWidget = WeakWidget.Get())
		{
			ShopWidget->RemoveFromParent();
			ShopWidget->SetVisibility(PreviousVisibility);
		}

		OnComplete.ExecuteIfBound();
	});

	World->GetTimerManager().SetTimerForNextTick(TimerDelegate);
}

void UPRUIControllerComponent::OpenWaypointTravel(bool bShowWorldResetButton)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	HideItemTooltip();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRWaypointTravelWidget* CreatedWaypointTravelWidget = GetOrCreateWaypointTravelWidget();
	if (!IsValid(CreatedWaypointTravelWidget))
	{
		return;
	}

	// Waypoint Travel Overview 갱신
	CreatedWaypointTravelWidget->SetWorldResetButtonVisible(bShowWorldResetButton);
	CreatedWaypointTravelWidget->RebuildOverview();
	UIManager->PushUIInstance(CreatedWaypointTravelWidget);
}

void UPRUIControllerComponent::OpenFaerinEncounterChoice(APRFaerinEncounterDirector* Director)
{
	if (!IsLocalPlayer() || !IsValid(Director))
	{
		return;
	}

	HideItemTooltip();
	CloseInventory();
	CloseTraitWindow();
	CloseInGameMenu();
	CloseWeaponUpgrade();
	CloseShop();
	CloseWaypointTravel();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRFaerinEncounterChoiceWidget* CreatedChoiceWidget = GetOrCreateFaerinEncounterChoiceWidget();
	if (!IsValid(CreatedChoiceWidget))
	{
		return;
	}

	CreatedChoiceWidget->InitializeChoice(Director);
	UIManager->PushUIInstance(CreatedChoiceWidget);
}

void UPRUIControllerComponent::CloseWeaponUpgrade()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	HideItemTooltip();

	if (!IsValid(WeaponUpgradeWidget) || !WeaponUpgradeWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(WeaponUpgradeWidget);
	}
	else
	{
		WeaponUpgradeWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::CloseShop()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	HideItemTooltip();

	if (!IsValid(ShopWidget) || !ShopWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(ShopWidget);
	}
	else
	{
		ShopWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::CloseWaypointTravel()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(WaypointTravelWidget) || !WaypointTravelWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		// Travel UI 스택 제거
		UIManager->PopUI(WaypointTravelWidget);
	}
	else
	{
		WaypointTravelWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::CloseFaerinEncounterChoice()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(FaerinEncounterChoiceWidget) || !FaerinEncounterChoiceWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(FaerinEncounterChoiceWidget);
	}
	else
	{
		FaerinEncounterChoiceWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ShowFaerinEncounterSubtitle(const FText& SpeakerText, const FText& BodyText)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRFaerinEncounterSubtitleWidget* Widget = GetOrCreateFaerinEncounterSubtitleWidget();
	if (!IsValid(Widget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinSubtitle] Subtitle widget not created. Is FaerinEncounterSubtitleWidgetClass assigned on the PlayerController's UIControllerComponent?"));
		return;
	}

	// 선택 UI(UIManager 스택)와 달리 자막은 입력을 막으면 안 되므로 AddToViewport + 높은 ZOrder.
	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(9000);
	}

	Widget->SetSubtitle(SpeakerText, BodyText);
}

void UPRUIControllerComponent::HideFaerinEncounterSubtitle()
{
	if (!IsValid(FaerinEncounterSubtitleWidget))
	{
		return;
	}

	FaerinEncounterSubtitleWidget->ClearSubtitle();
	FaerinEncounterSubtitleWidget->RemoveFromParent();
}

void UPRUIControllerComponent::SetHUDVisible(bool bVisible)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	bWantsHUDVisible = bVisible;
	if (IsValid(HUDWidget))
	{
		HUDWidget->SetVisibility(bWantsHUDVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPRUIControllerComponent::ShowWeaponScope()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	bWantsWeaponScopeVisible = true;
	RefreshWeaponScopeWidget();

	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPRUIControllerComponent::HideWeaponScope()
{
	bWantsWeaponScopeVisible = false;

	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPRUIControllerComponent::ShowDamageFX(EPRDamageFXIntensity DamageFXIntensity)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UUserWidget* CreatedDamageFXWidget = GetOrCreateDamageFXWidget();
	if (!IsValid(CreatedDamageFXWidget))
	{
		return;
	}

	if (!CreatedDamageFXWidget->IsInViewport())
	{
		CreatedDamageFXWidget->AddToViewport(DamageFXZOrder);
	}

	CreatedDamageFXWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	InvokeDamageFXCreate(CreatedDamageFXWidget, ResolveDamageFXSettings(DamageFXIntensity));
}

void UPRUIControllerComponent::ShowLevelUpPopup(int32 PreviousLevel, int32 CurrentLevel)
{
	if (!IsLocalPlayer() || CurrentLevel <= PreviousLevel)
	{
		return;
	}

	if (IsValid(LevelUpSound))
	{
		UGameplayStatics::PlaySound2D(this, LevelUpSound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	if (IsValid(HUDWidget))
	{
		HUDWidget->ShowLevelUpPopup(PreviousLevel, CurrentLevel);
	}
}

void UPRUIControllerComponent::ShowPickupRewardNotification(const FPRPickupNotificationPayload& Payload)
{
	if (!IsLocalPlayer() || Payload.Quantity <= 0)
	{
		return;
	}

	if (USoundBase* PickupRewardSound = ResolvePickupRewardSound(Payload))
	{
		UGameplayStatics::PlaySound2D(this, PickupRewardSound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	if (IsValid(HUDWidget))
	{
		HUDWidget->ShowPickupRewardNotification(Payload);
	}
}

void UPRUIControllerComponent::NotifyHUDMessage(EPRHUDMessageType MessageType)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventManager))
	{
		return;
	}

	// HUD 메시지 위젯이 구독하는 로컬 EventManager 이벤트 발송
	FPRHUDMessageEventPayload Payload;
	Payload.bShow = MessageType != EPRHUDMessageType::None;
	Payload.Message = ResolveHUDMessageText(MessageType);
	EventManager->BroadcastTyped(PRGameplayTags::Event_Player_HUDMessage, Payload);
}

void UPRUIControllerComponent::ShowItemTooltip(UWidget* TooltipOwner, const FPRInventoryItemSlotViewData& SlotViewData)
{
	if (!IsLocalPlayer() || !IsValid(TooltipOwner) || !IsValid(SlotViewData.ItemData.Get()))
	{
		HideItemTooltip();
		return;
	}

	UPRItemTooltipWidget* TooltipWidget = GetOrCreateItemTooltipWidget();
	if (!IsValid(TooltipWidget))
	{
		return;
	}

	if (IsValid(ActiveTooltipOwner) && ActiveTooltipOwner != TooltipOwner)
	{
		ActiveTooltipOwner->SetToolTip(nullptr);
	}

	TooltipWidget->SetTooltipViewData(UPRItemTooltipViewDataBuilder::BuildTooltipViewData(SlotViewData));
	TooltipOwner->SetToolTip(TooltipWidget);
	ActiveTooltipOwner = TooltipOwner;
}

void UPRUIControllerComponent::HideItemTooltip()
{
	if (IsValid(ActiveTooltipOwner))
	{
		ActiveTooltipOwner->SetToolTip(nullptr);
	}

	ActiveTooltipOwner = nullptr;
}

void UPRUIControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseInventory();
	InventoryWidget = nullptr;
	CloseBag();
	CloseTraitWindow();
	TraitWindowWidget = nullptr;
	CloseWeaponUpgrade();
	WeaponUpgradeWidget = nullptr;
	CloseShop();
	ShopWidget = nullptr;
	CloseWaypointTravel();
	WaypointTravelWidget = nullptr;
	CloseOption();
	OptionWidget = nullptr;
	CloseInGameMenu();
	InGameMenuWidget = nullptr;
	CloseFaerinEncounterChoice();
	FaerinEncounterChoiceWidget = nullptr;
	HideFaerinEncounterSubtitle();
	FaerinEncounterSubtitleWidget = nullptr;
	ClosePlayerMenu();
	PlayerMenuWidget = nullptr;

	UnbindWeaponManager();
	RemoveWeaponScopeWidget();
	RemoveDamageFXWidget();
	RemoveItemTooltipWidget();
	TearDownHUDWidget();

	Super::EndPlay(EndPlayReason);
}

void UPRUIControllerComponent::RefreshForPawn(APawn* InPawn)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	// 새 폰이 없으면 HUD 위젯만 정리하고 종료
	if (!IsValid(InPawn))
	{
		UnbindWeaponManager();
		RemoveWeaponScopeWidget();
		TearDownHUDWidget();
		return;
	}

	// 기존 HUD 위젯 정리 후 새 인스턴스 생성. EventManager 바인딩이 새 위젯 NativeOnInitialized에서 다시 등록됨
	TearDownHUDWidget();
	CreateHUDWidget();

	BindWeaponManager(GetWeaponManagerComponent());
	RefreshWeaponScopeWidget();
}

void UPRUIControllerComponent::RemoveAllWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
	}
	if (WeaponScopeWidget)
	{
		WeaponScopeWidget->RemoveFromParent();
	}
	if (IsValid(DamageFXWidget))
	{
		DamageFXWidget->RemoveFromParent();
		DamageFXWidget = nullptr;
	}
	if (ItemTooltipWidget)
	{
		HideItemTooltip();
		ItemTooltipWidget = nullptr;
	}
	if (InGameMenuWidget)
	{
		InGameMenuWidget->RemoveFromParent();
	}
	if (OptionWidget)
	{
		OptionWidget->RemoveFromParent();
	}
	if (PlayerMenuWidget)
	{
		ClosePlayerMenu();
	}
	if (WaypointTravelWidget)
	{
		WaypointTravelWidget->RemoveFromParent();
	}
	if (FaerinEncounterChoiceWidget)
	{
		FaerinEncounterChoiceWidget->RemoveFromParent();
	}
	if (FaerinEncounterSubtitleWidget)
	{
		FaerinEncounterSubtitleWidget->RemoveFromParent();
	}
	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ResetSystem();
	}
}

void UPRUIControllerComponent::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	RefreshWeaponScopeWidget();
}

bool UPRUIControllerComponent::IsLocalPlayer() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	return IsValid(PlayerController) && PlayerController->IsLocalController();
}

APlayerController* UPRUIControllerComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

UPRInventoryComponent* UPRUIControllerComponent::GetInventoryComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetInventoryComponent();
}

UPRWeaponManagerComponent* UPRUIControllerComponent::GetWeaponManagerComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}
	
	if (APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(PlayerController->GetPawn()))
	{
		return Player->GetWeaponManager();
	}
	
	return nullptr;
}

UPRQuickSlotComponent* UPRUIControllerComponent::GetQuickSlotComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetQuickSlotComponent();
}

UPREquipmentManagerComponent* UPRUIControllerComponent::GetEquipmentManagerComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetEquipmentManagerComponent();
}

UPRUIManagerSubsystem* UPRUIControllerComponent::GetUIManager() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UPRUIManagerSubsystem>();
}

USoundBase* UPRUIControllerComponent::ResolvePickupRewardSound(const FPRPickupNotificationPayload& Payload) const
{
	const UPRItemDataAsset* ItemData = ResolvePickupRewardItemData(Payload);
	if (IsValid(ItemData))
	{
		if (const TObjectPtr<USoundBase>* RaritySound = PickupRewardSoundsByRarity.Find(ItemData->GetRarity()))
		{
			if (IsValid(RaritySound->Get()))
			{
				return RaritySound->Get();
			}
		}
	}

	USoundBase* DefaultPickupSound = DefaultPickupRewardSound.Get();
	return IsValid(DefaultPickupSound) ? DefaultPickupSound : nullptr;
}

UPRItemDataAsset* UPRUIControllerComponent::ResolvePickupRewardItemData(const FPRPickupNotificationPayload& Payload) const
{
	if (Payload.RewardType != EPRRewardType::Item && Payload.RewardType != EPRRewardType::Ammo)
	{
		return nullptr;
	}

	if (!Payload.ItemAssetId.IsValid())
	{
		return nullptr;
	}

	return UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Payload.ItemAssetId);
}

UPRInventoryWidget* UPRUIControllerComponent::GetOrCreateInventoryWidget()
{
	if (IsValid(InventoryWidget))
	{
		return InventoryWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(InventoryWidgetClass.Get()))
	{
		return nullptr;
	}

	InventoryWidget = CreateWidget<UPRInventoryWidget>(PlayerController, InventoryWidgetClass);
	return InventoryWidget;
}

UPRWeaponUpgradeWidget* UPRUIControllerComponent::GetOrCreateWeaponUpgradeWidget()
{
	if (IsValid(WeaponUpgradeWidget))
	{
		return WeaponUpgradeWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(WeaponUpgradeWidgetClass.Get()))
	{
		return nullptr;
	}

	WeaponUpgradeWidget = CreateWidget<UPRWeaponUpgradeWidget>(PlayerController, WeaponUpgradeWidgetClass);
	return WeaponUpgradeWidget;
}

UPRShopWidget* UPRUIControllerComponent::GetOrCreateShopWidget()
{
	if (IsValid(ShopWidget))
	{
		return ShopWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(ShopWidgetClass.Get()))
	{
		return nullptr;
	}

	ShopWidget = CreateWidget<UPRShopWidget>(PlayerController, ShopWidgetClass);
	return ShopWidget;
}

UPRWaypointTravelWidget* UPRUIControllerComponent::GetOrCreateWaypointTravelWidget()
{
	if (IsValid(WaypointTravelWidget))
	{
		return WaypointTravelWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(WaypointTravelWidgetClass.Get()))
	{
		return nullptr;
	}

	// Travel UI 인스턴스 생성
	WaypointTravelWidget = CreateWidget<UPRWaypointTravelWidget>(PlayerController, WaypointTravelWidgetClass);
	return WaypointTravelWidget;
}

UPRFaerinEncounterChoiceWidget* UPRUIControllerComponent::GetOrCreateFaerinEncounterChoiceWidget()
{
	if (IsValid(FaerinEncounterChoiceWidget))
	{
		return FaerinEncounterChoiceWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(FaerinEncounterChoiceWidgetClass.Get()))
	{
		return nullptr;
	}

	FaerinEncounterChoiceWidget = CreateWidget<UPRFaerinEncounterChoiceWidget>(PlayerController, FaerinEncounterChoiceWidgetClass);
	return FaerinEncounterChoiceWidget;
}

UPRFaerinEncounterSubtitleWidget* UPRUIControllerComponent::GetOrCreateFaerinEncounterSubtitleWidget()
{
	if (IsValid(FaerinEncounterSubtitleWidget))
	{
		return FaerinEncounterSubtitleWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(FaerinEncounterSubtitleWidgetClass.Get()))
	{
		return nullptr;
	}

	FaerinEncounterSubtitleWidget = CreateWidget<UPRFaerinEncounterSubtitleWidget>(PlayerController, FaerinEncounterSubtitleWidgetClass);
	return FaerinEncounterSubtitleWidget;
}

UPRTraitWindowWidget* UPRUIControllerComponent::GetOrCreateTraitWindowWidget()
{
	if (IsValid(TraitWindowWidget))
	{
		return TraitWindowWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(TraitWindowWidgetClass.Get()))
	{
		return nullptr;
	}

	TraitWindowWidget = CreateWidget<UPRTraitWindowWidget>(PlayerController, TraitWindowWidgetClass);
	return TraitWindowWidget;
}

UPRInGameMenuWidget* UPRUIControllerComponent::GetOrCreateInGameMenuWidget()
{
	if (IsValid(InGameMenuWidget))
	{
		return InGameMenuWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(InGameMenuWidgetClass.Get()))
	{
		return nullptr;
	}

	InGameMenuWidget = CreateWidget<UPRInGameMenuWidget>(PlayerController, InGameMenuWidgetClass);
	return InGameMenuWidget;
}

UPROptionWidget* UPRUIControllerComponent::GetOrCreateOptionWidget()
{
	if (IsValid(OptionWidget))
	{
		return OptionWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(OptionWidgetClass.Get()))
	{
		return nullptr;
	}

	OptionWidget = CreateWidget<UPROptionWidget>(PlayerController, OptionWidgetClass);
	return OptionWidget;
}

UPRPlayerMenu* UPRUIControllerComponent::GetOrCreatePlayerMenuWidget()
{
	if (IsValid(PlayerMenuWidget))
	{
		return PlayerMenuWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(PlayerMenuWidgetClass.Get()))
	{
		return nullptr;
	}

	PlayerMenuWidget = CreateWidget<UPRPlayerMenu>(PlayerController, PlayerMenuWidgetClass);
	return PlayerMenuWidget;
}

UPRItemTooltipWidget* UPRUIControllerComponent::GetOrCreateItemTooltipWidget()
{
	if (IsValid(ItemTooltipWidget))
	{
		return ItemTooltipWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(ItemTooltipWidgetClass.Get()))
	{
		return nullptr;
	}

	ItemTooltipWidget = CreateWidget<UPRItemTooltipWidget>(PlayerController, ItemTooltipWidgetClass);
	if (!IsValid(ItemTooltipWidget))
	{
		return nullptr;
	}

	return ItemTooltipWidget;
}

void UPRUIControllerComponent::RemoveItemTooltipWidget()
{
	HideItemTooltip();
	ItemTooltipWidget = nullptr;
}

void UPRUIControllerComponent::TearDownHUDWidget()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}

	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->PopUI(HUDWidget);
	}
	else if (HUDWidget->IsInViewport())
	{
		HUDWidget->RemoveFromParent();
	}

	HUDWidget = nullptr;
}

void UPRUIControllerComponent::CreateHUDWidget()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(HUDWidgetClass.Get()))
	{
		return;
	}

	HUDWidget = CreateWidget<UPRHUDWidget>(PlayerController, HUDWidgetClass);
	if (!IsValid(HUDWidget))
	{
		return;
	}

	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->PushUIInstance(HUDWidget);
	}
	else
	{
		// UIManager가 없는 예외 케이스에 대한 폴백
		HUDWidget->AddToViewport();
	}

	HUDWidget->SetVisibility(bWantsHUDVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPRUIControllerComponent::RefreshWeaponScopeWidget()
{
	UPRWeaponManagerComponent* WeaponManagerComponent = GetWeaponManagerComponent();
	if (!IsValid(WeaponManagerComponent))
	{
		RemoveWeaponScopeWidget();
		return;
	}

	const EPRWeaponSlotType CurrentWeaponSlot = WeaponManagerComponent->GetCurrentWeaponSlot();
	const UPRWeaponDataAsset* WeaponData = WeaponManagerComponent->GetWeaponDataBySlotType(CurrentWeaponSlot);
	TSubclassOf<UUserWidget> ScopeWidgetClass = IsValid(WeaponData) ? WeaponData->ScopeWidgetClass : nullptr;
	if (!IsValid(ScopeWidgetClass.Get()))
	{
		RemoveWeaponScopeWidget();
		return;
	}

	if (IsValid(WeaponScopeWidget) && CurrentScopeWidgetClass == ScopeWidgetClass)
	{
		WeaponScopeWidget->SetVisibility(bWantsWeaponScopeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		return;
	}

	RemoveWeaponScopeWidget();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}

	WeaponScopeWidget = CreateWidget<UUserWidget>(PlayerController, ScopeWidgetClass);
	if (!IsValid(WeaponScopeWidget))
	{
		return;
	}

	CurrentScopeWidgetClass = ScopeWidgetClass;
	WeaponScopeWidget->AddToViewport(-1);
	WeaponScopeWidget->SetVisibility(bWantsWeaponScopeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPRUIControllerComponent::RemoveWeaponScopeWidget()
{
	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->RemoveFromParent();
	}

	WeaponScopeWidget = nullptr;
	CurrentScopeWidgetClass = nullptr;
}

UUserWidget* UPRUIControllerComponent::GetOrCreateDamageFXWidget()
{
	if (IsValid(DamageFXWidget))
	{
		return DamageFXWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(DamageFXWidgetClass.Get()))
	{
		return nullptr;
	}

	DamageFXWidget = CreateWidget<UUserWidget>(PlayerController, DamageFXWidgetClass);
	return DamageFXWidget;
}

void UPRUIControllerComponent::InvokeDamageFXCreate(UUserWidget* InDamageFXWidget, const FPRDamageFXSettings& DamageFXSettings) const
{
	if (!IsValid(InDamageFXWidget))
	{
		return;
	}

	UFunction* DamageFXFunction = InDamageFXWidget->FindFunction(DamageFXCreateFunctionName);
	if (!IsValid(DamageFXFunction))
	{
		return;
	}

	if (DamageFXFunction->ParmsSize <= 0)
	{
		InDamageFXWidget->ProcessEvent(DamageFXFunction, nullptr);
		return;
	}

	FStructOnScope DamageFXParameters(DamageFXFunction);
	void* ParameterData = DamageFXParameters.GetStructMemory();
	ApplyDamageFXParameters(DamageFXFunction, ParameterData, DamageFXSettings);
	InDamageFXWidget->ProcessEvent(DamageFXFunction, ParameterData);
}

const FPRDamageFXSettings& UPRUIControllerComponent::ResolveDamageFXSettings(EPRDamageFXIntensity DamageFXIntensity) const
{
	switch (DamageFXIntensity)
	{
	case EPRDamageFXIntensity::Strong:
		return StrongDamageFXSettings;
	case EPRDamageFXIntensity::Down:
		return DownDamageFXSettings;
	case EPRDamageFXIntensity::Weak:
	default:
		return WeakDamageFXSettings;
	}
}

void UPRUIControllerComponent::RemoveDamageFXWidget()
{
	if (IsValid(DamageFXWidget))
	{
		DamageFXWidget->RemoveFromParent();
	}

	DamageFXWidget = nullptr;
}

void UPRUIControllerComponent::BindWeaponManager(UPRWeaponManagerComponent* WeaponManagerComponent)
{
	UnbindWeaponManager();

	if (!IsValid(WeaponManagerComponent))
	{
		return;
	}

	BoundWeaponManager = WeaponManagerComponent;
	BoundWeaponManager->GetOnWeaponEquipmentChanged().AddDynamic(this, &ThisClass::HandleWeaponEquipmentChanged);
}

void UPRUIControllerComponent::UnbindWeaponManager()
{
	if (IsValid(BoundWeaponManager))
	{
		BoundWeaponManager->GetOnWeaponEquipmentChanged().RemoveAll(this);
	}

	BoundWeaponManager = nullptr;
}
