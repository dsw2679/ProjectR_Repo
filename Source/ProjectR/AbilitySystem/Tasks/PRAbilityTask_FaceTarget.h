// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (어빌리티 태스크 Face 타겟 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "PRAbilityTask_FaceTarget.generated.h"

class AActor;
class UGameplayAbility;

// 어빌리티 AvatarActor를 지정 대상 방향으로 회전시키고 접근시키는 AbilityTask
UCLASS()
class PROJECTR_API UPRAbilityTask_FaceTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPRAbilityTask_FaceTarget(const FObjectInitializer& ObjectInitializer);

	// 대상 방향 회전 및 접근 AbilityTask 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability|Tasks", meta = (DisplayName = "Face And Approach Target", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UPRAbilityTask_FaceTarget* FaceTarget(UGameplayAbility* OwningAbility, AActor* TargetActor, float BlendTime = 1.0f, float TaskDuration = 0.6f, float AcceptanceDistance = 120.0f, float MovementScale = 1.0f, float ObstacleBackoffDistance = 30.0f, float ApproachFanAngle = 90.0f, int32 TraceCount = 12);

	/*~ UGameplayTask Interface ~*/
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	/*~ UGameplayTask Interface ~*/
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// 현재 대상 방향 회전값 계산
	bool ResolveDesiredRotation(FRotator& OutRotation) const;

	// 플레이어 기준 대상 방향 접근 위치 계산
	bool ResolveApproachLocation(FVector& OutLocation) const;

	// 접근 위치의 플레이어 점유 여부 확인
	bool IsApproachLocationOccupied(const FVector& TestLocation) const;

	// 현재 프레임 회전 적용
	void ApplyFaceRotation(float DeltaTime);

	// 현재 프레임 접근 이동 입력 적용
	void ApplyApproachMovement();

private:
	// 방향을 바라볼 대상 액터
	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	// 회전과 이동 입력을 적용할 AvatarActor
	TWeakObjectPtr<AActor> CachedAvatarActor;

	// 대상 방향 회전 보간 시간
	float BlendTime = 1.0f;

	// Task 자동 종료 시간
	float TaskDuration = 0.6f;

	// 접근 완료로 판단할 거리
	float AcceptanceDistance = 120.0f;

	// 접근 이동 입력 크기
	float MovementScale = 1.0f;

	// 장애물 충돌 지점에서 뒤로 물러날 거리
	float ObstacleBackoffDistance = 30.0f;

	// 대상 방향 기준 접근 후보 부채꼴 각도
	float ApproachFanAngle = 90.0f;

	// 플레이어 기준 접근 후보 trace 개수
	int32 TraceCount = 12;

	// 회전 보간 경과 시간
	float ElapsedTime = 0.0f;

	// 회전 보간 시작 회전값
	FRotator StartRotation = FRotator::ZeroRotator;

	// 선택된 접근 위치
	FVector ApproachLocation = FVector::ZeroVector;

	// 접근 위치 선택 성공 여부
	bool bHasApproachLocation = false;
};
