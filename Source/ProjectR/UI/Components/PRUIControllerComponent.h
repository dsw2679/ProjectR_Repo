// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (레벨업, 픽업 알림 및 피격 피플래시 UI 호출 관리 구현)
// Author: 배유찬 (온라인 세션, 패스트 트래블 지도 및 테스트 UI 호출 관리 구현)
// Author: 손승우 (보스전 체력바 UI 호출 연동 구현)
// Author: 이건주 (인벤토리 메인 위젯 및 캐릭터 3D 프리뷰 호출 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/UI/HUD/PRHUDMessageTypes.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PlayerMenu/PRPlayerMenuTypes.h"
#include "PRUIControllerComponent.generated.h"

class UPRWidgetBase;
class APlayerController;
class APawn;
class APRFaerinEncounterDirector;
class UWidget;
class UUserWidget;
class UPREquipmentManagerComponent;
class UPRHUDWidget;
class UPRInGameMenuWidget;
class UPRInventoryComponent;
class UPRInventoryWidget;
class UPRItemDataAsset;
class UPRItemTooltipWidget;
class UPRFaerinEncounterChoiceWidget;
class UPRFaerinEncounterSubtitleWidget;
class UPROptionWidget;
class UPRPlayerMenu;
class UPRQuickSlotComponent;
class UPRShopComponent;
class UPRShopWidget;
class UPRTraitWindowWidget;
class UPRUIManagerSubsystem;
class UPRWaypointTravelWidget;
class UPRWeaponManagerComponent;
class UPRWeaponUpgradeComponent;
class UPRWeaponUpgradeWidget;
class USoundBase;

UENUM(BlueprintType)
enum class EPRDamageFXIntensity : uint8
{
	Weak,
	Strong,
	Down
};

USTRUCT(BlueprintType)
struct FPRDamageFXSettings
{
	GENERATED_BODY()

	// 첫 번째 피격 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX")
	FLinearColor DamageColor1 = FLinearColor(1.0f, 0.02f, 0.0f, 1.0f);

	// 두 번째 피격 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX")
	FLinearColor DamageColor2 = FLinearColor(0.45f, 0.0f, 0.0f, 1.0f);

	// 페이드 인 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.05f;

	// 페이드 아웃 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.35f;

	// 기존 재생 효과 강제 제거 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX")
	bool bForceRemove = true;

	// 펄스 반복 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX")
	bool bPulseLoop = false;

	// 펄스 투명도 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|HUD|FX")
	FVector2D PulseOpacityMinMax = FVector2D(0.25f, 1.0f);
};

