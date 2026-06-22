// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction Action 구현)
#include "PRInteractionAction.h"

#include "PRInteractableComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"


AActor* UPRInteractionAction::GetOwner() const
{
	return GetTypedOuter<AActor>();
}

UPRInteractableComponent* UPRInteractionAction::GetOwnerInteractable() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->FindComponentByClass<UPRInteractableComponent>();
	}
	return nullptr;
}

APawn* UPRInteractionAction::GetPawn(AActor* Interactor) const
{
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn;
	}
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller->GetPawn();
	}
	return nullptr;
}

AController* UPRInteractionAction::GetController(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller;
	}
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn->GetController();
	}
	return nullptr;
}

bool UPRInteractionAction::CanInteract_Implementation(AActor* Interactor) const
{
	// 기본 구현: 항상 실행 가능. 하위 클래스에서 조건을 재정의한다.
	return true;
}

void UPRInteractionAction::Execute_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 실제 행동을 구현한다.
	// 유지형인 경우 활성 상태로 전환
	if (ShouldSustained())
	{
		ActiveInteractors.AddUnique(Interactor);
	}
}

void UPRInteractionAction::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	// 기본 구현: 활성 상태 해제만 수행. 하위 클래스에서 정리 로직 추가
	ActiveInteractors.Remove(Interactor);
}

void UPRInteractionAction::OnHoldStart_Implementation(AActor* Interactor)
{
	// HUD 등 클라측 리스너에 Hold 시작을 알림. HoldDuration 을 함께 전달
	if (IsLocalPlayer(Interactor))
	{
		BroadcastHoldEvent(EPRInteractionHoldPhase::Start);
	}
}

void UPRInteractionAction::OnHoldCanceled_Implementation(AActor* Interactor)
{
	// HUD 등 클라측 리스너에 Hold 취소를 알림
	if (IsLocalPlayer(Interactor))
	{
		BroadcastHoldEvent(EPRInteractionHoldPhase::Canceled);
	}
}

void UPRInteractionAction::OnHoldFinished_Implementation(AActor* Interactor)
{
	// HUD 등 클라측 리스너에 Hold 완료를 알림 (Execute 직전 성공 펄스)
	if (IsLocalPlayer(Interactor))
	{
		BroadcastHoldEvent(EPRInteractionHoldPhase::Finished);	
	}
}

void UPRInteractionAction::BroadcastHoldEvent(EPRInteractionHoldPhase Phase) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	FPRInteractionHoldEventPayload Payload;
	Payload.Phase = Phase;
	Payload.HoldDuration = HoldDuration;
	Payload.ActionName = ActionName;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_Interaction_Hold, Payload);
}

int32 UPRInteractionAction::GetPriority_Implementation() const
{
	return Priority;
}

bool UPRInteractionAction::IsActive(AActor* Interactor) const
{
	if (IsValid(Interactor))
	{
		return ActiveInteractors.Contains(Interactor);
	}
	return false;
}

bool UPRInteractionAction::ShouldShowHint(AActor* Viewer) const
{
	if (!bShowHint)
	{
		return false;
	}

	if (!bOnlyShowHintNearScreenCenter)
	{
		return true;
	}

	const AActor* OwnerActor = GetOwner();
	APlayerController* PlayerController = Cast<APlayerController>(GetController(Viewer));
	if (!IsValid(OwnerActor) || !IsValid(PlayerController))
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	FVector2D ScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(OwnerActor->GetActorLocation(), ScreenPosition))
	{
		return false;
	}

	const bool bInsideScreen =
		ScreenPosition.X >= 0.f
		&& ScreenPosition.X <= static_cast<float>(ViewportSizeX)
		&& ScreenPosition.Y >= 0.f
		&& ScreenPosition.Y <= static_cast<float>(ViewportSizeY);
	if (!bInsideScreen)
	{
		return false;
	}

	// 화면 중앙 허용 반경 및 짧은 변 기준 비율
	const FVector2D ScreenCenter(static_cast<float>(ViewportSizeX) * 0.5f, static_cast<float>(ViewportSizeY) * 0.5f);
	const float MaxScreenDistance = FMath::Min(ViewportSizeX, ViewportSizeY) * FMath::Clamp(HintScreenCenterRadiusRatio, 0.f, 1.f);
	return FVector2D::Distance(ScreenPosition, ScreenCenter) <= MaxScreenDistance;
}

bool UPRInteractionAction::IsLocalPlayer(AActor* Interactor) const
{
	if (AController* AsController =  Cast<AController>(Interactor))
	{
		return AsController->IsLocalController();
	}
	return false;
}
