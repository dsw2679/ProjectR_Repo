// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotifyState_ActionInputCancelWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"

FString UPRAnimNotifyState_ActionInputCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("PR Action Input Cancel Window");
}

void UPRAnimNotifyState_ActionInputCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(MeshComp->GetOwner());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	if (UPRActionInputRouterComponent* ActionInputRouter = PlayerCharacter->GetActionInputRouter())
	{
		// NotifyState는 액션 종류를 모른다. 현재 라우터에 등록된 액션에 스킵 가능 구간만 전달한다.
		ActionInputRouter->SetInputCancelWindow(true, InputCancelBlendOutTime);
	}
}

void UPRAnimNotifyState_ActionInputCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(MeshComp->GetOwner());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	if (UPRActionInputRouterComponent* ActionInputRouter = PlayerCharacter->GetActionInputRouter())
	{
		// 구간이 끝나면 이후 입력은 더 이상 액션 스킵 요청으로 해석하지 않는다.
		ActionInputRouter->SetInputCancelWindow(false, 0.0f);
	}
}
