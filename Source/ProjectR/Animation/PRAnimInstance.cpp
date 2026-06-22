// Author: 김동석 (플레이어 이동(3인칭), 조준 오프셋 및 사망/좌수 IK 애니메이션 블렌딩 구현)
// Author: 배유찬 (재장전 시퀀스 연동 좌수 IK 보정 구현)
// Fill out your copyright notice in the Description page of Project Settings.

#include "PRAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

/*~ 초기화 및 업데이트 ~*/

void UPRAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<APRPlayerCharacter>(GetOwningActor());
	if (IsValid(PlayerCharacter))
	{
		CharacterMovement = PlayerCharacter->GetCharacterMovement();
	}
}

void UPRAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<APRPlayerCharacter>(GetOwningActor());
		if (IsValid(PlayerCharacter))
		{
			CharacterMovement = PlayerCharacter->GetCharacterMovement();
		}
	}
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	UpdateVelocity();
	UpdateAcceleration();
	UpdateDirection();
	UpdateFlags();
	UpdateMovementSpeedMultiplier();
	UpdateMovementMode();
	UpdateTurnInPlace();
	UpdateRootYawOffset();
	DetermineTargetTurnAngle();
	UpdateAim();
	UpdateLeftHandIK(DeltaSeconds);
}

void UPRAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
}

bool UPRAnimInstance::ShouldApplyLeftHandIK() const
{
	if (bIsFalling)
	{
		return false;
	}
	
	USkeletalMeshComponent* CharacterMeshComponent = GetSkelMeshComponent();
	if (!IsValid(CharacterMeshComponent) ||
		CharacterMeshComponent->GetBoneIndex(LeftHandIKTargetBoneName) == INDEX_NONE)
	{
		return false;
	}
	
	// 무기를 실제로 들고, aiming상태일때만 왼손 IK를 적용한다.
	if (!bIsAiming || ArmedState != EPRArmedState::Armed || EquippedWeaponSlot == EPRWeaponSlotType::None)
	{
		return false;
	}

	const UPRWeaponManagerComponent* WeaponManager = IsValid(PlayerCharacter) ? PlayerCharacter->GetWeaponManager() : nullptr;
	if (!IsValid(WeaponManager))
	{
		return false;
	}
	
	// WeaponActor의 ShouldApplyLeftHandIK 확인
	APRWeaponActor* ActiveWeaponActor = WeaponManager->GetActiveWeaponActor();
	if (!IsValid(ActiveWeaponActor) || !ActiveWeaponActor->ShouldApplyLeftHandIK())
	{
		return false;
	}
	
	// 방어 코드: WeaponMesh가 설정되지 않은 경우
	USkeletalMeshComponent* WeaponMeshComponent = ActiveWeaponActor->GetWeaponMeshComponent();
	if (!IsValid(WeaponMeshComponent) || !IsValid(WeaponMeshComponent->GetSkeletalMeshAsset()))
	{
		ensureMsgf(false, TEXT("Weapon Actor에 Mesh가 설정되지 않음"));
		return false;
	}
	
	return true;
}

APRWeaponActor* UPRAnimInstance::GetActiveWeaponActor() const
{
	const UPRWeaponManagerComponent* WeaponManager = IsValid(PlayerCharacter) ? PlayerCharacter->GetWeaponManager() : nullptr;
	if (!IsValid(WeaponManager))
	{
		return nullptr;
	}
	
	return  WeaponManager->GetActiveWeaponActor();
}

/*~ 이동 상태 갱신 ~*/

void UPRAnimInstance::UpdateVelocity()
{
	Velocity = CharacterMovement->Velocity;
	Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
	XYSpeed = Velocity.Size2D();

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	LocalVelocity2D = ActorRotation.UnrotateVector(Velocity2D);
}

void UPRAnimInstance::UpdateAcceleration()
{
	Acceleration = CharacterMovement->GetCurrentAcceleration();

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	const FVector AccelerationXY = FVector(Acceleration.X, Acceleration.Y, 0.0f);
	LocalAcceleration2D = ActorRotation.UnrotateVector(AccelerationXY);
}

