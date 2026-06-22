// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Barrier 어빌리티 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRBarrierAbilityDataAsset.generated.h"

class APRBarrierAnchorActor;
class APRGroundBoxProjectileBase;

// 배리어 앵커 액터의 부착 및 스프링 암 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBarrierAnchorConfig
{
	GENERATED_BODY()

	// 소스 액터 기준 앵커 상대 트랜스폼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|Anchor")
	FTransform AnchorRelativeTransform = FTransform::Identity;

	// 스프링 암 길이
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	float TargetArmLength = -200.0f;

	// 스프링 암 목표 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	FVector TargetOffset = FVector::ZeroVector;

	// 스프링 암 소켓 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	FVector SocketOffset = FVector::ZeroVector;

	// 소스 Pawn 조준 회전 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	bool bUsePawnControlRotation = false;

	// Pitch 상속 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	bool bInheritPitch = true;

	// Yaw 상속 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	bool bInheritYaw = true;

	// Roll 상속 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	bool bInheritRoll = true;

	// 회전 랙 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm")
	bool bEnableCameraRotationLag = false;

	// 회전 랙 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|SpringArm", meta = (ClampMin = "0.0"))
	float CameraRotationLagSpeed = 5.0f;
};

// 플레이어와 Penitent가 공유하는 배리어 어빌리티 설정 데이터
UCLASS(BlueprintType)
class PROJECTR_API UPRBarrierAbilityDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPRBarrierAbilityDataAsset();

public:
	// 생성할 배리어 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier")
	TSubclassOf<APRGroundBoxProjectileBase> BarrierActorClass;

	// 생성할 배리어 앵커 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier")
	TSubclassOf<APRBarrierAnchorActor> BarrierAnchorActorClass;

	// 배리어 앵커 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier")
	FPRBarrierAnchorConfig AnchorConfig;

	// 배리어 체력 오버라이드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|Health", meta = (ClampMin = "0.0"))
	float BarrierMaxHealth = 0.0f;

	// 배리어 발사 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|Launch", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 1800.0f;

	// 지면 스냅 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|GroundSnap")
	bool bUseGroundSnap = true;

	// 플레이어 배리어 접촉 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|PlayerDamage", meta = (ClampMin = "0.0"))
	float BarrierDamage = 0.0f;

	// 플레이어 배리어 접촉 그로기 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Barrier|PlayerDamage", meta = (ClampMin = "0.0"))
	float BarrierGroggyDamage = 0.0f;
};
