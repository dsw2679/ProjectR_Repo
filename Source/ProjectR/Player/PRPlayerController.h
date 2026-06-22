// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (로딩 화면 프리웜, 픽업 알림 및 강화/성장 UI 호출 제어 구현)
// Author: 배유찬 (세션 매치메이킹 UI, 리스폰 흐름 및 패스트 트래블 UI, 플래시라이트 입력 제어 구현)
// Author: 이건주 (마우스 감도 설정 갱신 및 인벤토리/무기 상태 HUD 호출 제어 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/HUD/PRHUDMessageTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponUpgradeTypes.h"
#include "PRPlayerController.generated.h"

enum class EPRFadeColorPreset : uint8;

UENUM(BlueprintType)
enum class EPRMapTransitionType : uint8
{
	None,
	MapTravel,
	WaypointTravelUI,
	Respawn,
	RespawnComplete,
	CancelTravel,
};

class UPRInteractorComponent;
class UPRProjectileManagerComponent;
class UPRFloatingTextManager;
class UAbilitySystemComponent;
class UPRInputConfigDataAsset;
class UPRAbilitySystemComponent;
class UPRQuickSlotComponent;
struct FInputActionValue;
class UInputAction;
class USoundBase;
class UUserWidget;
class UPRUIControllerComponent;
class UPRInteractionSensor;
class UPRCheatHandler;
class UPRItemInstance_Weapon;
class UPRInteraction_Waypoint;
class UPRShopComponent;
class UPRWeaponUpgradeComponent;
class UPRFXNetworkComponent;
class APRFaerinEncounterDirector;
class UPRWorldTickOptimizerReporterComponent;
enum class EFaerinEncounterSequence : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRWeaponUpgradeResultSignature, const FPRWeaponUpgradeResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopBuyResultSignature, const FPRShopBuyResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopSellResultSignature, const FPRShopSellResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRTraitInvestmentResultSignature, const FPRTraitInvestmentResult&, Result);

