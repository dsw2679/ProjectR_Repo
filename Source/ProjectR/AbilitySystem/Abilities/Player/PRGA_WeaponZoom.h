// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 Weapon Zoom 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "PRGA_WeaponZoom.generated.h"

class UPRAbilitySystemComponent;
class UPRCameraModifier;

UCLASS()
class PROJECTR_API UPRGA_WeaponZoom : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_WeaponZoom();

	/*~ UGameplayAbility Interface ~*/
	/** 줌 어빌리티를 활성화하고 로컬 카메라 표현을 적용 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	/** 토글 입력이 다시 들어오면 줌을 종료 */
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                          const FGameplayAbilityActivationInfo ActivationInfo) override;
	/** 어빌리티가 제거될 때 로컬 카메라 표현을 정리 */
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	/** 줌 어빌리티 종료 시 로컬 카메라 표현을 원복 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	/** 줌 상태에서 사용할 시야각 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|WeaponZoom|Camera")
	float ZoomFOV = 35.0f;

	/** 줌 상태에서 사용할 카메라 암 길이 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|WeaponZoom|Camera")
	float ZoomTargetArmLength = 20.0f;

	/** 줌 상태에서 사용할 카메라 기준 위치 보정 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|WeaponZoom|Camera")
	FVector ZoomTargetOffset = FVector(0.0f, 0.0f, 95.0f);

	/** 줌 상태에서 사용할 카메라 소켓 위치 보정 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|WeaponZoom|Camera")
	FVector ZoomSocketOffset = FVector::ZeroVector;

	/** PRCameraModifier가 적용할 추가 카메라 위치 보정 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|WeaponZoom|Camera")
	FVector ZoomModifierLocationOffset = FVector::ZeroVector;

private:
	/** 태그 변화에 따라 줌 종료 조건을 처리 */
	void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists);

	/** 로컬 플레이어에게 줌 카메라와 메시 은닉 표현을 적용 */
	void ApplyLocalZoom();

	/** 로컬 줌 표현과 태그 감시 바인딩을 정리 */
	void CleanupZoom();

	/** 지정된 취소 여부로 현재 줌 어빌리티를 종료 */
	void EndZoomAbility(bool bWasCancelled, EPRCameraMode InCameraModeAfterZoom);

private:
	FDelegateHandle GameplayTagUpdatedHandle;

	TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC;

	UPROPERTY(Transient)
	TObjectPtr<UPRCameraModifier> ActiveCameraModifier;

	EPRCameraMode CameraModeAfterZoom = EPRCameraMode::Default;
	EPRCameraMode PreviousCameraMode = EPRCameraMode::Default;
	FPRSpringArmCameraModeSettings PreviousCameraModeSettings;

	bool bHasSavedOwnerNoSee = false;
	bool bSavedOwnerNoSee = false;
	bool bHasPreviousCameraModeSettings = false;
	bool bZoomApplied = false;
};
