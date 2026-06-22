// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRActionInputRouterComponent.generated.h"

/**
 * 플레이어 액션 입력 라우터다.
 * 몽타주 기반 액션이 활성화된 동안 입력을 해당 액션에 먼저 전달해 일반 입력 처리를 막는다.
 */
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRActionInputRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRActionInputRouterComponent();

	/** 입력을 소비할 액션 객체를 등록한다 */
	void RegisterInputConsumer(UObject* InputConsumer);

	/** 등록된 액션 객체와 일치할 때만 입력 소비 대상을 해제한다 */
	void UnregisterInputConsumer(const UObject* InputConsumer);

	/** 현재 액션 입력 라우팅 중인지 반환한다 */
	bool IsRoutingInput() const;

	/** 현재 등록된 액션에 입력을 전달하고, 소비 여부를 반환한다 */
	bool HandleRoutedInput();

	/** 현재 등록된 액션에 입력 스킵 가능 구간 상태를 전달한다 */
	void SetInputCancelWindow(bool bCanCancel, float BlendOutTime);

private:
	/** 현재 입력과 노티파이 스테이트를 전달받을 액션 객체다 */
	TWeakObjectPtr<UObject> ActiveInputConsumer;
};
