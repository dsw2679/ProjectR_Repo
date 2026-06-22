// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Spring Arm 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "PRSpringArmComponent.generated.h"

UENUM(BlueprintType)
enum class EPRCameraMode : uint8
{
	Default UMETA(DisplayName = "Default"),
	Aim UMETA(DisplayName = "Aim"),
	WeaponZoom UMETA(DisplayName = "WeaponZoom")
};

USTRUCT(BlueprintType)
struct FPRSpringArmCameraModeSettings
{
	GENERATED_BODY()

	FPRSpringArmCameraModeSettings() = default;

	FPRSpringArmCameraModeSettings(float InTargetArmLength, const FVector& InTargetOffset, const FVector& InSocketOffset)
		: TargetArmLength(InTargetArmLength)
		, TargetOffset(InTargetOffset)
		, SocketOffset(InSocketOffset)
	{
	}

	/** 목표 카메라 암 길이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float TargetArmLength = 300.0f;

	/** 목표 카메라 기준 위치 보정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	FVector TargetOffset = FVector(0.0f, 0.0f, 80.0f);

	/** 목표 카메라 소켓 위치 보정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	FVector SocketOffset = FVector(0.0f, 70.0f, 0.0f);
};

/**
 * 카메라 붐의 길이와 오프셋 보간을 전담한다.
 * Default 모드 설정만 직접 소유하고, Aim과 WeaponZoom 설정은 각 어빌리티에서 주입한다.
 */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UPRSpringArmComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 현재 카메라 모드를 변경한다. */
	void SetCameraMode(EPRCameraMode NewCameraMode);

	/** 지정한 카메라 모드의 목표 보간 값을 갱신한다. */
	void SetCameraModeSettings(EPRCameraMode CameraMode, const FPRSpringArmCameraModeSettings& NewSettings);

	/** 현재 카메라 모드를 반환한다. */
	EPRCameraMode GetCameraMode() const { return CurrentCameraMode; }

	/** 현재 카메라 모드의 목표 보간 값을 반환한다. */
	const FPRSpringArmCameraModeSettings& GetCameraModeSettings() const;

	/** 외부 강제 이동 중 사용할 카메라 Lag 설정 갱신 */
	UFUNCTION(BlueprintCallable, Category = "PR|Camera|Lag")
	void SetExternalForcedMoveLagSettings(bool bInUseOverride, bool bInEnableCameraLag, float InCameraLagSpeed, float InCameraLagMaxDistance);

	/** 외부 강제 이동 카메라 Lag override 적용 */
	void ApplyExternalForcedMoveLagOverride();

	/** 외부 강제 이동 카메라 Lag override 복구 */
	void ClearExternalForcedMoveLagOverride();

protected:
	/** 카메라 암 길이 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float ArmLengthInterpSpeed = 5.0f;

	/** 카메라 오프셋 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float SocketOffsetInterpSpeed = 5.0f;

	/** 기본 카메라 모드 목표값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera|Mode")
	FPRSpringArmCameraModeSettings DefaultCameraSettings;

	/** 외부 강제 이동 중 별도 Lag 설정 사용 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera|Lag")
	bool bUseExternalForcedMoveLagOverride = true;

	/** 외부 강제 이동 중 카메라 Lag 활성 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera|Lag", meta = (EditCondition = "bUseExternalForcedMoveLagOverride"))
	bool bExternalForcedMoveEnableCameraLag = true;

	/** 외부 강제 이동 중 카메라 Lag 속도. 값이 클수록 추적 지연 감소 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera|Lag", meta = (ClampMin = "0.0", EditCondition = "bUseExternalForcedMoveLagOverride"))
	float ExternalForcedMoveCameraLagSpeed = 60.0f;

	/** 외부 강제 이동 중 카메라 Lag 최대 거리. 0이면 제한 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera|Lag", meta = (ClampMin = "0.0", EditCondition = "bUseExternalForcedMoveLagOverride"))
	float ExternalForcedMoveCameraLagMaxDistance = 30.0f;

private:
	const FPRSpringArmCameraModeSettings& GetCurrentCameraModeSettings() const;

private:
	EPRCameraMode CurrentCameraMode = EPRCameraMode::Default;
	FPRSpringArmCameraModeSettings CurrentCameraModeSettings;

	bool bExternalForcedMoveLagOverrideActive = false;
	bool bCachedCameraLagEnabled = true;
	float CachedCameraLagSpeed = 0.0f;
	float CachedCameraLagMaxDistance = 0.0f;
};
