// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Camera Modifier Recoil 구현)
#include "ProjectR/Player/PRCameraModifier_Recoil.h"

#include "Camera/PlayerCameraManager.h"
#include "Curves/CurveVector.h"
#include "GameFramework/PlayerController.h"
#include "InstancedStruct.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "Camera/CameraShakeBase.h" 

UPRCameraModifier_Recoil::UPRCameraModifier_Recoil()
{
	TargetRecoilRot = FRotator::ZeroRotator;
	CurrentRecoilRot = FRotator::ZeroRotator;
}

void UPRCameraModifier_Recoil::AddedToCamera(APlayerCameraManager* Camera)
{
	Super::AddedToCamera(Camera);

	if (!IsValid(Camera))
	{
		return;
	}

	APlayerController* OwningController = Camera->GetOwningPlayerController();
	if (!IsValid(OwningController) || !OwningController->IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
	{
		RecoilEventHandle = EventMgr->Listen(
			PRGameplayTags::Event_Player_Recoil,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRCameraModifier_Recoil::HandleRecoilEvent));
	}
}

bool UPRCameraModifier_Recoil::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastRecoilTime > RecoveryDelay)
	{
		TargetRecoilRot = FMath::RInterpTo(TargetRecoilRot, FRotator::ZeroRotator, DeltaTime, CurrentRecoverySpeed);
	}

	CurrentRecoilRot = FMath::RInterpTo(CurrentRecoilRot, TargetRecoilRot, DeltaTime, CurrentRecoilSpeed);
	InOutPOV.Rotation += CurrentRecoilRot;

	return false;
}

void UPRCameraModifier_Recoil::BeginDestroy()
{
	UE_LOG(LogTemp, Warning, TEXT("BeginDestroy()"));
	UnbindRecoilEvent();
	Super::BeginDestroy();
}

void UPRCameraModifier_Recoil::HandleRecoilEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRRecoilEventPayload* RecoilData = Payload.GetPtr<FPRRecoilEventPayload>();
	if (RecoilData == nullptr)
	{
		return;
	}

	const FPRRecoilProfile& Profile = RecoilData->RecoilProfile;
	const float ShotIndex = static_cast<float>(FMath::Max(1, RecoilData->ConsecutiveShots));

	FRotator AddRecoil = FRotator::ZeroRotator;
	if (IsValid(Profile.RecoilCurve))
	{
		const FVector CurveValue = Profile.RecoilCurve->GetVectorValue(ShotIndex);
		AddRecoil.Pitch = CurveValue.X;
		AddRecoil.Yaw = CurveValue.Y;
	}
	else
	{
		AddRecoil.Pitch = Profile.FallbackPitch;
		AddRecoil.Yaw = FMath::RandRange(Profile.FallbackYawMin, Profile.FallbackYawMax);
	}

	if (RecoilData->bIsAiming)                                                                  
	{                                                                                           
		AddRecoil *= Profile.ADSRecoilMultiplier;                                               
	}                                                                                           
                                                                                            
	AddRecoil *= Profile.CameraRecoilScale;                                                     
                                                                                            
	TargetRecoilRot.Pitch = FMath::Clamp(                                                       
		TargetRecoilRot.Pitch + AddRecoil.Pitch,                                                
		-Profile.MaxRecoilPitch,                                                                
		Profile.MaxRecoilPitch);                                                                
                                                                                            
	TargetRecoilRot.Yaw = FMath::Clamp(                                                         
		TargetRecoilRot.Yaw + AddRecoil.Yaw,                                                    
		-Profile.MaxRecoilYaw,                                                                  
		Profile.MaxRecoilYaw);                                                                  
                                                                                            
	CurrentRecoilSpeed = Profile.RecoilSpeed > 0.0f ? Profile.RecoilSpeed : CurrentRecoilSpeed; 
	CurrentRecoverySpeed = Profile.RecoilRecoverySpeed > 0.0f ? Profile.RecoilRecoverySpeed : CurrentRecoverySpeed;
	RecoveryDelay = Profile.RecoveryDelay;

	PlayRecoilCameraShake(*RecoilData);
	
	if (UWorld* World = GetWorld())
	{
		LastRecoilTime = World->GetTimeSeconds();
	}
}

void UPRCameraModifier_Recoil::UnbindRecoilEvent()
{
	if (!RecoilEventHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			EventMgr->Unlisten(PRGameplayTags::Event_Player_Recoil, RecoilEventHandle);
		}
	}

	RecoilEventHandle.Reset();
}

void UPRCameraModifier_Recoil::PlayRecoilCameraShake(const FPRRecoilEventPayload& RecoilData)
{
	const FPRRecoilProfile& Profile = RecoilData.RecoilProfile;
	if (!IsValid(Profile.CameraShakeClass))
	{
		return;
	}

	APlayerCameraManager* CameraManager = CameraOwner;
	if (!IsValid(CameraManager))
	{
		return;
	}

	APlayerController* OwningController = CameraManager->GetOwningPlayerController();
	if (!IsValid(OwningController) || !OwningController->IsLocalController())
	{
		return;
	}

	float ShakeScale = Profile.CameraShakeScale;
	if (RecoilData.bIsAiming)
	{
		ShakeScale *= Profile.ADSCameraShakeMultiplier;
	}

	CameraManager->StartCameraShake(Profile.CameraShakeClass, ShakeScale);
}
