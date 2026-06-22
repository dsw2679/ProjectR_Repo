// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Barrier Anchor Actor 구현)
#include "PRBarrierAnchorActor.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

APRBarrierAnchorActor::APRBarrierAnchorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicatingMovement(true);

	BarrierArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("BarrierArm"));
	SetRootComponent(BarrierArm);
	BarrierArm->bDoCollisionTest = false;
}

/*~ AActor Interface ~*/

void APRBarrierAnchorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		ApplySourceControlRotation();
	}
}

void APRBarrierAnchorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBarrierAnchorActor, SourceActor);
	DOREPLIFETIME(APRBarrierAnchorActor, AnchorConfig);
}

/*~ 외부 구동 ~*/

void APRBarrierAnchorActor::InitializeAnchor(AActor* InSourceActor, const UPRBarrierAbilityDataAsset* BarrierData)
{
	if (!HasAuthority() || !IsValid(InSourceActor) || !IsValid(BarrierData))
	{
		return;
	}

	SourceActor = InSourceActor;
	AnchorConfig = BarrierData->AnchorConfig;

	AttachToActor(SourceActor, FAttachmentTransformRules::KeepRelativeTransform);
	SetActorRelativeTransform(AnchorConfig.AnchorRelativeTransform);
	ApplyAnchorConfig();
	ForceNetUpdate();
}

USceneComponent* APRBarrierAnchorActor::GetBarrierAttachComponent() const
{
	return BarrierArm.Get();
}

FName APRBarrierAnchorActor::GetBarrierAttachSocketName() const
{
	return USpringArmComponent::SocketName;
}

/*~ 복제 처리 ~*/

void APRBarrierAnchorActor::OnRep_SourceActor()
{
	if (IsValid(SourceActor) && GetAttachParentActor() != SourceActor)
	{
		// 소스 부착 복원
		AttachToActor(SourceActor, FAttachmentTransformRules::KeepRelativeTransform);
		SetActorRelativeTransform(AnchorConfig.AnchorRelativeTransform);
	}
}

void APRBarrierAnchorActor::OnRep_AnchorConfig()
{
	ApplyAnchorConfig();
}

/*~ 설정 처리 ~*/

void APRBarrierAnchorActor::ApplyAnchorConfig()
{
	if (!IsValid(BarrierArm))
	{
		return;
	}

	// 스프링 암 설정
	BarrierArm->TargetArmLength = AnchorConfig.TargetArmLength;
	BarrierArm->TargetOffset = AnchorConfig.TargetOffset;
	BarrierArm->SocketOffset = AnchorConfig.SocketOffset;
	BarrierArm->bDoCollisionTest = false;
	BarrierArm->bUsePawnControlRotation = false;
	BarrierArm->bInheritPitch = AnchorConfig.bInheritPitch;
	BarrierArm->bInheritYaw = AnchorConfig.bInheritYaw;
	BarrierArm->bInheritRoll = AnchorConfig.bInheritRoll;
	BarrierArm->bEnableCameraRotationLag = AnchorConfig.bEnableCameraRotationLag;
	BarrierArm->CameraRotationLagSpeed = AnchorConfig.CameraRotationLagSpeed;

	SetActorTickEnabled(HasAuthority() && AnchorConfig.bUsePawnControlRotation);
	if (HasAuthority() && AnchorConfig.bUsePawnControlRotation)
	{
		ApplySourceControlRotation();
	}
}

void APRBarrierAnchorActor::ApplySourceControlRotation()
{
	if (!AnchorConfig.bUsePawnControlRotation)
	{
		return;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceActor);
	if (!IsValid(SourcePawn))
	{
		return;
	}

	// 소스 조준 회전 추적
	const FRotator SourceViewRotation = SourcePawn->GetViewRotation();
	const FRotator RelativeRotation = AnchorConfig.AnchorRelativeTransform.GetRotation().Rotator();
	SetActorRotation(SourceViewRotation + RelativeRotation);
}