void UPRAnimInstance::UpdateDirection()
{
	if (XYSpeed < MoveSpeedThreshold)
	{
		return;
	}

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity2D, ActorRotation);

	float TargetRefAngle = 0.0f;
	if (FMath::Abs(Direction) <= 22.5f)
	{
		TargetRefAngle = 0.0f;
	}
	else if (Direction > 22.5f && Direction <= 67.5f)
	{
		TargetRefAngle = 45.0f;
	}
	else if (Direction > 67.5f && Direction <= 112.5f)
	{
		TargetRefAngle = 90.0f;
	}
	else if (Direction > 112.5f && Direction <= 157.5f)
	{
		TargetRefAngle = 135.0f;
	}
	else if (FMath::Abs(Direction) > 157.5f)
	{
		TargetRefAngle = 180.0f;
	}
	else if (Direction < -22.5f && Direction >= -67.5f)
	{
		TargetRefAngle = -45.0f;
	}
	else if (Direction < -67.5f && Direction >= -112.5f)
	{
		TargetRefAngle = -90.0f;
	}
	else if (Direction < -112.5f && Direction >= -157.5f)
	{
		TargetRefAngle = -135.0f;
	}

	F_OrientationAngle = FRotator::NormalizeAxis(Direction - TargetRefAngle);
}

void UPRAnimInstance::UpdateFlags()
{
	bHasAcceleration = !LocalAcceleration2D.IsNearlyZero();
	bShouldMove = XYSpeed > MoveSpeedThreshold;
	bIsDecelerating = bShouldMove && !bHasAcceleration;
	bIsFalling = CharacterMovement->IsFalling();
	bIsCrouching = PlayerCharacter->IsCrouching();
	bIsSprint = PlayerCharacter->IsSprinting();
	bIsAiming = PlayerCharacter->IsAiming();
	bIsWalking = PlayerCharacter->IsWalking();
	bIsDown = PlayerCharacter->IsDown();
	bIsDodging = PlayerCharacter->IsDodging();
	bIsStrafeMode = bIsAiming || bIsWalking;

	if (bIsFalling)
	{
		LandState = ELandState::None;
	}
	else if (LandState == ELandState::None)
	{
		LandState = ELandState::Normal;
	}
}

void UPRAnimInstance::UpdateMovementMode()
{
	if (bIsDown)
	{
		MovementMode = EPRMovementMode::Down;
		return;
	}

	if (!bShouldMove)
	{
		MovementMode = EPRMovementMode::Idle;
		return;
	}

	if (bIsSprint)
	{
		MovementMode = EPRMovementMode::Sprinting;
		return;
	}

	const float Threshold = (PlayerCharacter->GetWalkSpeed() + PlayerCharacter->GetJogSpeed()) * 0.5f;
	MovementMode = XYSpeed <= Threshold ? EPRMovementMode::Walking : EPRMovementMode::Jogging;
}

void UPRAnimInstance::UpdateMovementSpeedMultiplier()
{
	MovementSpeedMultiplier = PlayerCharacter->GetMovementSpeedMultiplier();
}

/*~ 회전 및 조준 상태 갱신 ~*/

void UPRAnimInstance::UpdateTurnInPlace()
{
	const float TurnYawWeight = GetCurveValue(TurnYawWeightCurveName);
	const bool bWasTurning = bIsTurningInPlace;
	bIsTurningInPlace = TurnYawWeight > 0.1f;

	if (!bWasTurning && bIsTurningInPlace)
	{
		TurnStartYaw = RootYawOffset;
	}

	if (bWasTurning && !bIsTurningInPlace)
	{
		TargetTurnAngle = EPRTurnAngle::None;
	}
}

