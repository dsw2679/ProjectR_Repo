// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy 애니메이션 인스턴스 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PREnemyAnimInstance.generated.h"

class UCharacterMovementComponent;
class APREnemyBaseCharacter;

// 적 ABP가 공통으로 사용하는 이동/상태/전투 표현 값을 계산하는 AnimInstance다.
// 블루프린트는 이 클래스가 계산한 값을 읽어 상태머신과 AimOffset 연결에 집중한다.
UCLASS()
class PROJECTR_API UPREnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// 애님 인스턴스 초기 참조를 캐시한다.
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 적 이동/전투 표현용 값을 갱신한다.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// Pawn, MovementComponent 참조를 다시 캐시한다.
	void RefreshOwnerReferences();

	// 속도, 로컬 이동축, 방향처럼 이동 기반 값을 갱신한다.
	void UpdateMovementData();

	// Focus 유지, 전투 이동 포즈 사용 여부 같은 표현 문맥을 갱신한다.
	void UpdateCombatPresentationData();

	// BaseAimRotation 기준 AimOffset 입력값을 갱신한다.
	void UpdateAimData();

public:
	// 이 AnimInstance가 붙어 있는 적 캐릭터다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<APREnemyBaseCharacter> EnemyCharacter;

	// 이동 속성 갱신에 사용하는 CharacterMovement 참조다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	// 월드 기준 실제 속도 벡터다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	FVector Velocity = FVector::ZeroVector;

	// 캐릭터 로컬 기준 2D 속도 벡터다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	FVector LocalVelocity2D = FVector::ZeroVector;

	// 월드 기준 수평 이동 속력이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float XYSpeed = 0.0f;

	// 캐릭터 로컬 전방 기준 속도다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float ForwardSpeed = 0.0f;

	// 캐릭터 로컬 우측 기준 속도다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float RightSpeed = 0.0f;

	// 현재 이동 방향을 각도로 환산한 값이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float Direction = 0.0f;

	// Locomotion StateMachine에서 Idle/Move 전환에 사용한다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bShouldMove = false;

	// 현재 공중 상태 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsFalling = false;

	// EQS 전투 이동 중 타겟 Focus를 유지해야 하는지 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bMaintainCombatMoveFocus = false;

	// EQS 전투 이동 중 AimOffset을 써야 하는지 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatAimOffset = false;

	// 전투 이동 포즈 세트를 써야 하는지 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatMovePose = false;

	// 현재 전투 strafe 표현 상태로 볼 수 있는지 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsCombatStrafe = false;

	// Perch/Gargoyle 대기 전용 Idle 포즈를 우선 재생해야 하는지 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUsePerchIdlePose = false;

	// AimOffset용 Yaw 입력값이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float AimYaw = 0.0f;

	// AimOffset용 Pitch 입력값이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float AimPitch = 0.0f;

	// 아주 작은 속도 흔들림으로 Move 상태에 들어가는 것을 막기 위한 기준값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	float MoveSpeedThreshold = 3.0f;

	// AimOffset Yaw 보정 최대값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	float MaxAimYaw = 90.0f;

	// AimOffset Pitch 보정 최대값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	float MaxAimPitch = 60.0f;
};
