// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interactor 컴포넌트 구현)
#include "PRInteractorComponent.h"
#include "PRInteractableComponent.h"
#include "PRInteractionAction.h"
#include "PRInteractionInterface.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"

void FPRActiveInteractionInfo::Reset()
{
	ActiveInteractable = nullptr;
	ActiveActionIndex = INDEX_NONE;
	bSustained = false;
}

bool FPRActiveInteractionInfo::IsValid() const
{
	return ::IsValid(GetActiveAction());
}

bool FPRActiveInteractionInfo::RequiresRange() const
{
	const UPRInteractionAction* Action = GetActiveAction();
	if (::IsValid(Action))
	{
		return Action->bRequiresRange;
	}
	return false;
}

UPRInteractionAction* FPRActiveInteractionInfo::GetActiveAction() const
{
	if (!::IsValid(ActiveInteractable))
	{
		return nullptr;
	}

	if (!ActiveInteractable->InteractionActions.IsValidIndex(ActiveActionIndex))
	{
		return nullptr;
	}

	return ActiveInteractable->InteractionActions[ActiveActionIndex];
}

void FPRHoldInfo::Reset()
{
	HoldTarget = nullptr;
	HoldAction = nullptr;
	bIsHolding = false;
}

bool FPRHoldInfo::IsValid() const
{
	return ::IsValid(HoldTarget) && ::IsValid(HoldAction);
}

UPRInteractorComponent::UPRInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	SetIsReplicatedByDefault(true);
}

void UPRInteractorComponent::TryFocus(AActor* Actor)
{
	// 무시 목록에 포함된 액터는 포커스 해제
	for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoreActors)
	{
		if (IgnoredActor.Get() == Actor)
		{
			ClearFocus();
			return;
		}
	}

	// 인터페이스 미구현시 포커스 해제
	if (!Actor || !Actor->Implements<UPRInteractionInterface>())
	{
		ClearFocus();
		return;
	}
	
	IPRInteractionInterface* Interaction = Cast<IPRInteractionInterface>(Actor);
	SetFocus(Interaction ? Interaction->GetInteractableComponent() : nullptr);
}

void UPRInteractorComponent::SetIgnoreActors(const TArray<AActor*>& Actors)
{
	IgnoreActors.Reset();
	
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			IgnoreActors.Add(TWeakObjectPtr<AActor>(Actor));
		}
	}
}

void UPRInteractorComponent::SetFocus(UPRInteractableComponent* NewFocus)
{
	const bool bIsWithinRange =  IsWithinRange(FocusedComponent);
	
	if (FocusedComponent == NewFocus)
	{
		// 동일 대상이면 업데이트
		if (IsValid(FocusedComponent))
		{
			FocusedComponent->UpdateFocus(GetOwner(),bIsWithinRange);	
		}
		return;
	}

	// 기존 포커스 해제
	if (IsValid(FocusedComponent))
	{
		FocusedComponent->OnUnfocus();
	}

	FocusedComponent = NewFocus;

	// 신규 포커스 진입
	if (IsValid(FocusedComponent))
	{
		FocusedComponent->OnFocus(GetOwner(), IsWithinRange(FocusedComponent));
		UE_LOG(LogTemp,Warning,TEXT("Interactable Focused: %s"), *FocusedComponent->GetOwner()->GetName());
	}
}

void UPRInteractorComponent::ClearFocus()
{
	SetFocus(nullptr);
	
	UPREventManagerSubsystem* EventMgr = GetWorld()->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}
	
	FPRInteractableEventPayload Payload;
	Payload.bShowPrompt = false;
	Payload.bCanInteract =  false;

	EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_Interactable, Payload);
}

void UPRInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 활성 상호작용이 무효화되었으면 정리
		if (!ActiveInteractionInfo.IsValid())
		{
			Internal_EndActiveInteraction(true);
		}
		// 거리 유지가 필요한 경우 범위 체크
		else if (ActiveInteractionInfo.RequiresRange())
		{
			AActor* TargetActor = ActiveInteractionInfo.ActiveInteractable->GetOwner();
			if (!IsWithinRange(TargetActor))
			{
				Internal_EndActiveInteraction(true);
			}
		}
	}
}

void UPRInteractorComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(ThisClass, ActiveInteractionInfo, COND_OwnerOnly);
}

