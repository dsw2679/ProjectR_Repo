// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Fae 로열 아처 Character 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRFaeRoyalArcherCharacter.generated.h"

class UPRFaeRoyalArcherCombatDataAsset;
class UBlackboardComponent;

// Fae Royal Archer 전용 캐릭터 클래스다.
// 공중 이동 기본값과 궁수 전용 데이터 접근을 제공한다.
UCLASS()
class PROJECTR_API APRFaeRoyalArcherCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	// Fae Royal Archer 기본 CharacterID와 비행 이동 값을 초기화한다.
	APRFaeRoyalArcherCharacter();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ IPREnemyInterface Interface ~*/
	virtual void InitializeEnemyBlackboard(UBlackboardComponent* BlackboardComponent) const override;

	/*~ APREnemyBaseCharacter Interface ~*/
	virtual bool ShouldUsePerchIdlePose() const override { return bUsePerchIdlePose; }

public:
	// 궁수 전용 전투 데이터 자산을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|RoyalArcher")
	UPRFaeRoyalArcherCombatDataAsset* GetRoyalArcherCombatData() const;

	// 첫 전투 진입 때 Perch 기상 연출을 사용할지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|RoyalArcher")
	bool ShouldWakeFromPerchOnCombatStart() const { return bWakeFromPerchOnCombatStart; }

	// 화살 발사 소켓이 없을 때 사용할 fallback 본 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|RoyalArcher")
	FName GetProjectileFallbackSocketName() const { return ProjectileFallbackSocketName; }

	// WakeFromPerch 시작 직전에 Perch 전용 Gargoyle Idle 포즈 상태를 해제한다.
	void PrepareWakeFromPerch();

	// Death/Groggy 등 전투 불능 연출 진입 시 Perch 전용 대기 포즈를 해제한다.
	void ClearPerchIdlePose();

protected:
	// CharacterMovement에 Royal Archer의 기본 비행 이동 정책을 적용한다.
	void ApplyRoyalArcherMovementDefaults();

	// BeginPlay에서 Perch 전용 대기 포즈를 적용한다.
	void InitializePerchIdlePose();

protected:
	// BeginPlay 시 Flying 이동 모드로 시작할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight")
	bool bStartInFlyingMode = true;

	// 기본 공중 이동 최대 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultMaxFlySpeed = 420.0f;

	// Flying 이동 중 제동 감속 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultBrakingDecelerationFlying = 800.0f;

	// Flying 이동 중 회전 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultFlightRotationYawRate = 540.0f;

	// ProjectileSocket이 아직 없을 때 단발 사격 기준으로 사용할 본 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Projectile")
	FName ProjectileFallbackSocketName = TEXT("Bone_FA_Weapon_Arrow");

	// Perch/Gargoyle 배치 개체만 전투 시작 WakeFromPerch 연출을 사용하게 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Perch")
	bool bWakeFromPerchOnCombatStart = false;

	// Perch/Gargoyle 시작 개체가 Alert 전까지 Gargoyle Idle 루프를 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Perch")
	bool bStartInPerchIdlePose = true;

	// 현재 ABP가 Gargoyle Idle 루프를 우선 사용해야 하는지 나타낸다.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Perch")
	bool bUsePerchIdlePose = false;

	// BT Wake 분기 진입 여부를 기록할 Blackboard Bool 키다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Perch")
	FName ShouldWakeFromPerchKey = TEXT("should_wake_from_perch");

	// Wake 반복 방지 상태를 초기화할 Blackboard Bool 키다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Perch")
	FName HasPlayedWakeFromPerchKey = TEXT("has_played_wake_from_perch");
};