void UPRAnimInstance::UpdateRootYawOffset()
{
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

	if (bShouldMove || bIsFalling)
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, RootYawOffsetInterpSpeed);
		return;
	}

	const float TurnSwitch = GetCurveValue(TurnYawWeightCurveName);
	const float RotationAlpha = GetCurveValue(TurnRotationCurveName);
	const bool bWasTurning = bIsTurningInPlace;
	bIsTurningInPlace = TurnSwitch > 0.5f;

	if (bIsTurningInPlace)
	{
		if (!bWasTurning)
		{
			TurnStartYaw = CurrentDiff;
		}

		RootYawOffset = TurnStartYaw * (1.0f - FMath::Clamp(RotationAlpha, 0.0f, 1.0f));
	}
	else
	{
		RootYawOffset = 0.0f;
	}

	RemainingTurnYaw = RootYawOffset;
}

void UPRAnimInstance::UpdateAim()
{
	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

	ArmedState = EPRArmedState::Unarmed;
	EquippedWeaponSlot = EPRWeaponSlotType::None;

	if (const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
	{
		ArmedState = WeaponManager->GetArmedState();
		EquippedWeaponSlot = WeaponManager->GetCurrentWeaponSlot();
	}

	if (bIsStrafeMode)
	{
		AimYaw = CurrentDiff - RootYawOffset;
	}
	else if (FMath::Abs(CurrentDiff) < TurnThreshold - 5.0f)
	{
		AimYaw = CurrentDiff;
	}
	else
	{
		AimYaw = FMath::FInterpTo(AimYaw, 0.0f, GetWorld()->GetDeltaSeconds(), 5.0f);
	}

	AimPitch = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Pitch;
}

void UPRAnimInstance::UpdateLeftHandIK(float DeltaSeconds)
{
	if (!ShouldApplyLeftHandIK())
	{
		LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, 0.0f, DeltaSeconds, LeftHandIKBlendOutSpeed);
		if (LeftHandIKAlpha <= KINDA_SMALL_NUMBER)
		{
			ResetLeftHandIK();
		}
		return;
	}
	
	APRWeaponActor* ActiveWeaponActor = GetActiveWeaponActor();
	USkeletalMeshComponent* WeaponMeshComponent = ActiveWeaponActor->GetWeaponMeshComponent();
	if (!IsValid(WeaponMeshComponent))
	{
		return;
	}
	
	// 무기 메시의 왼손 그립 소켓을 월드 공간에서 가져온 뒤 무기별 보정값을 적용한다.
	const FTransform SocketWorldTransform = WeaponMeshComponent->GetSocketTransform(ActiveWeaponActor->LeftHandIKSocketName, RTS_World);
	const FTransform TargetWorldTransform = ActiveWeaponActor->LeftHandIKOffset * SocketWorldTransform;

	FVector EffectorLocation = FVector::ZeroVector;
	FRotator EffectorRotation = FRotator::ZeroRotator;
	// FABRIK 노드가 오른손 본 기준 Bone Space를 쓰도록 월드 트랜스폼을 오른손 본 공간으로 변환한다.
	GetSkelMeshComponent()->TransformToBoneSpace(
		LeftHandIKTargetBoneName,
		TargetWorldTransform.GetLocation(),
		TargetWorldTransform.Rotator(),
		EffectorLocation,
		EffectorRotation);

	LeftHandIKEffectorTransform = FTransform(EffectorRotation, EffectorLocation, FVector::OneVector);
	LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, 1.0f, DeltaSeconds, LeftHandIKBlendInSpeed);
	bHasLeftHandIKTarget = true;
}

void UPRAnimInstance::ResetLeftHandIK()
{
	// 유효하지 않은 이전 프레임 타깃이 AnimGraph에 남지 않도록 기본값으로 되돌린다.
	LeftHandIKEffectorTransform = FTransform::Identity;
	LeftHandIKAlpha = 0.0f;
	bHasLeftHandIKTarget = false;
}

void UPRAnimInstance::DetermineTargetTurnAngle()
{
	if (bIsTurningInPlace || !bIsStrafeMode)
	{
		TargetTurnAngle = EPRTurnAngle::None;
		return;
	}

	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float AbsDiff = FMath::Abs(UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw);

	TargetTurnAngle = AbsDiff > TurnThreshold ? EPRTurnAngle::Angle90 : EPRTurnAngle::None;
}
