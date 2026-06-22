// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 충전 게이지 비주얼 연동 구현)
// Author: 이건주 (무기 종류 아이콘 실시간 갱신 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WeaponStatusHUD/FPRWeaponStatusViewData.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRWeaponStatusWidget.generated.h"

struct FInstancedStruct;
struct FGameplayTag;
class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;

// 무기 하나의 아이콘, 탄창, 잔탄, Mod 상태를 표시하는 HUD 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWeaponStatusWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// 주무기 또는 보조무기 슬롯 타입을 고정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetSlotType(EPRWeaponSlotType InSlotType);

	// 표시 데이터 전체를 한 번에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetWeaponStatus(const FPRWeaponStatusViewData& ViewData);

	// 무기 아이콘을 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetWeaponIcon(UTexture2D* Icon);

	// 탄창 용량 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetMagazineAmmoText(float MagazineAmmo);

	// 잔탄 용량 표시 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetReserveAmmoText(float ReserveAmmo);

	// Mod 아이콘을 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModIcon(UTexture2D* Icon);

	// 원형 게이지 바 퍼센트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModGaugePercent(float Percent);

	// Mod 스택 개수 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModStackText(float StackCount);

	// 무기가 없는 슬롯의 기본 표시 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void ClearWeaponStatus();

	// Mod가 없는 슬롯의 기본 표시 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void ClearModStatus();

	void BindHighlightEvent();
	void UnbindHighlightEvent();
	void HandleModActivation(FGameplayTag EventTag, const FInstancedStruct& Payload);
protected:
	// UMG에서 바인딩할 무기 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UImage> WeaponIconImage;

	// UMG에서 바인딩할 탄창 용량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> MagazineAmmoText;

	// UMG에서 바인딩할 잔탄 용량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> ReserveAmmoText;

	// UMG에서 바인딩할 Mod 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UImage> ModIconImage;

	// UMG에서 바인딩할 원형 게이지 바
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UProgressBar> ModGaugeBar;

	// UMG에서 바인딩할 Mod 스택 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> ModStackText;

	// UMG에서 바인딩할 Mod 지속시간 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> ModDurationText;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UPanelWidget> HighlightBorder;
	
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetAnimOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UWidgetAnimation> HighlightAnimation;
	
private:
	// 지속시간 텍스트 표시 시작
	void StartModDurationText(float RemainingDurationSeconds);

	// 지속시간 텍스트 표시 종료
	void StopModDurationText();

	// 지속시간 텍스트 현재값 갱신
	void RefreshModDurationText();

	// 지속시간 텍스트 표시값 생성
	FText MakeModDurationText(float RemainingDurationSeconds) const;

	// 현재 서버 기준 월드 시간 반환
	float ResolveServerWorldTimeSeconds() const;

	// Mod 스택 텍스트 숨김
	void HideModStackText();

	// 이 위젯이 담당하는 무기 슬롯 타입
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon", meta = (AllowPrivateAccess = "true"))
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	
	FDelegateHandle HighlightEventDelegateHandle;

	FTimerHandle ModDurationTimerHandle;

	float ModDurationEndServerWorldTimeSeconds = 0.0f;
};