// 플레이어 입력·UI 소유. Join 시 캐릭터 페이로드를 서버로 전송하고,
// 인게임 중 발생한 보상 Grant를 연결이 살아있는 동안 즉시 수령하여 GameInstance에 반영한다
UCLASS()
class PROJECTR_API APRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APRPlayerController();
	
	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	
	/*~ APlayerController Interface ~*/
	virtual void ReceivedPlayer() override;
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	/*~ APRPlayerController Interface ~*/
	// 서버 권위에서 동작하는 치트 핸들러 조회. 본인 클라에는 복제로 전달
	UPRCheatHandler* GetCheatHandler() const { return CheatHandler; }

	// 로컬 플레이어 UI 흐름을 담당하는 컴포넌트 조회
	UPRUIControllerComponent* GetUIController() const { return UIControllerComponent; }
	
	UPRProjectileManagerComponent* GetProjectileManagerComponent() const {return ProjectileManager;}

	// 플로팅 텍스트 매니저 컴포넌트를 반환
	UPRFloatingTextManager* GetFloatingTextManager() const { return FloatingTextManager; }
	
	// PlayerState 우선 경로로 ASC 조회
	UPRAbilitySystemComponent* GetPRASC() const;

	// 플레이어 컨트롤러에 남은 로컬 런타임 상태와 ASC 캐시를 초기화한다
	void ResetPlayer();
	
	// 로컬 클라에서 호출. GameInstance의 LocalCharacterSave를 서버로 제출
	// ReceivedPlayer 조기 제출과 possession fallback 재시도 허용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void SubmitLocalCharacterToServer();

	// 서버 -> 본인 클라. 캐릭터 페이로드 검증 결과. 거부 시 Detail에 사유
	UFUNCTION(Client, Reliable)
	void ClientCharacterAccepted(bool bAccepted, const FString& Detail);

	// 서버 -> 본인 클라. 보상 발생 이벤트 시점에 즉시 호출
	// 수신 즉시 GameInstance.ApplyRewardGrant로 반영. 세션 종료 정산 경로는 사용하지 않는다
	UFUNCTION(Client, Reliable)
	void ClientGrantReward(const FPRRewardGrant& Grant);

	// 서버 -> 본인 클라. 현재 Pawn에 생존 상태 전환 Gameplay Event를 발행
	UFUNCTION(Client, Reliable)
	void ClientDispatchSurvivalGameplayEvent(FGameplayTag EventTag);

	// 맵 이동 또는 리스폰 전환 연출 시작
	UFUNCTION(Client, Reliable)
	void ClientNotifyMapTransition(float Delay, EPRMapTransitionType TransitionType);

	// 맵 로딩 화면 선표시 요청
	UFUNCTION(Client, Reliable)
	void ClientBeginMapLoadingScreen(const FString& MapName);

	// 로딩 오버레이 표시 완료 여부
	bool HasAcknowledgedMapLoadingScreen(const FString& MapName) const;

	// 로딩 오버레이 표시 완료 상태 초기화
	void ResetAcknowledgedMapLoadingScreen();

	// 전멸 확정 효과음 재생
	UFUNCTION(Client, Reliable)
	void ClientPlayPartyWipeSound(USoundBase* Sound);

	// 전멸 확정 위젯 표시
	UFUNCTION(Client, Reliable)
	void ClientShowPartyWipeWidget(TSubclassOf<UUserWidget> WidgetClass);

	// 서버 권위 인카운터 연출 중 소유 클라이언트의 이동/시야 입력 잠금을 적용한다.
	UFUNCTION(Client, Reliable)
	void ClientSetEncounterInputLock(bool bLock);

	// 서버 권위 인카운터 연출 종료 후 소유 클라이언트의 카메라를 현재 Pawn으로 복구한다.
	UFUNCTION(Client, Reliable)
	void ClientRestoreFaerinEncounterViewTarget(float BlendTime);

	// 서버 권위 인카운터 연출 중 소유 클라이언트의 HUD 표시 상태 변경
	UFUNCTION(Client, Reliable)
	void ClientSetFaerinEncounterHUDVisible(bool bVisible);

	// Faerin 인카운터 입력 잠금 상태를 로컬 PlayerController에 즉시 적용한다.
	void SetEncounterInputLockLocal(bool bLock);

	// Faerin 인카운터 카메라를 로컬 Pawn으로 즉시 복귀시킨다.
	void RestoreFaerinEncounterViewTargetLocal(float BlendTime);

	// Faerin 인카운터 HUD 표시 상태를 로컬 화면에 즉시 적용
	void SetFaerinEncounterHUDVisibleLocal(bool bVisible);
	
	// 서버 -> 본인 클라. 무기 강화 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyWeaponUpgradeResult(const FPRWeaponUpgradeResult& Result);

	// 서버 -> 본인 클라. 강화 UI를 열고 강화 컴포넌트 Context를 전달한다
	UFUNCTION(Client, Reliable)
	void ClientOpenWeaponUpgradeUI(UPRWeaponUpgradeComponent* UpgradeComponent);

	// 서버 -> 본인 클라. 상점 UI를 열고 상점 컴포넌트 Context를 전달한다
	UFUNCTION(Client, Reliable)
	void ClientOpenShopUI(UPRShopComponent* ShopComponent);

	// 서버 -> 호스트 클라. 웨이포인트 Travel UI 열기
	UFUNCTION(Client, Reliable)
	void ClientOpenWaypointTravelUI(bool bShowWorldResetButton);

	// 서버 -> 본인 클라. Faerin 인카운터 선택 UI를 연다
	UFUNCTION(Client, Reliable)
	void ClientOpenFaerinEncounterChoice(APRFaerinEncounterDirector* Director);

	// 서버 -> 본인 클라. Faerin 인카운터 선택 UI를 닫는다
	UFUNCTION(Client, Reliable)
	void ClientCloseFaerinEncounterChoice(bool bRestoreDefaultBGM);

	// Faerin 인카운터 전투 선택을 서버에 전달한다
	UFUNCTION(Server, Reliable)
	void ServerChooseFaerinEncounterFight(APRFaerinEncounterDirector* Director);

	// Faerin 인카운터 거절 선택을 서버에 전달한다
	UFUNCTION(Server, Reliable)
	void ServerChooseFaerinEncounterDecline(APRFaerinEncounterDirector* Director);

	// 서버 -> 본인 클라. Gather 범위 안 플레이어에게 Faerin 하단 자막을 표시한다. 본문은 NodeId로 로컬 해석한다.
	UFUNCTION(Client, Reliable)
	void ClientShowFaerinSubtitle(APRFaerinEncounterDirector* Director, FName DialogueNodeId);

	// 서버 -> 본인 클라. Faerin 하단 자막을 숨긴다.
	UFUNCTION(Client, Reliable)
	void ClientHideFaerinSubtitle();

	// 서버 -> 본인 클라. Gather 범위 안 플레이어 로컬 화면에 Intro/FightStart 시퀀스를 재생한다.
	UFUNCTION(Client, Reliable)
	void ClientPlayFaerinEncounterSequence(APRFaerinEncounterDirector* Director, EFaerinEncounterSequence SequenceType);

	// 서버 -> 본인 클라. 로컬 시퀀스 재생을 중단하고 입력/카메라/자막을 복구한다.
	UFUNCTION(Client, Reliable)
	void ClientStopFaerinEncounterSequence(APRFaerinEncounterDirector* Director, FName Reason);
	// Faerin 하단 자막을 로컬 화면에 즉시 표시한다.
	void ShowFaerinSubtitleLocal(APRFaerinEncounterDirector* Director, FName DialogueNodeId);

	// Faerin 하단 자막을 로컬 화면에 직접 표시한다. Sequence cue처럼 DialogueNodeId가 없는 경우 사용한다.
	void ShowFaerinSubtitleTextLocal(const FText& SpeakerText, const FText& BodyText);

	// Faerin 하단 자막을 로컬 화면에서 즉시 숨긴다.
	void HideFaerinSubtitleLocal();

	// Faerin 인카운터 시퀀스를 로컬 화면에서 즉시 재생한다.
	void PlayFaerinEncounterSequenceLocal(APRFaerinEncounterDirector* Director, EFaerinEncounterSequence SequenceType);

	// Faerin 인카운터 시퀀스를 로컬 화면에서 즉시 중단하고 입력/카메라/자막을 복구한다.
	void StopFaerinEncounterSequenceLocal(APRFaerinEncounterDirector* Director, FName Reason);

	// 본인 클라 -> 서버. 상호작용자가 표시한 현재 대화 노드를 서버에 알려 Gather 자막 송출을 트리거한다.
	UFUNCTION(Server, Reliable)
	void ServerNotifyFaerinDialogueNodePresented(APRFaerinEncounterDirector* Director, FName DialogueNodeId);

	// 서버 -> 본인 클라. 상점 구매 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyShopBuyResult(const FPRShopBuyResult& Result);

	// 서버 -> 본인 클라. 상점 판매 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyShopSellResult(const FPRShopSellResult& Result);

	// 서버 -> 본인 클라. 특성 투자 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyTraitInvestmentResult(const FPRTraitInvestmentResult& Result);

	// 서버에서 owning client로 레벨업 팝업 표시 요청
	UFUNCTION(Client, Reliable)
	void ClientNotifyPlayerLevelUp(int32 PreviousLevel, int32 CurrentLevel);

	// 서버에서 owning client로 드롭 보상 획득 알림 표시 요청
	UFUNCTION(Client, Reliable)
	void ClientNotifyPickupReward(const FPRPickupNotificationPayload& Payload);

	// 서버에서 owning client로 HUD 안내 메시지 표시 상태 전달
	UFUNCTION(Client, Reliable)
	void ClientNotifyHUDMessage(EPRHUDMessageType MessageType);

	// 강화 UI에서 선택한 무기 강화를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void RequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem);

	// 상점 UI에서 선택한 아이템 구매를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void RequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 상점 UI에서 선택한 아이템 판매를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void RequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 웨이포인트 Travel UI 선택 목적지 이동 서버 요청
	void RequestWaypointTravel(const FPRWaypointKey& WaypointKey);

	// 웨이포인트 Travel UI 닫힘 서버 통지
	void RequestCancelWaypointTravel();

	// 서버 웨이포인트 Travel UI 대기 상호작용 등록
	void SetPendingWaypointTravelInteraction(UPRInteraction_Waypoint* WaypointInteraction);

	// 서버 웨이포인트 Travel UI 대기 상호작용 정리
	void ClearPendingWaypointTravelInteraction(const UPRInteraction_Waypoint* WaypointInteraction);

	// 성장 UI에서 특성 투자 확정을 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 성장 UI에서 특성 투자 초기화를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestResetTraitInvestment();
	