void UPRInteractorComponent::InteractFocused()
{
	if (!IsValid(FocusedComponent))
	{
		return;
	}
	
	if (!IsWithinRange(FocusedComponent))
	{
		return;
	}

	// InteractableComponent의 Action 자동 선택
	UPRInteractionAction* BestAction = FocusedComponent->SelectBestAction(GetOwner());
	int8 ActionIndex = static_cast<int8>(FocusedComponent->FindActionIndex(BestAction));
	
	if (!IsValid(BestAction))
	{
		return;
	}
	
	if (BestAction->ShouldSustained())
	{
		ActiveInteractionInfo.Reset();
		ActiveInteractionInfo.bSustained = true;
	}
	
	if (BestAction->IsHoldTrigger())
	{
		ClearHoldInfo();
		HoldInfo.bIsHolding = true;
	}
	
	ServerInteract(FocusedComponent, ActionIndex);
}

void UPRInteractorComponent::OnInteractionReleased()
{
	// 누르자마자 떼는 케이스 보호:
	// InteractFocused 가 ServerInteract 를 보낸 직후엔 HoldTarget/HoldAction 이 아직 비어 있으므로
	// HoldInfo.IsValid() 로 검사하면 false 가 되어 취소 RPC 가 발송되지 않는다.
	// 대신 bIsHolding 플래그를 기준으로 한다 (ServerInteract/ServerCancelHold 모두 Reliable 이라 서버 도착 순서는 보장됨).
	if (HoldInfo.bIsHolding)
	{
		// 클라 선반영: 중복 OnInteractionReleased 호출에 대한 방어
		HoldInfo.bIsHolding = false;
		ServerCancelHold();
	}
}

void UPRInteractorComponent::ServerCancelHold_Implementation()
{
	MulticastCancelHold();
}

void UPRInteractorComponent::ServerFinishHold_Implementation()
{
	if (!HoldInfo.IsValid())
	{
		ClearHoldInfo();
		return;
	}

	UPRInteractableComponent* InteractionTarget = HoldInfo.HoldTarget;
	int8 ActionIndex = static_cast<int8>(InteractionTarget->FindActionIndex(HoldInfo.HoldAction));
	
	MulticastFinishHold();
	Internal_HandleInteraction(InteractionTarget, ActionIndex);
}

void UPRInteractorComponent::MulticastStartHold_Implementation(UPRInteractableComponent* Target, int8 ActionIndex)
{
	if (!IsValid(Target))
	{
		ClearHoldInfo();
		return;
	}

	UPRInteractionAction* Action = Target->FindActionByIndex(ActionIndex);
	if (!IsValid(Action))
	{
		ClearHoldInfo();
		return;
	}
	
	SetHoldInfo(Target, Action);
	if (!HoldInfo.IsValid())
	{
		return;
	}
	
	Target->SetIsHolding(true);
	// HoldStart 피드백
	HoldInfo.HoldAction->OnHoldStart(GetOwner());
}

void UPRInteractorComponent::MulticastCancelHold_Implementation()
{
	if (HoldInfo.IsValid())
	{
		HoldInfo.HoldTarget->SetIsHolding(false);
		HoldInfo.HoldAction->OnHoldCanceled(GetOwner());
	}
	
	ClearHoldInfo();
}

void UPRInteractorComponent::MulticastFinishHold_Implementation()
{
	if (HoldInfo.IsValid())
	{
		HoldInfo.HoldTarget->SetIsHolding(false);
		HoldInfo.HoldAction->OnHoldFinished(GetOwner());
		if (HoldInfo.HoldAction->ShouldSustained())
		{
			ActiveInteractionInfo.bSustained = true;
		}
	}
	
	ClearHoldInfo();
}

void UPRInteractorComponent::ServerInteract_Implementation(UPRInteractableComponent* Target, int8 ActionIndex)
{
	if (!IsValid(Target))
	{
		ClientNotifyDenied();
		return;
	}

	if (!IsWithinRange(Target))
	{
		ClientNotifyDenied();
		return;
	}

	ClearPreviousInteraction();

	// 상호작용 가능한 액션 탐색
	UPRInteractionAction* Action = Target->FindActionByIndex(ActionIndex);
	if (!IsValid(Action))
	{
		ClientNotifyDenied();
		return;
	}

	// 홀드 타입인 경우
	if (Action->IsHoldTrigger())
	{
		Internal_StartHold(Target, ActionIndex);
	}
	// 그 외 (즉발 타입인 경우)
	else
	{
		Internal_HandleInteraction(Target, ActionIndex);
	}
}

