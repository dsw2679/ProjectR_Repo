// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Recoil 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRRecoilTypes.generated.h"

class UCurveVector;
class UCameraShakeBase;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRecoilProfile
{
	GENERATED_BODY()

public:
	// 연속 발사 횟수 기준 반동값을 반환하는 커브다. X는 Pitch, Y는 Yaw, Z는 예비값으로 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	TObjectPtr<UCurveVector> RecoilCurve = nullptr;

	// 커브가 없을 때 사용하는 기본 수직 반동값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	float FallbackPitch = 0.3f;

	// 커브가 없을 때 사용하는 최소 수평 반동값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	float FallbackYawMin = -0.1f;

	// 커브가 없을 때 사용하는 최대 수평 반동값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	float FallbackYawMax = 0.1f;

	// 카메라가 목표 반동값으로 이동하는 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float RecoilSpeed = 20.0f;

	// 사격 중단 후 카메라 반동이 복구되는 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float RecoilRecoverySpeed = 12.0f;

	// 마지막 발사 후 복구를 시작하기까지 대기하는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float RecoveryDelay = 0.15f;

	// ADS 상태일 때 적용할 반동 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float ADSRecoilMultiplier = 0.75f;

	// 사격 1회당 크로스헤어 확산 증가량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float CrosshairSpreadIncrease = 1.0f;

	// 크로스헤어 확산 최대값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float MaxCrosshairSpread = 10.0f;

	// 초당 크로스헤어 확산 회복량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))
	float CrosshairRecoverySpeed = 5.0f;
	
	// 카메라에 실제 적용할 반동 배율                                                                      
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))  
	float CameraRecoilScale = 0.25f;                                                                            
                                                                                                            
	// 카메라 Pitch 반동 최대 누적값                                                                  
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))  
	float MaxRecoilPitch = 3.0f;                                                                                
                                                                                                            
	// 카메라 Yaw 반동 최대 누적값                                                               
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil", meta = (ClampMin = "0.0"))  
	float MaxRecoilYaw = 1.5f;
	
	//-------------------Camera Shake---------------------------------
	// 사격 반동과 함께 재생할 카메라 셰이크 클래스다                                                               
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil|Shake")                           
	TSubclassOf<UCameraShakeBase> CameraShakeClass;                                                                 
                                                                                                                
	// 카메라 셰이크 재생 강도다                                                                                    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil|Shake", meta = (ClampMin = "0.0"))
	float CameraShakeScale = 1.0f;                                                                                  
                                                                                                                
	// ADS 상태에서 카메라 셰이크에 곱할 배율이다                                                                   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil|Shake", meta = (ClampMin = "0.0"))
	float ADSCameraShakeMultiplier = 0.75f;                                                                         
};
