// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 HP/포이즈 게이지 및 픽업 보상 알림 UI 구현)
// Author: 배유찬 (Mod 게이지, 핑/마커 알림 및 홀드 상호작용 게이지 UI 구현)
// Author: 이건주 (무기 장착 상태 및 소모품 퀵슬롯 UI 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRHUDWidget.generated.h"

class UPRInteractionHintWidget;
class UPRLevelUpPopupWidget;
class UPRPickupNotificationListWidget;
class UPRQuickSlotWidget;
class UPRWeaponHUDWidget;
class UPRCrosshairWidget;
class UPRInteractionProgressBar;
class UPRBossHealthBarWidget;
class UPRHealthBarWidget;
class UPRHUDMessageWidget;
class UPRPartyHealthListWidget;
class UPRStaminaBarWidget;
class UPRTraitNotiTextWidget;
class UPRWorldMarkerLayerWidget;
class UPREventManagerSubsystem;
class APRBossBaseCharacter;
struct FInstancedStruct;
struct FGameplayTag;

/**
 * 인게임 HUD 컨테이너 위젯.
 * BP UMG에서 자식 위젯들을 BindWidgetOptional 로 구성하며,
 * 자식 존재 여부에 따라 EventManager 이벤트 바인딩을 자동으로 처리한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRHUDWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRHUDWidget();

	// 보스 HP 바를 지정한 보스 캐릭터에 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Boss")
	void BindBossHealthBar(APRBossBaseCharacter* InBoss);

	// 보스 HP 바 바인딩을 해제하고 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Boss")
	void ClearBossHealthBar();

	// 레벨업 팝업 표시 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Growth")
	void ShowLevelUpPopup(int32 PreviousLevel, int32 CurrentLevel);

	// 드롭 보상 획득 알림 표시 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Pickup")
	void ShowPickupRewardNotification(const FPRPickupNotificationPayload& Payload);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/*~ UPRHUDWIdget Interface ~*/
	void OnPlayerReady();
	
private:
	void HandlePlayerReady(FGameplayTag EventTag, const FInstancedStruct& Payload);
	
	// EventManager 콜백: 에이밍 시작 - 크로스헤어 표시
	void HandleAimStart(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 에이밍 종료 - 크로스헤어 숨김
	void HandleAimEnd(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: Hold 상호작용 단계 알림 - InteractionHUD 갱신
	void HandleInteractionHold(FGameplayTag EventTag, const FInstancedStruct& Payload);
	
	// 상호작용 대상 관련 이벤트 (힌트 표시)
	void HandleInteractableChanged(FGameplayTag EventTag, const FInstancedStruct& Payload);
	
	// EventManager 콜백: 보스 조우 시작 - BossHealthBar에 대상 보스 바인딩
	void HandleBossEncounterBegin(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 보스 조우 종료 - BossHealthBar 해제
	void HandleBossEncounterEnd(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 위젯 생성 시점에 월드에 이미 존재하는 보스를 BossHealthBar에 바인딩
	void DiscoverActiveBossInWorld();

	// 등록된 EventManager 핸들 정리
	void UnbindFromEventManager();

	// 크로스헤어 가시성 토글
	void SetCrosshairVisible(bool bVisible);

protected:
	// UMG 트리에서 동일 이름("CrosshairWidget")의 자식이 있을 때 자동 바인딩.
	// BP 레이아웃에 크로스헤어가 없으면 nullptr이며, 이 경우 에임 이벤트 바인딩도 건너뛴다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRCrosshairWidget> CrosshairWidget;

	// UMG 트리에서 동일 이름("InteractionHUD")의 자식이 있을 때 자동 바인딩.
	// 없으면 Hold 이벤트 바인딩도 건너뛴다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRInteractionProgressBar> InteractionProgressBar;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRInteractionHintWidget> InteractionHint;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRWeaponHUDWidget> WeaponHUD;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRQuickSlotWidget> QuickSlotHUD;

	// UMG 트리에서 동일 이름("PlayerHealthBar")의 자식이 있을 때 자동 바인딩.
	// PlayerReady 이후 ASC 기준으로 현재 체력, 회복 가능 체력, 지연 체력 표시를 갱신한다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRHealthBarWidget> PlayerHealthBar;

	// PlayerReady 이후 ASC 기준으로 현재 스태미나 표시를 갱신한다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRStaminaBarWidget> PlayerStaminaBar;

	// UMG 트리에서 동일 이름("PartyHealthList")의 자식이 있을 때 자동 바인딩.
	// PlayerReady 이후 GameState PlayerArray 기준으로 파티원 체력 슬롯을 갱신한다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRPartyHealthListWidget> PartyHealthList;

	// UMG 트리에서 동일 이름("BossHealthBar")의 자식이 있을 때 자동 바인딩.
	// 보스 조우 흐름에서 BindBossHealthBar 호출 시 화면 상단 보스 HP를 표시한다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRBossHealthBarWidget> BossHealthBar;

	// UMG 트리에서 동일 이름("WorldMarkerLayer")의 자식이 있을 때 자동 바인딩.
	// 핑 마커 추가·제거 이벤트 구독과 화면 투영 처리
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRWorldMarkerLayerWidget> WorldMarkerLayer;

	// UMG 트리에서 동일 이름("LevelUpPopupWidget")의 자식이 있을 때 자동 바인딩
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRLevelUpPopupWidget> LevelUpPopupWidget;

	// W_HUD에 배치한 픽업 알림 목록 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRPickupNotificationListWidget> PickupNotificationListWidget;

	// W_HUD에 배치한 안내 메시지 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRHUDMessageWidget> HUDMessageWidget;

	// UMG 트리에서 동일 이름("TraitNotiTextWidget")의 자식이 있을 때 자동 바인딩
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRTraitNotiTextWidget> TraitNotiTextWidget;

private:
	TArray<FDelegateHandle> EventHandles;

	// Hold 진행 상태 추적 (NativeTick 에서 ProgressBar 갱신에 사용)
	bool bIsHoldActive = false;
	float HoldElapsed = 0.f;
	float HoldDuration = 0.f;
};
