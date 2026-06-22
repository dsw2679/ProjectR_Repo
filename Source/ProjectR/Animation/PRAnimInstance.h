// Author: 김동석 (플레이어 이동(3인칭), 조준 오프셋 및 사망/좌수 IK 애니메이션 블렌딩 구현)
// Author: 배유찬 (재장전 시퀀스 연동 좌수 IK 보정 구현)
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PRAnimationTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRAnimInstance.generated.h"

class APRWeaponActor;
class APRPlayerCharacter;
class UCharacterMovementComponent;

/**
 * 플레이어 메인 AnimInstance다.
 * 이동, 조준, 무장 상태처럼 AnimGraph가 상시 참조하는 값을 갱신한다.
 */
UCLASS()
class PROJECTR_API UPRAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** 애니메이션 인스턴스 초기화 시 캐릭터와 이동 컴포넌트 참조를 캐시한다 */
	virtual void NativeInitializeAnimation() override;

	/** 매 프레임 캐릭터 이동, 조준, 무장, 액션 태그 상태를 갱신한다 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 평가 이후 일회성 애니메이션 요청 상태를 정리한다 */
	virtual void NativePostEvaluateAnimation() override;

protected:
	// 왼손 IK 적용 가능 여부를 확인
	virtual bool ShouldApplyLeftHandIK() const;
	
	// 현재 장착 중인 무기 Actor를 반환
	APRWeaponActor* GetActiveWeaponActor() const;
	
private:
	void UpdateVelocity();
	void UpdateAcceleration();
	void UpdateDirection();
	void UpdateFlags();
	void UpdateMovementMode();
	void UpdateMovementSpeedMultiplier();
	void UpdateAim();
	// 현재 무기의 왼손 그립 소켓 캐시 및 FABRIK Alpha 보간 처리
	void UpdateLeftHandIK(float DeltaSeconds);
	// 왼손 IK 타깃이 유효하지 않을 때 AnimGraph 입력값을 초기화한다
	void ResetLeftHandIK();
	void UpdateTurnInPlace();
	void UpdateRootYawOffset();
	void DetermineTargetTurnAngle();

public:
	/*~ 캐릭터 참조 ~*/
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APRPlayerCharacter> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/*~ 이동 상태 ~*/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRMovementMode MovementMode;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float MoveSpeedThreshold = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Acceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector LocalVelocity2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector LocalAcceleration2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float XYSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float MovementSpeedMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CameraX;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ECardinalDirection CardinalDirection;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float F_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FR_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FL_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float B_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BR_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BL_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsDecelerating;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprint;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsWalking;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsDown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ELandState LandState;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	bool bIsTurningInPlace;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RemainingTurnYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RootYawOffset;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnStartThreshold = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnEndThreshold = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float RootYawOffsetInterpSpeed = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRTurnAngle TargetTurnAngle = EPRTurnAngle::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnYawWeightCurveName = FName(TEXT("TurnYawWeight"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnRotationCurveName = FName(TEXT("TurnRotationAlpha"));

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsStrafeMode;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRWeaponSlotType EquippedWeaponSlot = EPRWeaponSlotType::None;
	
	// FABRIK 노드에 전달할 왼손 Effector 트랜스폼이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	FTransform LeftHandIKEffectorTransform = FTransform::Identity;

	// 왼손 IK 적용 비율이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	float LeftHandIKAlpha = 0.0f;

	// 현재 프레임에 사용할 왼손 IK 타깃이 준비되었는지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	bool bHasLeftHandIKTarget = false;

	// Effector 트랜스폼을 계산할 기준 오른손 본 이름이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	FName LeftHandIKTargetBoneName = FName(TEXT("Bone_M_Hand_R"));

	// 왼손 IK가 다시 유효해질 때 Alpha 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|IK", meta = (ClampMin = "0.01"))
	float LeftHandIKBlendInSpeed = 10.0f;

	// 왼손 IK가 억제될 때 Alpha 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|IK", meta = (ClampMin = "0.01"))
	float LeftHandIKBlendOutSpeed = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodging = false;

private:
	float TurnStartYaw = 0.0f;
	float DistanceCurve = 0.0f;
	float LastDistanceCurve = 0.0f;
	FRotator MovingRotation = FRotator::ZeroRotator;
	FRotator LastMovingRotation = FRotator::ZeroRotator;
	const float TurnThreshold = 90.0f;
	FRotator LastCameraRotation;
};
