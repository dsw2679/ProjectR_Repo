// Author: 김동석 (Animation 타입 및 구조체 정의)
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRAnimationTypes.generated.h"

// 캐릭터 착지 강도
UENUM(BlueprintType)
enum class ELandState : uint8
{
	None,
	Normal,
	Soft,
	Heavy,
};

// 캐릭터 이동 방향
UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	Forward,
	ForwardRight,
	ForwardLeft,
	Backward,
	BackwardRight,
	BackwardLeft
};

// 캐릭터 이동 모드 (애니메이션 판별용)
UENUM(BlueprintType)
enum class EPRMovementMode : uint8
{
	Idle,
	Walking,
	Jogging,
	Sprinting,
	Down
};

// 캐릭터 턴 각도
UENUM(BlueprintType)
enum class EPRTurnAngle : uint8
{
	None,
	Angle90
};

// 캐릭터 회피 애니메이션 종류
UENUM(BlueprintType)
enum class EPRDodgeAnimationType : uint8
{
	ForwardRoll,
	BackStep
};

// 원격 클라이언트에 전달할 회피 애니메이션 요청 정보
USTRUCT(BlueprintType)
struct FPRDodgeAnimationRequest
{
	GENERATED_BODY()

	// 회피 요청 순번이다
	UPROPERTY(BlueprintReadOnly)
	int32 RequestId = 0;

	// 월드 기준 회피 방향이다
	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	// 재생할 회피 애니메이션 종류다
	UPROPERTY(BlueprintReadOnly)
	EPRDodgeAnimationType AnimationType = EPRDodgeAnimationType::ForwardRoll;
};
