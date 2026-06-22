// Author: 김동석 (Linked 애니메이션 인스턴스 구현)
// Fill out your copyright notice in the Description page of Project Settings.


#include "PRLinkedAnimInstance.h"

#include "PRAnimInstance.h"

/*~ 초기화 및 업데이트 ~*/

void UPRLinkedAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	MainAnimInstance = Cast<UPRAnimInstance>(Blueprint_GetMainAnimInstance());
}

void UPRLinkedAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!IsValid(MainAnimInstance))
	{
		MainAnimInstance = Cast<UPRAnimInstance>(Blueprint_GetMainAnimInstance());
	}
	
	if (!IsValid(MainAnimInstance))
	{
		return;
	}

	// Linked AnimInstance는 회피 몽타주를 재생하지 않는다.
	// 회피 몽타주는 DodgeAbility가 소유하고, 레이어는 상태 변수만 읽어 그래프 표현을 보조한다.
	// 링크드 레이어 AnimBP가 메인 AnimInstance의 이동 상태를 그대로 읽을 수 있도록 복사한다.
	MovementMode = MainAnimInstance->MovementMode;
	bShouldMove = MainAnimInstance->bShouldMove;
	bIsDecelerating = MainAnimInstance->bIsDecelerating;
	bIsFalling = MainAnimInstance->bIsFalling;
	bIsSprint = MainAnimInstance->bIsSprint;
	bIsCrouching = MainAnimInstance->bIsCrouching;
	bIsAiming = MainAnimInstance->bIsAiming;
	bIsWalking = MainAnimInstance->bIsWalking;
	F_OrientationAngle = MainAnimInstance->F_OrientationAngle;
	FR_OrientationAngle = MainAnimInstance->FR_OrientationAngle;
	FL_OrientationAngle = MainAnimInstance->FL_OrientationAngle;
	B_OrientationAngle = MainAnimInstance->B_OrientationAngle;
	BL_OrientationAngle = MainAnimInstance->BL_OrientationAngle;
	BR_OrientationAngle = MainAnimInstance->BR_OrientationAngle;
	CardinalDirection = MainAnimInstance->CardinalDirection;
	AimPitch = MainAnimInstance->AimPitch;
	AimYaw = MainAnimInstance->AimYaw;
	RootYawOffset = MainAnimInstance->RootYawOffset;
	TargetTurnAngle = MainAnimInstance->TargetTurnAngle;
	XYSpeed = MainAnimInstance->XYSpeed;
	MovementSpeedMultiplier = MainAnimInstance->MovementSpeedMultiplier;
	Direction = MainAnimInstance->Direction;
	ArmedState = MainAnimInstance->ArmedState;
	EquippedWeaponSlot = MainAnimInstance->EquippedWeaponSlot;
	//AimOffsetWeaponSlot = MainAnimInstance->AimOffsetWeaponSlot;
	LeftHandIKEffectorTransform = MainAnimInstance->LeftHandIKEffectorTransform;
	LeftHandIKAlpha = MainAnimInstance->LeftHandIKAlpha;
	bHasLeftHandIKTarget = MainAnimInstance->bHasLeftHandIKTarget;
	LeftHandIKTargetBoneName = MainAnimInstance->LeftHandIKTargetBoneName;

	// 회피 상태는 Ability 태그를 관찰한 메인 AnimInstance 값을 복사한다.
	bIsDodging = MainAnimInstance->bIsDodging;
}
