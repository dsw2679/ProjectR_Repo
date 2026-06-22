// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interactable 컴포넌트 구현)
#include "PRInteractableComponent.h"
#include "PRInteractionAction.h"
#include "Components/MeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

UPRInteractableComponent::UPRInteractableComponent()
{
	SetIsReplicatedByDefault(true);
}

void UPRInteractableComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentInteractor);
}

FVector UPRInteractableComponent::GetActorLocation() const
{
	if (GetOwner())
	{
		return GetOwner()->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UPRInteractableComponent::OnRep_CurrentInteractor()
{
	// TODO : 클라측 피드백 (사용 중 표시 등)
}

void UPRInteractableComponent::ApplyDepthStencilValues(bool bIsInRange)
{
	bDepthStencilApplied = true;
	
	TArray<UMeshComponent*> Meshes;
	UPRGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	SavedCustomDepthStates.Reset();
	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			// 원래 RenderCustomDepth 상태 저장
			SavedCustomDepthStates.Add(TObjectPtr<UMeshComponent>(Mesh), Mesh->bRenderCustomDepth);

			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue(bIsInRange ? PRStencilValues::Interaction : PRStencilValues::Highlight);
		}
	}
}

void UPRInteractableComponent::OnFocus(AActor* Viewer, bool bIsInRange)
{
	bIsFocused = true;

	if (ShouldApplyDepthStencilValue(Viewer))
	{
		ApplyDepthStencilValues(bIsInRange);
	}

	BroadcastInteractableEvent(Viewer, bIsInRange);
}

void UPRInteractableComponent::UpdateFocus(AActor* Viewer, bool bIsInRange)
{
	if (ShouldApplyDepthStencilValue(Viewer))
	{
		if (!bDepthStencilApplied)
		{
			ApplyDepthStencilValues(bIsInRange);
		}
		else
		{
			for (auto& DepthState : SavedCustomDepthStates)
			{
				UMeshComponent* Mesh = DepthState.Key;
				if (IsValid(Mesh))
				{
					Mesh->SetCustomDepthStencilValue(bIsInRange ? PRStencilValues::Interaction : PRStencilValues::Highlight);
				}
			}
		}
	}
	else
	{
		ResetDepthStencilValues();
	}

	BroadcastInteractableEvent(Viewer, bIsInRange);
}


void UPRInteractableComponent::OnUnfocus()
{
	bIsFocused = false;

	if (IsDepthStencilApplied())
	{
		ResetDepthStencilValues();
	}

	BroadcastInteractableEvent(nullptr, false);
}

void UPRInteractableComponent::BroadcastInteractableEvent(AActor* Viewer, bool bIsInRange) const
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

	const UPRInteractionAction* BestAction = IsValid(Viewer) ? SelectBestAction(Viewer) : nullptr;

	FPRInteractableEventPayload Payload;
	// 거리 내에 있고, 선택된 Action 이 힌트 표시를 허용해야 프롬프트 표시
	Payload.bShowPrompt = bIsFocused && bIsInRange && IsValid(BestAction) && BestAction->ShouldShowHint(Viewer);
	Payload.bCanInteract = IsValid(Viewer) && HasAvailableAction(Viewer);
	if (IsValid(BestAction))
	{
		Payload.ActionHintText = BestAction->ActionHintText;
	}

	EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_Interactable, Payload);
}


void UPRInteractableComponent::ResetDepthStencilValues()
{
	TArray<UMeshComponent*> Meshes;
	UPRGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetCustomDepthStencilValue(PRStencilValues::Default);

		// 원래 상태로 복원. 저장된 값이 없으면 false로 초기화
		const bool* bWasEnabled = SavedCustomDepthStates.Find(TObjectPtr<UMeshComponent>(Mesh));
		Mesh->SetRenderCustomDepth(bWasEnabled ? *bWasEnabled : false);
	}
	
	bDepthStencilApplied = false;
	SavedCustomDepthStates.Reset();
}

void UPRInteractableComponent::OnInteract(AActor* Interactor, int32 ActionIndex)
{
	UPRInteractionAction* TargetAction = FindActionByIndex(ActionIndex);
	if (!IsValid(TargetAction))
	{
		return;
	}

	TargetAction->Execute(Interactor);

	// 유지형 Action이면 활성 Action으로 추적
	if (TargetAction->ShouldSustained())
	{
		ActiveAction = TargetAction;
	}
}

UPRInteractionAction* UPRInteractableComponent::SelectBestAction(AActor* Interactor) const
{
	UPRInteractionAction* BestAction = nullptr;
	int32 BestPriority = INT32_MIN;

	for (UPRInteractionAction* Action : InteractionActions)
	{
		if (!IsValid(Action))
		{
			continue;
		}

		if (!Action->CanInteract(Interactor))
		{
			continue;
		}

		const int32 ActionPriority = Action->GetPriority();
		if (ActionPriority > BestPriority)
		{
			BestPriority = ActionPriority;
			BestAction = Action;
		}
	}

	return BestAction;
}


void UPRInteractableComponent::EndActiveInteraction(AActor* Interactor, bool bCanceled)
{
	if (IsValid(ActiveAction))
	{
		ActiveAction->EndInteraction(Interactor, bCanceled);
		ActiveAction = nullptr;
	}

	// 배타 점유 해제 (Sustained 종료 동시에 점유도 풀림)
	ReleaseExclusive();
}

UPRInteractionAction* UPRInteractableComponent::FindActionByIndex(int32 InActionIndex) const
{
	if (InteractionActions.IsValidIndex(InActionIndex))
	{
		return InteractionActions[InActionIndex];
	}
	return nullptr;
}

int32 UPRInteractableComponent::FindActionIndex(UPRInteractionAction* InAction) const
{
	int32 Index = -1;
	InteractionActions.Find(InAction, Index);
	return Index;
}

bool UPRInteractableComponent::HasAvailableAction(AActor* Interactor) const
{
	for (UPRInteractionAction* Action : InteractionActions)
	{
		if (IsValid(Action) && Action->CanInteract(Interactor))
		{
			return true;
		}
	}
	return false;
}

bool UPRInteractableComponent::CanBeInteractedBy(AActor* Interactor) const
{
	if (!bExclusiveInteraction)
	{
		return true;
	}

	// 배타 점유 모드: 비점유 또는 본인 점유 시에만 진입 가능
	return !IsValid(CurrentInteractor) || CurrentInteractor == Interactor;
}

void UPRInteractableComponent::AcquireExclusive(AActor* Interactor)
{
	if (bExclusiveInteraction)
	{
		CurrentInteractor = Interactor;
	}
}

void UPRInteractableComponent::ReleaseExclusive()
{
	if (bExclusiveInteraction)
	{
		CurrentInteractor = nullptr;
	}
}

bool UPRInteractableComponent::ShouldApplyDepthStencilValue(AActor* Viewer) const
{
	return !bOnlyApplyDepthStencilOnAvailable || HasAvailableAction(Viewer) || IsHolding();
}
