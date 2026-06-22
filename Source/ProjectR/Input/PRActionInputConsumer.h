// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRActionInputConsumer.generated.h"

/**
 * 액션 입력 라우터가 입력 소비를 위임할 대상 인터페이스다.
 * 회피, 피격 경직 몽타주 동안 입력을 막거나 특정 구간에서 스킵하려는 액션이 구현한다.
 */
UINTERFACE(MinimalAPI)
class UPRActionInputConsumer : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPRActionInputConsumer
{
	GENERATED_BODY()

public:
	/** 라우터로 들어온 입력을 소비하고, 필요한 경우 액션을 조기 종료한다 */
	virtual bool HandleRoutedActionInput() = 0;

	/** 몽타주 노티파이 스테이트가 여는 입력 스킵 가능 구간을 액션에 전달한다 */
	virtual void SetRoutedInputCancelWindow(bool bCanCancel, float BlendOutTime) = 0;
};