void UPRInteractorComponent::ClientNotifyDenied_Implementation()
{
	// 클라 선반영된 Hold 상태 정리 (OnHoldEnd broadcast 포함)
	ClearHoldInfo();

	// 클라 선반영된 Sustained 상태 정리
	if (OnInteractionEnd.IsBound())
	{
		OnInteractionEnd.Broadcast();
	}
	ActiveInteractionInfo.Reset();
}

void UPRInteractorComponent::RequestEndInteract(bool bCanceled)
{
	// 오너 클라는 OwnerOnly 복제로 활성 정보를 보유하므로 로컬 선검사 가능
	if (!ActiveInteractionInfo.IsValid())
	{
		return;
	}

	ServerEndInteract(bCanceled);
}

void UPRInteractorComponent::ClientEndInteract_Implementation()
{
	if (OnInteractionEnd.IsBound())
	{
		OnInteractionEnd.Broadcast();
	}
	ActiveInteractionInfo.Reset();
}

void UPRInteractorComponent::ServerEndInteract_Implementation(bool bCanceled)
{
	if (!ActiveInteractionInfo.IsValid())
	{
		return;
	}

	Internal_EndActiveInteraction(bCanceled);
}

bool UPRInteractorComponent::HasFocus() const
{
	return IsValid(FocusedComponent);
}

bool UPRInteractorComponent::IsWithinRange(const AActor* TargetActor) const
{
	const IPRInteractionInterface* Interaction = Cast<IPRInteractionInterface>(TargetActor);
	if (!Interaction)
	{
		return false;
	}
	
	const UPRInteractableComponent* Interactable = Interaction->GetInteractableComponent();
	return IsWithinRange(Interactable);
}

bool UPRInteractorComponent::IsWithinRange(const UPRInteractableComponent* TargetComponent) const
{
	if (!IsValid(TargetComponent))
	{
		return false;
	}
	
	// 컨트롤러에 부착된 컴포넌트이므로 빙의 폰의 위치를 기준으로 거리 계산
	const AController* Controller = Cast<AController>(GetOwner());
	const APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return false;
	}
	
	const FVector PawnLocation = Pawn->GetActorLocation();
	float DistSq = FVector::DistSquared(PawnLocation, TargetComponent->GetActorLocation());

	if (const AActor* TargetOwner = TargetComponent->GetOwner())
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		TargetOwner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

		for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!IsValid(PrimitiveComponent)
				|| PrimitiveComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision
				|| PrimitiveComponent->GetCollisionObjectType() != PRCollisionChannels::ECC_Interactable)
			{
				continue;
			}

			// 포커스 후보를 만든 상호작용 콜리전의 실제 범위 기준으로 거리 판정을 보정한다.
			const FVector ClosestPoint = PrimitiveComponent->Bounds.GetBox().GetClosestPointTo(PawnLocation);
			DistSq = FMath::Min(DistSq, FVector::DistSquared(PawnLocation, ClosestPoint));
		}
	}

	return DistSq <= FMath::Square(MaxInteractionDistance * TargetComponent->InteractionRangeScale);
}

bool UPRInteractorComponent::IsSustained() const
{
	return ActiveInteractionInfo.bSustained;
}

bool UPRInteractorComponent::IsHolding() const
{
	return HoldInfo.bIsHolding;
}

bool UPRInteractorComponent::ShouldListenInputReleased() const
{
	return HoldInfo.bIsHolding;
}

void UPRInteractorComponent::Internal_EndActiveInteraction(bool bCanceled)
{
	if (ActiveInteractionInfo.IsValid())
	{
		ActiveInteractionInfo.ActiveInteractable->EndActiveInteraction(GetOwner(), bCanceled);
	}

	ActiveInteractionInfo.Reset();
	SetComponentTickEnabled(false);
	ClientEndInteract();
}

void UPRInteractorComponent::Internal_SetActiveInteraction(UPRInteractionAction* Action, UPRInteractableComponent* Interactable)
{
	if (!IsValid(Action) || !IsValid(Interactable))
	{
		return;
	}

	// Interactable의 InteractionActions 내 인덱스 검색 (포인터 직접 복제 회피)
	const int32 FoundIndex = Interactable->InteractionActions.IndexOfByKey(Action);
	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	ActiveInteractionInfo.ActiveInteractable = Interactable;
	ActiveInteractionInfo.ActiveActionIndex = static_cast<int8>(FoundIndex);
	ActiveInteractionInfo.bSustained = true;

	// 서버 권위 유지형 상호작용 시작 알림
	if (OnInteractionStart.IsBound())
	{
		OnInteractionStart.Broadcast();
	}

	// 거리 유지가 필요한 경우에만 Tick 활성화
	if (ActiveInteractionInfo.RequiresRange())
	{
		SetComponentTickEnabled(true);
	}
}

