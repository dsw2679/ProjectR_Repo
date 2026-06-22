// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Debug Log 상호작용 액션 실행 로직 구현)
#include "PRInteraction_DebugLog.h"

#include "Engine/Engine.h"

UPRInteraction_DebugLog::UPRInteraction_DebugLog()
{
}

void UPRInteraction_DebugLog::Execute_Implementation(AActor* Interactor)
{
	// 부모 구현 호출. Sustained 인 경우 bIsActive = true 로 토글된다
	Super::Execute_Implementation(Interactor);
	PrintDebug(TEXT("Execute"), Interactor);
}

void UPRInteraction_DebugLog::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	Super::EndInteraction_Implementation(Interactor, bCanceled);
	PrintDebug(TEXT("EndInteraction"), nullptr);
}

void UPRInteraction_DebugLog::OnHoldStart_Implementation(AActor* Interactor)
{
	// 부모 구현 호출하여 EventManager 로 Hold.Start 브로드캐스트가 그대로 발송되도록 함
	Super::OnHoldStart_Implementation(Interactor);
	PrintDebug(FString::Printf(TEXT("OnHoldStart (Duration=%.2fs)"), HoldDuration), Interactor);
}

void UPRInteraction_DebugLog::OnHoldCanceled_Implementation(AActor* Interactor)
{
	Super::OnHoldCanceled_Implementation(Interactor);
	PrintDebug(TEXT("OnHoldCanceled"), Interactor);
}

void UPRInteraction_DebugLog::OnHoldFinished_Implementation(AActor* Interactor)
{
	Super::OnHoldFinished_Implementation(Interactor);
	PrintDebug(TEXT("OnHoldFinished"), Interactor);
}

void UPRInteraction_DebugLog::PrintDebug(const FString& Phase, const AActor* Interactor) const
{
	const AActor* Owner = GetOwner();
	const FString OwnerName = IsValid(Owner) ? Owner->GetName() : TEXT("<NoOwner>");
	const FString InteractorName = IsValid(Interactor) ? Interactor->GetName() : TEXT("<None>");

	const FString Message = FString::Printf(
		TEXT("[Interaction:%s] %s | Owner=%s | Interactor=%s"),
		*DebugLabel, *Phase, *OwnerName, *InteractorName);

	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine != nullptr)
	{
		// 단계별 키를 분리해 동일 단계 메시지가 누적되지 않도록 한다
		const uint64 Key = static_cast<uint64>(GetTypeHash(DebugLabel)) ^ static_cast<uint64>(GetTypeHash(Phase));
		GEngine->AddOnScreenDebugMessage(Key, ScreenDuration, ScreenColor, Message);
	}
}