protected:
	// 클라이언트 -> 서버. 로컬 캐릭터 페이로드 제출
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCharacter(const FPRCharacterSaveData& Payload);
	
	// 클라이언트 -> 서버. 강화 NPC 또는 터미널 컴포넌트에 무기 강화를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem);
	
	void OnMouseSensitivityActionUp();

	// 클라이언트 -> 서버. 상점 컴포넌트에 아이템 구매를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 클라이언트 -> 서버. 상점 컴포넌트에 아이템 판매를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 클라이언트 -> 서버. 활성 또는 UI 대기 웨이포인트 상호작용 목적지 이동 위임
	UFUNCTION(Server, Reliable)
	void ServerRequestWaypointTravel(FPRWaypointKey WaypointKey);

	// 클라이언트 -> 서버. 활성 또는 UI 대기 웨이포인트 상호작용 취소 위임
	UFUNCTION(Server, Reliable)
	void ServerRequestCancelWaypointTravel();

	// 클라이언트 로딩 오버레이 표시 완료 알림
	UFUNCTION(Server, Reliable)
	void ServerAcknowledgeMapLoadingScreen(const FString& MapName);

	// 클라이언트 -> 서버. 성장 컴포넌트에 특성 투자 확정을 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 클라이언트 -> 서버. 성장 컴포넌트에 특성 투자 초기화를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestResetTraitInvestment();

	void OnMouseSensitivityActionDown();
	
	// IA Pressed 콜백. InputTag를 ASC로 전달
	void OnAbilityInputPressed(FGameplayTag InputTag);

	// IA Released 콜백
	void OnAbilityInputReleased(FGameplayTag InputTag);

	// 플래시라이트 토글
	void ToggleFlashlight(const FInputActionValue& Value);
	
    // ====== UI =====
	// 인벤토리 입력 시작을 처리
	void OnInventoryInputStarted();

	// 특성 투자창 입력 시작을 처리
	void OnTraitWindowInputStarted();

	// 인게임 메뉴 입력 시작을 처리
	void OnInGameMenuInputStarted();
	
	// 퀵슬롯 입력 시작을 처리
	void OnQuickSlotInputStarted(int32 SlotIndex);

	// 플레이어 퀵슬롯 컴포넌트를 조회
	UPRQuickSlotComponent* GetQuickSlotComponent() const;
	
