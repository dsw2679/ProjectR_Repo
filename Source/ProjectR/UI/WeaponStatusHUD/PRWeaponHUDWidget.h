// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (무기 변경 시 HUD 동기화 구현)
// Author: 이건주 (무기 장착 상태, 탄창 실시간 잔여량 및 Mod 버프 애니메이션 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "ProjectR/UI/WeaponStatusHUD/FPRWeaponStatusViewData.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRWeaponHUDWidget.generated.h"

class UPRAttributeSet_Weapon;
class UPRWeaponManagerComponent;
class UPRWeaponModDataAsset;
class UPRWeaponStatusWidget;
struct FOnAttributeChangeData;

// 무기 슬롯 위젯 에디터 배치와 렌더 강조 설정
struct FPRWeaponHUDSlotPresentation
{
	// 캔버스 슬롯 배치값
	FAnchorData LayoutData;

	// 콘텐츠 크기 자동 맞춤 여부
	bool bAutoSize = false;

	// 캔버스 렌더 순서
	int32 ZOrder = 0;

	// 렌더 스케일
	FVector2D RenderScale = FVector2D(1.0f, 1.0f);

	// 렌더 투명도
	float RenderOpacity = 1.0f;

	// 캐시 성공 여부
	bool bValid = false;
};

// 주무기와 보조무기 상태 표시 위젯을 묶는 HUD 전용 컨테이너다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWeaponHUDWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// PlayerState, WeaponManager, WeaponSet 연결 후 이벤트 구독을 시작한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void InitializeWeaponHUD();

	// 외부에서 명시적으로 표시 소스를 주입할 때 사용한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetWeaponHUDSources(UPRWeaponManagerComponent* InWeaponManagerComponent, UPRAttributeSet_Weapon* InWeaponSet);

	// 주무기와 보조무기 상태 표시를 모두 새로 만든다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void RefreshAllWeaponStatus();

	// 지정 슬롯의 표시 데이터만 다시 만들고 하위 위젯에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void RefreshWeaponStatus(EPRWeaponSlotType SlotType);

	// 지정 슬롯의 현재 표시 데이터를 구성한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Weapon")
	FPRWeaponStatusViewData BuildWeaponStatusViewData(EPRWeaponSlotType SlotType) const;

	// 현재 사용 무기 기준 슬롯 강조 및 배치 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void RefreshWeaponSlotHighlight();

protected:
	/*~ UUserWidget Interface ~*/
	
	virtual void NativeConstruct() override;
	
	// 제거 시점에 도메인 이벤트 구독을 해제한다
	virtual void NativeDestruct() override;

private:
	// WeaponManager 장착 변경 신호를 받아 해당 슬롯을 갱신한다
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot);

	// 슬롯 타입 대응 상태 위젯 반환
	UPRWeaponStatusWidget* GetWeaponStatusWidgetBySlot(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯 위젯 강조 렌더 상태 적용
	void ApplyWeaponSlotHighlight(EPRWeaponSlotType SlotType, bool bHighlighted) const;

	// 에디터 배치와 렌더 강조 설정 캐시
	void CacheWeaponSlotPresentations();

	// 대상 슬롯 위젯의 현재 설정 캡처
	bool CaptureWeaponSlotPresentation(UPRWeaponStatusWidget* StatusWidget, FPRWeaponHUDSlotPresentation& OutPresentation) const;

	// 대상 슬롯 위젯에 캐시된 배치와 렌더 설정 적용
	void ApplyWeaponSlotPresentation(EPRWeaponSlotType SlotType, const FPRWeaponHUDSlotPresentation& LayoutPresentation, const FPRWeaponHUDSlotPresentation& RenderPresentation) const;

	// Mod 데이터가 지속시간형 어빌리티를 사용하는지 확인
	bool DoesModUseDuration(const UPRWeaponModDataAsset* ModData) const;

	// 현재 서버 기준 월드 시간을 반환
	float ResolveServerWorldTimeSeconds() const;

	// 주무기 자원 Attribute 변화 신호를 받아 주무기 슬롯을 갱신한다
	void HandlePrimaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData);

	// 보조무기 자원 Attribute 변화 신호를 받아 보조무기 슬롯을 갱신한다
	void HandleSecondaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData);

	// 장착 변경과 Attribute 변화 이벤트 구독을 해제한다
	void UnbindWeaponHUDSources();

protected:
	// UMG에서 바인딩할 주무기 슬롯 상태 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UPRWeaponStatusWidget> PrimaryWeaponStatusWidget;

	// UMG에서 바인딩할 보조무기 슬롯 상태 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UPRWeaponStatusWidget> SecondaryWeaponStatusWidget;

private:
	// 장착 상태와 슬롯별 무기 데이터를 읽기 위한 약한 참조
	TWeakObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 탄약과 Mod 자원을 읽기 위한 약한 참조
	TWeakObjectPtr<UPRAttributeSet_Weapon> WeaponSet;

	// ASC Attribute 변화 이벤트를 해제하기 위한 핸들 묶음
	TArray<FDelegateHandle> AttributeChangeHandles;

	// 활성 슬롯 기준 설정 캐시
	FPRWeaponHUDSlotPresentation ActiveSlotPresentation;

	// 비활성 슬롯 기준 설정 캐시
	FPRWeaponHUDSlotPresentation InactiveSlotPresentation;
};