// 플레이어 컨트롤러에 부착되어 로컬 플레이어 UI 표시 흐름을 관리한다
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRUIControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// UI 컨트롤러 컴포넌트 기본 설정을 초기화한다
	UPRUIControllerComponent();

	// 인벤토리 위젯을 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleInventory();

	// 가방 탭을 열거나 닫음
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleBag();

	// 특성 창 위젯을 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleTraitWindow();

	// 인게임 메뉴 위젯을 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleInGameMenu();

	// 플레이어 메뉴 위젯을 지정 탭으로 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void TogglePlayerMenu(EPRPlayerMenuTabType TargetTabType);

	// 인벤토리 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseInventory();

	// 가방 탭이 열려 있으면 닫음
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseBag();

	// 특성 창 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseTraitWindow();

	// 인게임 메뉴 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseInGameMenu();

	// 옵션 위젯을 연다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void OpenOption();

	// 옵션 위젯을 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseOption();

	// 플레이어 메뉴 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ClosePlayerMenu();

	// 강화 위젯을 열고 강화 컴포넌트 컨텍스트를 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void OpenWeaponUpgrade(UPRWeaponUpgradeComponent* UpgradeComponent);

	// 상점 위젯을 열고 상점 컴포넌트 컨텍스트를 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void OpenShop(UPRShopComponent* ShopComponent);

	// 로딩 화면 뒤에서 상점 위젯 생성과 선택적 렌더 프리웜
	void PrewarmShopUI(const TArray<UPRShopComponent*>& ShopComponents, bool bRenderPrewarm, FSimpleDelegate OnComplete);

	// 웨이포인트 Travel UI 열기
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void OpenWaypointTravel(bool bShowWorldResetButton = false);

	// Faerin 인카운터 선택 UI를 열고 Director 컨텍스트를 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void OpenFaerinEncounterChoice(APRFaerinEncounterDirector* Director);

	// 강화 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseWeaponUpgrade();

	// 상점 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseShop();

	// 웨이포인트 Travel UI 닫기
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseWaypointTravel();

	// Faerin 인카운터 선택 UI가 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseFaerinEncounterChoice();

	// Faerin 하단 자막을 표시/갱신한다. 선택 UI와 분리된 비입력 위젯이다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowFaerinEncounterSubtitle(const FText& SpeakerText, const FText& BodyText);

	// Faerin 하단 자막을 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void HideFaerinEncounterSubtitle();

	// 현재 캐시된 인벤토리 위젯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	UPRInventoryWidget* GetInventoryWidget() const { return InventoryWidget; }

	// 현재 HUD 위젯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	UPRHUDWidget* GetHUDWidget() const { return HUDWidget; }

	// 현재 HUD 위젯 표시 상태 변경
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void SetHUDVisible(bool bVisible);

	// 레벨업 팝업 표시를 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowLevelUpPopup(int32 PreviousLevel, int32 CurrentLevel);

	// 획득 보상 알림 표시를 현재 HUD에 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowPickupRewardNotification(const FPRPickupNotificationPayload& Payload);

	// HUD 안내 메시지 타입을 로컬 EventManager 이벤트로 발송한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void NotifyHUDMessage(EPRHUDMessageType MessageType);

	// 아이템 슬롯 툴팁을 지정 위젯에 연결한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI|Tooltip")
	void ShowItemTooltip(UWidget* TooltipOwner, const FPRInventoryItemSlotViewData& SlotViewData);

	// 현재 표시 중인 아이템 슬롯 툴팁을 숨긴다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI|Tooltip")
	void HideItemTooltip();

	// 장착 무기에 맞는 스코프 위젯을 표시한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowWeaponScope();

	// 현재 스코프 위젯을 숨긴다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void HideWeaponScope();

	// 피격 화면 이펙트 표시 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowDamageFX(EPRDamageFXIntensity DamageFXIntensity);

	// possession 시점에 의존하는 위젯을 재초기화한다
	void RefreshForPawn(APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void RemoveAllWidget();

protected:
	/*~ UActorComponent Interface ~*/
	// 컴포넌트 종료 시 생성한 위젯 참조를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot);

	// 소유 플레이어 컨트롤러가 로컬 컨트롤러인지 확인한다
	bool IsLocalPlayer() const;

	// 소유 플레이어 컨트롤러를 조회한다
	APlayerController* GetOwningPlayerController() const;

	// 플레이어 인벤토리 컴포넌트를 조회한다
	UPRInventoryComponent* GetInventoryComponent() const;

	// 현재 폰의 무기 매니저 컴포넌트를 조회한다
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const;

	// 플레이어 퀵슬롯 컴포넌트를 조회한다
	UPRQuickSlotComponent* GetQuickSlotComponent() const;

	// 플레이어 장비 매니저 컴포넌트를 조회한다
	UPREquipmentManagerComponent* GetEquipmentManagerComponent() const;

	// 로컬 플레이어 UI 매니저 서브시스템을 조회한다
	UPRUIManagerSubsystem* GetUIManager() const;

	// 인벤토리 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRInventoryWidget* GetOrCreateInventoryWidget();

	// 강화 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRWeaponUpgradeWidget* GetOrCreateWeaponUpgradeWidget();

	// 상점 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRShopWidget* GetOrCreateShopWidget();

	// 웨이포인트 Travel UI 인스턴스 생성 또는 캐시 반환
	UPRWaypointTravelWidget* GetOrCreateWaypointTravelWidget();

	// Faerin 인카운터 선택 UI 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRFaerinEncounterChoiceWidget* GetOrCreateFaerinEncounterChoiceWidget();

	// Faerin 하단 자막 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRFaerinEncounterSubtitleWidget* GetOrCreateFaerinEncounterSubtitleWidget();

	// 특성 창 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRTraitWindowWidget* GetOrCreateTraitWindowWidget();

	// 인게임 메뉴 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRInGameMenuWidget* GetOrCreateInGameMenuWidget();

	// 옵션 위젯 인스턴스 생성 또는 캐시 반환
	UPROptionWidget* GetOrCreateOptionWidget();

	// 플레이어 메뉴 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRPlayerMenu* GetOrCreatePlayerMenuWidget();

	// 아이템 툴팁 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRItemTooltipWidget* GetOrCreateItemTooltipWidget();

	// 아이템 툴팁 위젯 참조를 정리한다
	void RemoveItemTooltipWidget();

	// 보상 획득 효과음 선택
	USoundBase* ResolvePickupRewardSound(const FPRPickupNotificationPayload& Payload) const;

	// 보상 알림 아이템 데이터 조회
	UPRItemDataAsset* ResolvePickupRewardItemData(const FPRPickupNotificationPayload& Payload) const;

	// 현재 HUD 위젯을 UIManager에서 Pop하고 참조 정리
	void TearDownHUDWidget();

	// HUDWidgetClass로 HUD 위젯을 새로 생성하여 UIManager에 Push
	void CreateHUDWidget();

	// 현재 무기에 맞는 스코프 위젯을 준비한다
	void RefreshWeaponScopeWidget();

	// 스코프 위젯을 제거한다
	void RemoveWeaponScopeWidget();

	// 피격 화면 이펙트 위젯 인스턴스 생성 또는 캐시 반환
	UUserWidget* GetOrCreateDamageFXWidget();

	// 피격 화면 이펙트 위젯의 재생 이벤트 호출
	void InvokeDamageFXCreate(UUserWidget* InDamageFXWidget, const FPRDamageFXSettings& DamageFXSettings) const;

	// 피격 화면 이펙트 강도별 설정 반환
	const FPRDamageFXSettings& ResolveDamageFXSettings(EPRDamageFXIntensity DamageFXIntensity) const;

	// 피격 화면 이펙트 위젯 제거
	void RemoveDamageFXWidget();

	// 무기 장비 변경 델리게이트를 바인딩한다
	void BindWeaponManager(UPRWeaponManagerComponent* WeaponManagerComponent);

	// 무기 장비 변경 델리게이트 바인딩을 해제한다
	void UnbindWeaponManager();

protected:
	// 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Inventory")
	TSubclassOf<UPRInventoryWidget> InventoryWidgetClass;

	// 강화 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|WeaponUpgrade")
	TSubclassOf<UPRWeaponUpgradeWidget> WeaponUpgradeWidgetClass;

	// 상점 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Shop")
	TSubclassOf<UPRShopWidget> ShopWidgetClass;

	// 웨이포인트 Travel UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|WaypointTravel")
	TSubclassOf<UPRWaypointTravelWidget> WaypointTravelWidgetClass;

	// Faerin 인카운터 선택 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Faerin|Encounter")
	TSubclassOf<UPRFaerinEncounterChoiceWidget> FaerinEncounterChoiceWidgetClass;

	// Faerin 하단 자막 위젯 클래스 (WBP_FaerinEncounterSubtitle 지정)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Faerin|Encounter")
	TSubclassOf<UPRFaerinEncounterSubtitleWidget> FaerinEncounterSubtitleWidgetClass;

	// 특성 창 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Growth")
	TSubclassOf<UPRTraitWindowWidget> TraitWindowWidgetClass;

	// 인게임 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|InGameMenu")
	TSubclassOf<UPRInGameMenuWidget> InGameMenuWidgetClass;

	// 옵션 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Option")
	TSubclassOf<UPROptionWidget> OptionWidgetClass;

	// 플레이어 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|PlayerMenu")
	TSubclassOf<UPRPlayerMenu> PlayerMenuWidgetClass;

	// 생성 후 재사용할 상점 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRShopWidget> ShopWidget;

	// 생성 후 재사용할 웨이포인트 Travel UI 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRWaypointTravelWidget> WaypointTravelWidget;

	// 생성 후 재사용할 Faerin 인카운터 선택 UI 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinEncounterChoiceWidget> FaerinEncounterChoiceWidget;

	// 생성 후 재사용할 Faerin 하단 자막 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinEncounterSubtitleWidget> FaerinEncounterSubtitleWidget;

	// HUD 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD")
	TSubclassOf<UPRHUDWidget> HUDWidgetClass;

	// 레벨업 팝업 표시 효과음
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|Growth")
	TObjectPtr<USoundBase> LevelUpSound = nullptr;

	// 피격 화면 이펙트 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|FX")
	TSubclassOf<UUserWidget> DamageFXWidgetClass;

	// 약한 피격 화면 이펙트 설정
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|FX")
	FPRDamageFXSettings WeakDamageFXSettings;

	// 강한 피격 화면 이펙트 설정
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|FX")
	FPRDamageFXSettings StrongDamageFXSettings;

	// 다운 피격 화면 이펙트 설정
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|FX")
	FPRDamageFXSettings DownDamageFXSettings;

	// 레어도별 보상 획득 효과음
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|Pickup")
	TMap<EPRItemRarity, TObjectPtr<USoundBase>> PickupRewardSoundsByRarity;

	// 레어도 사운드 미설정 시 사용할 보상 획득 효과음
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|Pickup")
	TObjectPtr<USoundBase> DefaultPickupRewardSound = nullptr;

	// 아이템 툴팁 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Tooltip")
	TSubclassOf<UPRItemTooltipWidget> ItemTooltipWidgetClass;

private:
	// 생성 후 재사용할 인벤토리 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryWidget> InventoryWidget;

	// 현재 활성 HUD 위젯 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UPRHUDWidget> HUDWidget;

	// 장착 무기 데이터로 생성한 스코프 위젯
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WeaponScopeWidget;

	// 피격 화면 이펙트 위젯 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DamageFXWidget;

	// 아이템 툴팁 표시를 담당하는 전역 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRItemTooltipWidget> ItemTooltipWidget;

	// 현재 툴팁을 연결한 위젯
	UPROPERTY(Transient)
	TObjectPtr<UWidget> ActiveTooltipOwner;

	// 생성 후 재사용할 강화 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRWeaponUpgradeWidget> WeaponUpgradeWidget;

	// 생성 후 재사용할 특성 창 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRTraitWindowWidget> TraitWindowWidget;

	// 생성 후 재사용할 인게임 메뉴 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRInGameMenuWidget> InGameMenuWidget;

	// 생성 후 재사용할 옵션 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPROptionWidget> OptionWidget;

	// 생성 후 재사용할 플레이어 메뉴 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRPlayerMenu> PlayerMenuWidget;

	// 현재 바인딩된 무기 매니저
	UPROPERTY(Transient)
	TObjectPtr<UPRWeaponManagerComponent> BoundWeaponManager;

	// 현재 스코프 위젯 클래스
	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> CurrentScopeWidgetClass;

	bool bWantsHUDVisible = true;
	bool bWantsWeaponScopeVisible = false;
};
