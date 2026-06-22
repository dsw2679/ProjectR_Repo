// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Party Member Health UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPartyMemberHealthWidget.generated.h"

class APRPlayerState;
class UPRHealthBarWidget;
class UImage;
class UTextBlock;
class UTexture2D;

// 파티원 생존 표시 상태
UENUM(BlueprintType)
enum class EPRPartyMemberSurvivalState : uint8
{
	Alive,
	Down,
	Dead,
};

// 파티원 한 명의 체력과 생존 상태를 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPartyMemberHealthWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRPartyMemberHealthWidget(const FObjectInitializer& ObjectInitializer);

	// 표시할 파티원 PlayerState를 지정하고 체력 바를 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Party")
	void SetPlayerState(APRPlayerState* InPlayerState);

	// 현재 파티원 표시를 비우고 바인딩을 해제한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Party")
	void ClearPlayerState();

	// 현재 바인딩된 PlayerState를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Party")
	APRPlayerState* GetBoundPlayerState() const { return BoundPlayerState.Get(); }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 파티원 이름이 갱신될 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Party")
	void BP_OnDisplayNameChanged(const FText& InDisplayName);

	// 파티원 생존 상태가 갱신될 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Party")
	void BP_OnSurvivalStateChanged(EPRPartyMemberSurvivalState InState);

private:
	// PlayerState 표시명 변경 이벤트 구독
	void BindDisplayNameChanged(APRPlayerState* InPlayerState);

	// PlayerState 표시명 변경 이벤트 구독 해제
	void UnbindDisplayNameChanged();

	// PlayerState 표시명 변경 이벤트 수신 처리
	void HandleDisplayNameChanged(const FString& NewDisplayName);

	void RefreshDisplayName();
	void RefreshSurvivalState();
	void ApplySurvivalStateIcon();
	UTexture2D* GetSurvivalStateIconTexture() const;

private:
	// 파티원 체력을 표시하는 자식 체력 바
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UPRHealthBarWidget> HealthBar;

	// 파티원 이름을 표시하는 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UTextBlock> DisplayNameText;

	// 파티원 생존 상태 아이콘을 표시하는 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UImage> SurvivalStateIconImage;

	// 생존 상태일 때 표시할 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UTexture2D> AliveIconTexture;

	// 다운 상태일 때 표시할 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UTexture2D> DownIconTexture;

	// 사망 상태일 때 표시할 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UTexture2D> DeadIconTexture;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	EPRPartyMemberSurvivalState SurvivalState = EPRPartyMemberSurvivalState::Alive;

	TWeakObjectPtr<APRPlayerState> BoundPlayerState;

	// 표시명 변경 이벤트 핸들
	FDelegateHandle DisplayNameChangedDelegateHandle;
};
