// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 Footstep 사운드 윈도우/트리거 노티파이 구현)
#include "PRAnimNotify_FootstepSound.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

/*~ UAnimNotify Interface ~*/

FString UPRAnimNotify_FootstepSound::GetNotifyName_Implementation() const
{
	const TCHAR* FootText = Foot == EPRFootstepFoot::Left ? TEXT("Left") : TEXT("Right");
	const TCHAR* MoveText = TEXT("Jog");
	switch (MoveType)
	{
	case EPRFootstepMoveType::Crouch:
		MoveText = TEXT("Crouch");
		break;

	case EPRFootstepMoveType::Walk:
		MoveText = TEXT("Walk");
		break;

	case EPRFootstepMoveType::Jog:
		MoveText = TEXT("Jog");
		break;

	case EPRFootstepMoveType::Sprint:
		MoveText = TEXT("Sprint");
		break;

	default:
		break;
	}

	return FString::Printf(TEXT("PR Footstep: %s %s"), FootText, MoveText);
}

void UPRAnimNotify_FootstepSound::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp) || !IsValid(FootstepSoundData))
	{
		return;
	}

	FVector TraceOrigin = FVector::ZeroVector;
	if (!ResolveTraceOrigin(MeshComp, TraceOrigin))
	{
		return;
	}

	FHitResult HitResult;
	const bool bHitSurface = TraceFootstepSurface(MeshComp, TraceOrigin, HitResult);
	if (!bHitSurface && !FootstepSoundData->bPlayDefaultWhenTraceMissed)
	{
		return;
	}

	const EPhysicalSurface SurfaceType = bHitSurface
		? UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get())
		: SurfaceType_Default;
	const FPRFootstepSoundEntry* SoundEntry = FootstepSoundData->FindSoundEntry(SurfaceType);
	if (!SoundEntry || !IsValid(SoundEntry->Sound))
	{
		return;
	}

	const FVector SoundLocation = bHitSurface ? HitResult.ImpactPoint : TraceOrigin;
	const FPRFootstepMoveModifier& MoveModifier = FootstepSoundData->ResolveMoveModifier(MoveType);
	PlayFootstepSound(MeshComp, *SoundEntry, MoveModifier, SoundLocation);
	ReportFootstepNoise(MeshComp, *SoundEntry, MoveModifier, SoundLocation);
}

bool UPRAnimNotify_FootstepSound::ResolveTraceOrigin(const USkeletalMeshComponent* MeshComp, FVector& OutTraceOrigin) const
{
	if (!IsValid(MeshComp) || !IsValid(FootstepSoundData))
	{
		return false;
	}

	const FName SocketName = FootstepSoundData->ResolveFootSocketName(Foot);
	if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		OutTraceOrigin = MeshComp->GetSocketLocation(SocketName);
		return true;
	}

	if (!bFallbackToCapsuleBottom)
	{
		return false;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner());
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->GetCapsuleComponent()))
	{
		return false;
	}

	OutTraceOrigin = OwnerCharacter->GetActorLocation()
		- FVector(0.0f, 0.0f, OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	return true;
}

bool UPRAnimNotify_FootstepSound::TraceFootstepSurface(
	const USkeletalMeshComponent* MeshComp,
	const FVector& TraceOrigin,
	FHitResult& OutHit) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const FVector Start = TraceOrigin + FVector(0.0f, 0.0f, FMath::Max(TraceStartUpOffset, 0.0f));
	const FVector End = TraceOrigin - FVector(0.0f, 0.0f, FMath::Max(TraceDistance, 0.0f));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFootstepSoundTrace), bTraceComplex);
	QueryParams.bReturnPhysicalMaterial = true;

	if (AActor* OwnerActor = MeshComp->GetOwner())
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		TraceChannel,
		QueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDebugDraw)
	{
		const FColor TraceColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(World, Start, End, TraceColor, false, 1.0f, 0, 2.0f);
		if (bHit)
		{
			DrawDebugSphere(World, OutHit.ImpactPoint, 8.0f, 12, TraceColor, false, 1.0f);
		}
	}
#endif

	return bHit;
}

void UPRAnimNotify_FootstepSound::PlayFootstepSound(
	USkeletalMeshComponent* MeshComp,
	const FPRFootstepSoundEntry& SoundEntry,
	const FPRFootstepMoveModifier& MoveModifier,
	const FVector& SoundLocation) const
{
	if (!IsValid(MeshComp) || !IsValid(SoundEntry.Sound))
	{
		return;
	}

	const float ClampedVolumeMultiplier = FMath::Max(SoundEntry.VolumeMultiplier * MoveModifier.VolumeMultiplier, 0.0f);
	const float ClampedPitchMultiplier = FMath::Max(SoundEntry.PitchMultiplier * MoveModifier.PitchMultiplier, 0.1f);

	const APawn* OwnerPawn = Cast<APawn>(MeshComp->GetOwner());
	if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
	{
		UGameplayStatics::SpawnSound2D(
			MeshComp,
			SoundEntry.Sound,
			ClampedVolumeMultiplier,
			ClampedPitchMultiplier,
			0.0f,
			MoveModifier.ConcurrencySettings,
			false,
			true);
		return;
	}

	UGameplayStatics::SpawnSoundAtLocation(
		MeshComp,
		SoundEntry.Sound,
		SoundLocation,
		FRotator::ZeroRotator,
		ClampedVolumeMultiplier,
		ClampedPitchMultiplier,
		0.0f,
		MoveModifier.AttenuationSettings,
		MoveModifier.ConcurrencySettings,
		true);
}

void UPRAnimNotify_FootstepSound::ReportFootstepNoise(
	const USkeletalMeshComponent* MeshComp,
	const FPRFootstepSoundEntry& SoundEntry,
	const FPRFootstepMoveModifier& MoveModifier,
	const FVector& NoiseLocation) const
{
	if (!bReportNoiseToAI || !IsValid(MeshComp) || NoiseMaxRange <= 0.0f || NoiseLoudness <= 0.0f)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(OwnerActor);
	if (!IsValid(InstigatorPawn))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float FinalLoudness = FMath::Max(
		NoiseLoudness * SoundEntry.NoiseLoudnessMultiplier * MoveModifier.NoiseLoudnessMultiplier,
		0.0f);
	if (FinalLoudness <= 0.0f)
	{
		return;
	}

	UAISense_Hearing::ReportNoiseEvent(
		World,
		NoiseLocation,
		FinalLoudness,
		InstigatorPawn,
		NoiseMaxRange,
		NoiseTag);
}
