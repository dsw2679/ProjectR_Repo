// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotifyState_LoopingMusic.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

/*~ UAnimNotifyState Interface ~*/

FString UPRAnimNotifyState_LoopingMusic::GetNotifyName_Implementation() const
{
	if (IsValid(Music))
	{
		return FString::Printf(TEXT("PR Looping Music: %s"), *Music->GetName());
	}

	return TEXT("PR Looping Music");
}

void UPRAnimNotifyState_LoopingMusic::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp) || !IsValid(Music) || !CanPlayForMesh(MeshComp))
	{
		return;
	}

	StopActiveAudioComponent(MeshComp);

	const float ClampedVolumeMultiplier = FMath::Max(VolumeMultiplier, 0.0f);
	const float ClampedPitchMultiplier = FMath::Max(PitchMultiplier, 0.1f);
	const float ClampedStartTime = FMath::Max(StartTime, 0.0f);

	UAudioComponent* AudioComponent = nullptr;
	if (bOwnerOnly)
	{
		AudioComponent = UGameplayStatics::SpawnSound2D(
			MeshComp,
			Music,
			ClampedVolumeMultiplier,
			ClampedPitchMultiplier,
			ClampedStartTime,
			ConcurrencySettings,
			false,
			false);
	}
	else
	{
		AudioComponent = UGameplayStatics::SpawnSoundAttached(
			Music,
			MeshComp,
			AttachSocketName,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true,
			ClampedVolumeMultiplier,
			ClampedPitchMultiplier,
			ClampedStartTime,
			AttenuationSettings,
			ConcurrencySettings,
			false);
	}

	if (IsValid(AudioComponent))
	{
		ActiveAudioComponents.Add(MeshComp, AudioComponent);
	}
}

void UPRAnimNotifyState_LoopingMusic::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!bRestartWhenFinished)
	{
		return;
	}

	UAudioComponent* AudioComponent = FindActiveAudioComponent(MeshComp);
	if (!IsValid(AudioComponent))
	{
		ActiveAudioComponents.Remove(MeshComp);
		return;
	}

	if (!AudioComponent->IsPlaying())
	{
		AudioComponent->Play(FMath::Max(StartTime, 0.0f));
	}
}

void UPRAnimNotifyState_LoopingMusic::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	StopActiveAudioComponent(MeshComp);
}

bool UPRAnimNotifyState_LoopingMusic::CanPlayForMesh(const USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	if (!bOwnerOnly)
	{
		return true;
	}

	const APawn* OwnerPawn = Cast<APawn>(MeshComp->GetOwner());
	if (IsValid(OwnerPawn))
	{
		return OwnerPawn->IsLocallyControlled();
	}

	return !World->IsGameWorld();
}

UAudioComponent* UPRAnimNotifyState_LoopingMusic::FindActiveAudioComponent(USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return nullptr;
	}

	const TWeakObjectPtr<UAudioComponent>* AudioComponent = ActiveAudioComponents.Find(MeshComp);
	if (AudioComponent == nullptr)
	{
		return nullptr;
	}

	return AudioComponent->Get();
}

void UPRAnimNotifyState_LoopingMusic::StopActiveAudioComponent(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	UAudioComponent* AudioComponent = FindActiveAudioComponent(MeshComp);
	if (IsValid(AudioComponent))
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}

	ActiveAudioComponents.Remove(MeshComp);
}
