// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI 매니저 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PRWidgetBase.h"
#include "PRUIManagerSubsystem.generated.h"

/**
 * UI 스택 매니저 서브시스템
 * Push/Pop 기반으로 위젯을 관리하고, 스택 최상위 위젯의 InputMode를 자동 적용한다.
 */
UCLASS()
class PROJECTR_API UPRUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetSystem();
	
	// 위젯을 스택에 Push (클래스로부터 새 인스턴스 생성)
	UFUNCTION(BlueprintCallable, Category = "UI", meta = (DeterminesOutputType = "WidgetClass"))
	UPRWidgetBase* PushUI(TSubclassOf<UPRWidgetBase> WidgetClass);

	// 이미 생성/캐싱된 인스턴스를 스택에 Push. 중복/유효성 검사 후 뷰포트에 부착
	UFUNCTION(BlueprintCallable, Category = "UI")
	UPRWidgetBase* PushUIInstance(UPRWidgetBase* Instance);

	// 위젯을 스택에서 Pop (nullptr이면 최상위)
	UFUNCTION(BlueprintCallable, Category = "UI")
	UPRWidgetBase* PopUI(UPRWidgetBase* Instance = nullptr);

	// 스택에 특정 클래스 위젯이 있는지
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUIActive(TSubclassOf<UPRWidgetBase> WidgetClass) const;

	// 스택 최상위 위젯 조회
	UFUNCTION(BlueprintPure, Category = "UI")
	UPRWidgetBase* GetTopWidget() const;

private:
	// 스택 최상위 InputMode를 PlayerController에 적용
	void UpdateInputMode();

	// 외부에서 제거된 위젯 정리
	void CleanupInvalidEntries();

	// Layer 우선순위에 맞는 스택 인덱스 산출 (같은 레이어 내에서는 후입선출)
	int32 FindLayerInsertIndex(EPRUILayer Layer) const;

	// 레이어로부터 ZOrder 산출
	int32 GetZOrderForLayer(EPRUILayer Layer) const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWidgetBase>> UIStack;
};

/** TODO [고도화] - GameplayTag 기반 레이어 시스템:
 *  PBWidgetBase의 LayerTag를 읽어 ZOrder를 자동 결정한다.
 *  레이어 간 정책(블러, 입력 차단 등)은 서브시스템이 중앙에서 집행한다.
 *
 * TODO [고도화] - 기타:
 *  - 위젯 캐싱 (자주 열리는 패널 재사용)
 *  - 비동기 로딩 지원
 */