// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Flashlight 컴포넌트 구현)
#include "ProjectR/Player/Components/PRFlashlightComponent.h"

#include "GameFramework/Pawn.h"

UPRFlashlightComponent::UPRFlashlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bAutoActivate = false;
	
	SetVisibility(false);
	SetCastShadows(false);
	
	Intensity = 5000.0f;
	AttenuationRadius = 2500.0f;
	InnerConeAngle = 17.0f;
	OuterConeAngle = 45.0f;
	VolumetricScatteringIntensity = 1.6f;
}

void UPRFlashlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 비활성 상태 방어
	if (!bFlashlightEnabled)
	{
		return;
	}

	UpdateFlashlightRotation(DeltaTime);
}

void UPRFlashlightComponent::SetFlashlightEnabled(bool bNewEnabled)
{
	bFlashlightEnabled = bNewEnabled;
	SetComponentTickEnabled(bFlashlightEnabled);
	SetVisibility(bFlashlightEnabled);

	if (bFlashlightEnabled)
	{
		Activate(true);
		UpdateFlashlightRotation(0.0f);
	}
	else
	{
		Deactivate();
	}
}

void UPRFlashlightComponent::UpdateFlashlightRotation(float DeltaTime)
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled())
	{
		SetFlashlightEnabled(false);
		return;
	}

	const FRotator AimRotation = OwnerPawn->GetBaseAimRotation();
	const FRotator ActorRotation = OwnerPawn->GetActorRotation();
	const FRotator AimDeltaRotation = AimRotation - ActorRotation;

	const float AimYaw = FMath::Clamp(FRotator::NormalizeAxis(AimDeltaRotation.Yaw), MinAimYaw, MaxAimYaw);
	const float AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimDeltaRotation.Pitch), MinAimPitch, MaxAimPitch);
	const FRotator TargetRotation(AimPitch, AimYaw, 0.0f);

	if (DeltaTime <= 0.0f || RotationInterpSpeed <= 0.0f)
	{
		SetRelativeRotation(TargetRotation);
		return;
	}

	SetRelativeRotation(FMath::RInterpTo(GetRelativeRotation(), TargetRotation, DeltaTime, RotationInterpSpeed));
}
