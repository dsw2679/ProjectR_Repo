// Copyright ProjectR. All Rights Reserved.

#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"

#include "ProjectR/Input/PRActionInputConsumer.h"

UPRActionInputRouterComponent::UPRActionInputRouterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/** 입력을 소비할 액션 객체를 등록한다 */
void UPRActionInputRouterComponent::RegisterInputConsumer(UObject* InputConsumer)
{
	if (!IsValid(InputConsumer) || !InputConsumer->GetClass()->ImplementsInterface(UPRActionInputConsumer::StaticClass()))
	{
		return;
	}

	// 한 번에 하나의 액션만 입력을 선점한다. 새 액션이 등록되면 이후 입력은 새 액션으로 전달된다.
	ActiveInputConsumer = InputConsumer;
}

/** 등록된 액션 객체와 일치할 때만 입력 소비 대상을 해제한다 */
void UPRActionInputRouterComponent::UnregisterInputConsumer(const UObject* InputConsumer)
{
	if (ActiveInputConsumer.Get() == InputConsumer)
	{
		ActiveInputConsumer.Reset();
	}
}

/** 현재 액션 입력 라우팅 중인지 반환한다 */
bool UPRActionInputRouterComponent::IsRoutingInput() const
{
	return ActiveInputConsumer.IsValid();
}

/** 현재 등록된 액션에 입력을 전달하고, 소비 여부를 반환한다 */
bool UPRActionInputRouterComponent::HandleRoutedInput()
{
	UObject* InputConsumer = ActiveInputConsumer.Get();
	if (!IsValid(InputConsumer))
	{
		ActiveInputConsumer.Reset();
		return false;
	}

	IPRActionInputConsumer* ConsumerInterface = Cast<IPRActionInputConsumer>(InputConsumer);
	if (ConsumerInterface == nullptr)
	{
		ActiveInputConsumer.Reset();
		return false;
	}

	// Character는 입력 종류만 감지하고, 실제 소비/스킵 판단은 현재 활성 액션에 위임한다.
	return ConsumerInterface->HandleRoutedActionInput();
}

void UPRActionInputRouterComponent::SetInputCancelWindow(bool bCanCancel, float BlendOutTime)
{
	UObject* InputConsumer = ActiveInputConsumer.Get();
	if (!IsValid(InputConsumer))
	{
		ActiveInputConsumer.Reset();
		return;
	}

	IPRActionInputConsumer* ConsumerInterface = Cast<IPRActionInputConsumer>(InputConsumer);
	if (ConsumerInterface == nullptr)
	{
		ActiveInputConsumer.Reset();
		return;
	}

	// NotifyState는 액션을 직접 알지 못하므로 라우터를 통해 현재 활성 액션에 창 상태만 전달한다.
	ConsumerInterface->SetRoutedInputCancelWindow(bCanCancel, BlendOutTime);
}
