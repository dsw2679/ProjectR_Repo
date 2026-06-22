// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Interact 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Interact.generated.h"

class UAbilityTask_WaitInputRelease;
class UPRAbilityTask_FaceTarget;
class UPRInteractorComponent;
UCLASS()
class PROJECTR_API UPRGA_Interact : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Interact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
protected:
	UFUNCTION(BlueprintCallable)
	UPRInteractorComponent* GetInteractorComponent(AController* InController);

	// 유지형 상호작용 종료 대기 바인딩
	void BindToSustained(UPRInteractorComponent* Interactor);

	// 서버 유지형 상호작용 시작 대기 바인딩
	void BindToSustainedStart(UPRInteractorComponent* Interactor);

	// Hold 완료 이후 유지형 상호작용 전환 대기 바인딩
	void BindToHoldFinished(UPRInteractorComponent* Interactor);

	// Hold 입력 해제 대기 Task 시작
	void WaitHoldRelease(UPRInteractorComponent* Interactor);

	// 유지형 상호작용 대상 방향 회전 및 접근 Task 시작
	void StartFaceTargetTask(UPRInteractorComponent* Interactor);

	// 유지형 상호작용 대상 방향 회전 및 접근 Task 종료
	void StopFaceTargetTask();

private:
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

private:
	TWeakObjectPtr<UPRInteractorComponent> CachedInteractorComponent;

	// 유지형 상호작용 대상 방향 회전 보간 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0"))
	float FaceTargetBlendTime = 0.3f;

	// 유지형 상호작용 대상 방향 회전 및 접근 Task 자동 종료 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0"))
	float FaceTargetTaskDuration = 0.6f;

	// 유지형 상호작용 접근 완료 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0"))
	float FaceTargetAcceptanceDistance = 20.0f;

	// 유지형 상호작용 접근 이동 입력 크기
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0"))
	float FaceTargetMovementScale = 1.0f;

	// 유지형 상호작용 장애물 충돌 지점 후퇴 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0"))
	float FaceTargetObstacleBackoffDistance = 30.0f;

	// 유지형 상호작용 대상 방향 기준 접근 후보 부채꼴 각도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float FaceTargetApproachFanAngle = 90.0f;

	// 유지형 상호작용 플레이어 기준 접근 후보 trace 개수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Interaction", meta = (ClampMin = "0"))
	int32 FaceTargetTraceCount = 12;

	UPROPERTY()
	UAbilityTask_WaitInputRelease* WaitHoldReleaseTask = nullptr;

	// 유지형 상호작용 대상 방향 회전 및 접근 Task
	UPROPERTY()
	TObjectPtr<UPRAbilityTask_FaceTarget> ActiveFaceTargetTask;
};
