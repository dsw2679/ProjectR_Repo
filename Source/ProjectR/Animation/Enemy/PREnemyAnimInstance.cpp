// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy 애니메이션 인스턴스 구현)
#include "PREnemyAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"

// ===== UAnimInstance Interface =====

void UPREnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RefreshOwnerReferences();
}

void UPREnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(EnemyCharacter))
	{
		RefreshOwnerReferences();
	}

	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	UpdateMovementData();
	UpdateCombatPresentationData();
	UpdateAimData();
}

// ===== 내부 갱신 =====

void UPREnemyAnimInstance::RefreshOwnerReferences()
{
	// AnimInstance 초기화 시점에는 Pawn 참조가 아직 없을 수 있어 Update에서도 다시 캐시한다.
	EnemyCharacter = Cast<APREnemyBaseCharacter>(TryGetPawnOwner());
	CharacterMovement = IsValid(EnemyCharacter) ? EnemyCharacter->GetCharacterMovement() : nullptr;
}

void UPREnemyAnimInstance::UpdateMovementData()
{
	if (!IsValid(EnemyCharacter) || !IsValid(CharacterMovement))
	{
		Velocity = FVector::ZeroVector;
		LocalVelocity2D = FVector::ZeroVector;
		XYSpeed = 0.0f;
		ForwardSpeed = 0.0f;
		RightSpeed = 0.0f;
		Direction = 0.0f;
		bShouldMove = false;
		bIsFalling = false;
		return;
	}

	Velocity = EnemyCharacter->GetVelocity();
	Velocity.Z = 0.0f;
	LocalVelocity2D = EnemyCharacter->GetActorRotation().UnrotateVector(Velocity);
	LocalVelocity2D.Z = 0.0f;

	XYSpeed = Velocity.Size2D();
	ForwardSpeed = LocalVelocity2D.X;
	RightSpeed = LocalVelocity2D.Y;
	bShouldMove = XYSpeed > MoveSpeedThreshold;
	bIsFalling = CharacterMovement->IsFalling();

	if (bShouldMove)
	{
		Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, EnemyCharacter->GetActorRotation());
	}
	else
	{
		Direction = 0.0f;
	}
}

void UPREnemyAnimInstance::UpdateCombatPresentationData()
{
	bMaintainCombatMoveFocus = false;
	bUseCombatAimOffset = false;
	bUseCombatMovePose = false;
	bIsCombatStrafe = false;
	bUsePerchIdlePose = false;

	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	bMaintainCombatMoveFocus = EnemyCharacter->ShouldMaintainCombatMoveFocus();
	bUseCombatMovePose = EnemyCharacter->ShouldUseCombatMovePose();
	bUseCombatAimOffset = EnemyCharacter->ShouldUseCombatAimOffset();
	bIsCombatStrafe = bShouldMove && EnemyCharacter->ShouldUseCombatStrafeState();
	bUsePerchIdlePose = EnemyCharacter->ShouldUsePerchIdlePose();
}

void UPREnemyAnimInstance::UpdateAimData()
{
	AimYaw = 0.0f;
	AimPitch = 0.0f;

	if (!IsValid(EnemyCharacter) || !bUseCombatAimOffset)
	{
		return;
	}

	const FRotator ActorRotation = EnemyCharacter->GetActorRotation();
	const FRotator BaseAimRotation = EnemyCharacter->GetBaseAimRotation();
	const FRotator AimDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(BaseAimRotation, ActorRotation);

	AimYaw = FMath::Clamp(FRotator::NormalizeAxis(AimDeltaRotation.Yaw), -MaxAimYaw, MaxAimYaw);
	AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimDeltaRotation.Pitch), -MaxAimPitch, MaxAimPitch);
}
