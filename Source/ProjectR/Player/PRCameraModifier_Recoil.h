// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Camera Modifier Recoil 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Camera/CameraModifier.h"
#include "StructUtils/InstancedStruct.h"
#include "PRCameraModifier_Recoil.generated.h"

struct FPRRecoilEventPayload;

/**
 * 사격 시 무기 데이터의 반동 패턴(Curve)을 읽어와
 * 카메라의 시야(Rotation)에 가산하는 카메라 모디파이어
 */
UCLASS()
class PROJECTR_API UPRCameraModifier_Recoil : public UCameraModifier
{
	GENERATED_BODY()
	
public:
	UPRCameraModifier_Recoil();

	/*~ UCameraModifier Interface ~*/
	// 카메라에 추가될 때 로컬 플레이어의 반동 이벤트를 구독한다
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

	// 매 프레임 누적 반동을 카메라 회전에 적용한다
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;

	/*~ UObject Interface ~*/
	// 객체 해제 시 이벤트 구독을 정리한다
	virtual void BeginDestroy() override;

protected:
	// 반동 이벤트를 수신한다
	void HandleRecoilEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 등록된 반동 이벤트 구독을 정리한다
	void UnbindRecoilEvent();

	// 반동 프로파일에 설정된 카메라 셰이크를 재생한다                  
	void PlayRecoilCameraShake(const FPRRecoilEventPayload& RecoilData);
private:
	// 반동 목표 회전값
	FRotator TargetRecoilRot;

	// 현재 카메라에 적용 중인 회전값
	FRotator CurrentRecoilRot;

	// 반동 적용 속도
	float CurrentRecoilSpeed = 20.0f;

	// 반동 회복 속도
	float CurrentRecoverySpeed = 12.0f;

	// 회복 시작 지연 시간
	float RecoveryDelay = 0.15f;

	// 마지막 반동 수신 시각
	float LastRecoilTime = 0.0f;

	// 이벤트 시스템 리스너 핸들
	FDelegateHandle RecoilEventHandle;
	
};