void UPRInteractorComponent::SetHoldInfo(UPRInteractableComponent* Target, UPRInteractionAction* HoldAction)
{
	if (!IsValid(Target))
	{
		return;
	}
	HoldInfo.HoldTarget = Target;
	HoldInfo.HoldAction = HoldAction;
	HoldInfo.bIsHolding = true;

	// 액션의 HoldDuration 을 복사. 서버 측 Internal_StartHold 가 이 값으로 SetTimer 를 호출하므로
	// 누락 시 0 으로 SetTimer 가 호출되어 타이머 자체가 등록되지 않는다 (UE FTimerManager 동작상 InRate <= 0 은 무시).
	HoldInfo.HoldDuration = IsValid(HoldAction) ? HoldAction->HoldDuration : 0.f;
}

void UPRInteractorComponent::ClearHoldInfo()
{
	if (HoldInfo.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldInfo.HoldTimerHandle);
	}

	// Reset 을 Broadcast 보다 먼저 수행. OnHoldEnd 외부 핸들러가 EndAbility 를 경유해 IsHolding 을 확인하는데,
	// Reset 이전이라면 리슨 서버에서 bIsHolding 이 true 로 남아 OnInteractionReleased -> ServerCancelHold 경로가 발동해
	// Finish 흐름임에도 Cancel 콜백까지 호출되는 문제 발생
	HoldInfo.Reset();

	if (OnHoldEnd.IsBound())
	{
		OnHoldEnd.Broadcast();
		OnHoldEnd.Clear();
	}
}

void UPRInteractorComponent::OnHoldFinished()
{
	if (!HoldInfo.IsValid())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	const float HoldTime = World->GetTimeSeconds() - HoldInfo.StartTime;
	if (HoldTime < HoldInfo.HoldDuration - SMALL_NUMBER)
	{
		UE_LOG(LogTemp,Warning,TEXT("UPRInteractorComponent::OnHoldFinished, 잘못된 Hold 종료 신호"))
		return;
	}
	
	ServerFinishHold();
}

void UPRInteractorComponent::OnRep_ActiveInteractionInfo()
{
	// TODO : 클라측 피드백
}

void UPRInteractorComponent::Internal_HandleInteraction(UPRInteractableComponent* Target,int32 ActionIndex)
{
	if (!IsValid(Target))
	{
		return;
	}
	
	if (!IsWithinRange(Target))
	{
		return;
	}
	
	Target->OnInteract(GetOwner(), ActionIndex);

	// OnInteract 후 유지형 Action이 시작되었으면 추적 시작
	UPRInteractionAction* NewActiveAction = Target->GetActiveAction();
	if (IsValid(NewActiveAction) && NewActiveAction->ShouldSustained())
	{
		if (NewActiveAction->IsActive(GetOwner()))
		{
			Internal_SetActiveInteraction(NewActiveAction, Target);	
		}
		else
		{
			ActiveInteractionInfo.Reset();
		}
	}
}

void UPRInteractorComponent::Internal_StartHold(UPRInteractableComponent* Target, int32 ActionIndex)
{
	if (!IsValid(Target))
	{
		ClearHoldInfo();
		return;
	}

	UPRInteractionAction* HoldAction = Target->FindActionByIndex(ActionIndex);
	if (!IsValid(HoldAction) || !HoldAction->IsHoldTrigger())
	{
		ClearHoldInfo();
		return;
	}
	
	MulticastStartHold(Target, ActionIndex);
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HoldInfo.HoldTimerHandle, 
			this, &ThisClass::OnHoldFinished,
			HoldInfo.HoldDuration,false);
		HoldInfo.StartTime = World->GetTimeSeconds();
	}
}

void UPRInteractorComponent::ClearPreviousInteraction()
{
	// 이미 유지형 상호작용 중이면 먼저 종료
	if (ActiveInteractionInfo.IsValid())
	{
		Internal_EndActiveInteraction(true);
	}
	
	// 이미 홀드 중인 대상이 있었으면 종료
	if (HoldInfo.IsValid())
	{
		ServerCancelHold();
	}
}


