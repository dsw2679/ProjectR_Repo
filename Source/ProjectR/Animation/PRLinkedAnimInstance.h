// Author: 김동석 (Linked 애니메이션 인스턴스 구현)
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRAnimationTypes.h"
#include "Animation/AnimInstance.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRLinkedAnimInstance.generated.h"

class UPRAnimInstance;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRLinkedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 레이어 애니메이션 인스턴스 초기화 시 메인 AnimInstance를 캐시한다
	virtual void NativeInitializeAnimation() override;

	// 메인 AnimInstance의 애니메이션 변수를 레이어로 복사한다
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPRAnimInstance> MainAnimInstance;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsSprint;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouching;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsWalking;
	
	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bShouldMove;

	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bIsDecelerating;
	
	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bIsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRMovementMode MovementMode;
	
	// 전방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float F_OrientationAngle;
	
	// 우측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FR_OrientationAngle;
	
	// 좌측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FL_OrientationAngle;
	
	// 후방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float B_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BL_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BR_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ECardinalDirection CardinalDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimPitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float RootYawOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float XYSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float MovementSpeedMultiplier = 1.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;
	
	// 레이어의 AnimGraph에서 사용할 변수
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRTurnAngle TargetTurnAngle = EPRTurnAngle::None;
	
	// 현재 무장 상태
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;
	
	// 현재 장착 또는 활성화된 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRWeaponSlotType EquippedWeaponSlot = EPRWeaponSlotType::None;

	// 현재 AimOffset 선택에 사용할 무기 슬롯
	// UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	// EPRWeaponSlotType AimOffsetWeaponSlot = EPRWeaponSlotType::None;

	// 메인 AnimInstance에서 계산한 FABRIK용 왼손 Effector 트랜스폼이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	FTransform LeftHandIKEffectorTransform = FTransform::Identity;

	// 레이어 AnimGraph에서 사용할 왼손 IK 적용 비율이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	float LeftHandIKAlpha = 0.0f;

	// 메인 AnimInstance가 현재 프레임에 유효한 왼손 IK 타깃을 찾았는지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	bool bHasLeftHandIKTarget = false;

	// Effector 트랜스폼이 기준으로 삼는 오른손 본 이름이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|IK")
	FName LeftHandIKTargetBoneName = FName(TEXT("Bone_M_Hand_R"));

	// DodgeAbility가 활성화되어 회피 상태 태그를 소유 중인지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodging = false;
};
