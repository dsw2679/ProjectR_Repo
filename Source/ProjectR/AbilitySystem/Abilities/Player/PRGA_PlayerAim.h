// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (무기별 줌(Zoom) 기능 및 조준용 카메라/애님레이어 구현)
// Author: 배유찬 (조준 시 HUD 크로스헤어 및 사격 프리뷰 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerAim.generated.h"

UCLASS()
class PROJECTR_API UPRGA_PlayerAim : public UPRGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPRGA_PlayerAim();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	/** 조준 종료 시 로컬 카메라와 조준 피드백을 기본 상태로 되돌린다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	/** 조준 시 변경할 시야각(FOV) */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	float AimFOV = 50.0f;

	/** 조준 시 변경할 카메라 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	float AimTargetArmLength = 100.0f;

	/** 조준 시 변경할 회전축(TargetOffset) 높이 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	FVector AimTargetOffset = FVector(0.0f, 0.0f, 75.0f);

	/** 조준 시 변경할 어깨 오프셋(SocketOffset) */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	FVector AimSocketOffset = FVector(0.0f, 40.0f, 0.0f);

private:
	void ApplyAimCameraMode();
};
