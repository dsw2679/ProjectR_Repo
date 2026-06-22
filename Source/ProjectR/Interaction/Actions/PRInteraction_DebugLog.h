// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Debug Log 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_DebugLog.generated.h"

/**
 * 상호작용 흐름 디버그용 테스트 Action.
 * Execute / EndInteraction / OnHoldStart / OnHoldCanceled / OnHoldFinished 시점마다
 * 화면 출력(GEngine->AddOnScreenDebugMessage)과 로그를 남긴다.
 * 실 게임 로직은 부모 기본 구현(Sustained 활성 토글, Hold 이벤트 브로드캐스트)을 그대로 사용한다.
 */
UCLASS(meta = (DisplayName = "Interaction Debug Log"))
class PROJECTR_API UPRInteraction_DebugLog : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_DebugLog();

	/*~ UPRInteractionAction Interface ~*/
	virtual void Execute_Implementation(AActor* Interactor) override;
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;
	virtual void OnHoldStart_Implementation(AActor* Interactor) override;
	virtual void OnHoldCanceled_Implementation(AActor* Interactor) override;
	virtual void OnHoldFinished_Implementation(AActor* Interactor) override;

public:
	// 디버그 메시지 식별용 라벨. 여러 액션을 동시에 두고 구분할 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	FString DebugLabel = TEXT("Debug");

	// 화면 메시지 표시 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug", meta = (ClampMin = "0.0"))
	float ScreenDuration = 2.0f;

	// 화면 메시지 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	FColor ScreenColor = FColor::Cyan;

private:
	// 화면 + 로그 동시 출력 헬퍼
	void PrintDebug(const FString& Phase, const AActor* Interactor) const;
};