private:
	// 로컬 상호작용 포커스와 본인 캐릭터 외곽선 정리
	void ResetLocalInteractionVisualState();

	// 활성 또는 UI 대기 중인 웨이포인트 상호작용 액션 반환
	UPRInteraction_Waypoint* ResolveWaypointTravelInteraction() const;

	// 서버 객체 기준 호스트 컨트롤러 여부 확인
	bool IsHostControllerForWaypointTravel() const;

	void UpdateCompanionHighlight();

	// 전멸 확정 위젯 제거
	void HidePartyWipeWidget();

public:
	// 무기 강화 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradeResultSignature OnWeaponUpgradeResult;
	
	// 상점 구매 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopBuyResultSignature OnShopBuyResult;

	// 상점 판매 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopSellResultSignature OnShopSellResult;

	// 특성 투자 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Growth")
	FPRTraitInvestmentResultSignature OnTraitInvestmentResult;
protected:
	// ====== Configs ======
	// 치트 핸들러 클래스. BP에서 지정. 비어있으면 핸들러 미생성
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UPRCheatHandler> CheatHandlerClass;

	// IA에 대한 InputTag 매핑을 담고 있는 InputConfig. 각 IA에 Pressed/Released 콜백을 라우팅
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UPRInputConfigDataAsset> InputConfig;
	
	// 플래시 라이트 On/Off 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UInputAction> FlashlightAction;
	
	// 인벤토리 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> InventoryAction;

	// 특성 투자창 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> TraitWindowAction;

	// 인게임 메뉴 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> InGameMenuAction;

	// 퀵슬롯 입력 액션 목록. 배열 인덱스가 퀵슬롯 인덱스와 일치한다
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TArray<UInputAction*> QuickSlotActions;
	
	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionUp;
	
	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionDown;
	
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Effect")
	float FadeInDuration = 1.0f;

private:
	// 캐릭터 페이로드를 이미 제출했는지 여부. 중복 제출 방지
	bool bCharacterSubmitted = false;
	
	mutable TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC;

	// 서버 권위에서 생성되어 본인 클라에 복제되는 치트 명령 처리 객체
	UPROPERTY(Replicated)
	TObjectPtr<UPRCheatHandler> CheatHandler;
	
	// ====== Components =====
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRProjectileManagerComponent> ProjectileManager;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRFloatingTextManager> FloatingTextManager;

	// 로컬 플레이어 UI 표시 흐름을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|UI")
	TObjectPtr<UPRUIControllerComponent> UIControllerComponent;

	// 주변 Interactable 감지, 포커스 후보 선정을 담당하는 컴포넌트 (로컬 컨트롤러 전용 동작)
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Interaction")
	TObjectPtr<UPRInteractionSensor> InteractionSensor;
	
	// 상호작용 매니저 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Interaction")
	TObjectPtr<UPRInteractorComponent> InteractorComponent;
	
	// FX 서버 요청과 Client RPC 수신을 담당하는 Player 소유 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|FX")
	TObjectPtr<UPRFXNetworkComponent> FXNetworkComponent;

	// WorldTickOptimizer 클라이언트 렌더 상태 보고 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|TickOptimization")
	TObjectPtr<UPRWorldTickOptimizerReporterComponent> TickOptimizerReporterComponent;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PartyWipeWidget;

	// 서버 전용 웨이포인트 Travel UI 요청 대상 상호작용
	TWeakObjectPtr<UPRInteraction_Waypoint> PendingWaypointTravelInteraction;

	// 서버에서 확인한 마지막 로딩 오버레이 표시 MapName
	FString LastAcknowledgedLoadingScreenMapName;

	// 로딩 오버레이 FadeIn 이후 ack 전송 타이머
	FTimerHandle LoadingScreenAckTimerHandle;
};
