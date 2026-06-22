// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (크로스헤어 UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRCrosshairInterface.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRCrosshairWidget.generated.h"

class UPREventManagerSubsystem;
struct FInstancedStruct;
struct FGameplayTag;

/**
 * 크로스헤어 위젯 베이스
 * EventManager의 Event.Player.HitShot, Event.Player.Recoil, Event.Player.PreviewHit 바인딩
 * 인터페이스(IPRCrosshairInterface)의 OnHit / OnRecoil / OnPreviewHit 호출
 * 인터페이스 함수의 실제 시각 처리 위치 BP
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRCrosshairWidget : public UPRWidgetBase, public IPRCrosshairInterface
{
	GENERATED_BODY()

public:
	UPRCrosshairWidget();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	// EventManager 콜백: 사격 적중
	void HandleHitShot(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 사격 반동
	void HandleRecoil(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 크로스헤어 Config 교체
	void HandleChangeCrosshair(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 히트스캔 미리보기 적중 상태
	void HandlePreviewHit(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 등록된 EventManager 핸들 정리
	void UnbindFromEventManager();

private:
	FDelegateHandle HitShotHandle;
	FDelegateHandle RecoilHandle;
	FDelegateHandle ChangeCrosshairHandle;
	FDelegateHandle PreviewHitHandle;
};
