// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Pattern VFX 컴포넌트 구현)
#include "PRFaerinPatternVFXComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UPRFaerinPatternVFXComponent::UPRFaerinPatternVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRFaerinPatternVFXComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bShowGroundCircleOnBeginPlay)
	{
		ShowGroundCircleVFX();
	}
	else if (bHideGroundCircleOnBeginPlay)
	{
		HideGroundCircleVFX();
	}
}

void UPRFaerinPatternVFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyGroundCircleVFXComponent();
	Super::EndPlay(EndPlayReason);
}

void UPRFaerinPatternVFXComponent::ShowGroundCircleVFX()
{
	UNiagaraComponent* NiagaraComponent = EnsureGroundCircleVFXComponent();
	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	NiagaraComponent->SetHiddenInGame(false);
	NiagaraComponent->Activate(true);
}

void UPRFaerinPatternVFXComponent::HideGroundCircleVFX()
{
	if (!IsValid(GroundCircleVFXComponent))
	{
		return;
	}

	GroundCircleVFXComponent->Deactivate();
	GroundCircleVFXComponent->SetHiddenInGame(true);
}

void UPRFaerinPatternVFXComponent::SetGroundCircleVFXVisible(const bool bVisible)
{
	if (bVisible)
	{
		ShowGroundCircleVFX();
	}
	else
	{
		HideGroundCircleVFX();
	}
}

void UPRFaerinPatternVFXComponent::ResetPatternVFX()
{
	if (bResetGroundCircleToVisible)
	{
		ShowGroundCircleVFX();
	}
	else
	{
		HideGroundCircleVFX();
	}
}

USceneComponent* UPRFaerinPatternVFXComponent::ResolveGroundCircleAttachComponent() const
{
	if (IsValid(GroundCircleAttachComponent))
	{
		return GroundCircleAttachComponent;
	}

	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) ? OwnerActor->GetRootComponent() : nullptr;
}

UNiagaraComponent* UPRFaerinPatternVFXComponent::EnsureGroundCircleVFXComponent()
{
	if (IsValid(GroundCircleVFXComponent))
	{
		return GroundCircleVFXComponent;
	}

	if (!IsValid(GroundCircleVFXSystem))
	{
		return nullptr;
	}

	USceneComponent* AttachComponent = ResolveGroundCircleAttachComponent();
	if (!IsValid(AttachComponent))
	{
		return nullptr;
	}

	GroundCircleVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		GroundCircleVFXSystem,
		AttachComponent,
		GroundCircleAttachSocketName,
		GroundCircleRelativeLocation,
		GroundCircleRelativeRotation,
		GroundCircleRelativeScale,
		EAttachLocation::KeepRelativeOffset,
		false,
		ENCPoolMethod::None,
		false,
		false);

	if (!IsValid(GroundCircleVFXComponent))
	{
		return nullptr;
	}

	GroundCircleVFXComponent->SetRelativeLocation(GroundCircleRelativeLocation);
	GroundCircleVFXComponent->SetRelativeRotation(GroundCircleRelativeRotation);
	GroundCircleVFXComponent->SetRelativeScale3D(GroundCircleRelativeScale);
	GroundCircleVFXComponent->SetHiddenInGame(true);
	return GroundCircleVFXComponent;
}

void UPRFaerinPatternVFXComponent::DestroyGroundCircleVFXComponent()
{
	if (!IsValid(GroundCircleVFXComponent))
	{
		return;
	}

	GroundCircleVFXComponent->Deactivate();
	GroundCircleVFXComponent->DestroyComponent();
	GroundCircleVFXComponent = nullptr;
}
