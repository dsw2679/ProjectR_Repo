// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State 검격 Trail Niagara 윈도우/트리거 노티파이 구현)
#include "PRAnimNotifyState_SwordTrailNiagara.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ObjectKey.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRSwordTrailNiagaraNotify, Log, All);

UPRAnimNotifyState_SwordTrailNiagara::UPRAnimNotifyState_SwordTrailNiagara()
{
}

FString UPRAnimNotifyState_SwordTrailNiagara::GetNotifyName_Implementation() const
{
	return TEXT("PR Sword Trail Niagara");
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!ShouldRunForMesh(MeshComp) || !IsValid(TrailSystem))
	{
		return;
	}

	// 같은 메시에서 같은 Notify State가 재진입하면 이전 컴포넌트를 먼저 정리한다.
	EndTrailForMesh(MeshComp);

	TArray<FPRSwordTrailSocketPair> SocketPairs;
	if (bUsePrimarySocketPair)
	{
		FPRSwordTrailSocketPair PrimarySocketPair;
		PrimarySocketPair.bEnabled = true;
		PrimarySocketPair.BladeBottomSocketName = BladeBottomSocketName;
		PrimarySocketPair.BladeTopSocketName = BladeTopSocketName;
		SocketPairs.Add(PrimarySocketPair);
	}

	for (const FPRSwordTrailSocketPair& AdditionalSocketPair : AdditionalSocketPairs)
	{
		if (AdditionalSocketPair.bEnabled)
		{
			SocketPairs.Add(AdditionalSocketPair);
		}
	}

	if (SocketPairs.Num() <= 0)
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail notify has no enabled socket pairs on mesh '%s'."),
			*GetNameSafe(MeshComp));
		return;
	}

	FActiveTrailData NewTrailData;

	for (const FPRSwordTrailSocketPair& SocketPair : SocketPairs)
	{
		FVector TrailLocation = FVector::ZeroVector;
		FRotator TrailRotation = FRotator::ZeroRotator;
		float TrailSize = 0.0f;
		if (!BuildTrailTransform(MeshComp,
			SocketPair.BladeBottomSocketName,
			SocketPair.BladeTopSocketName,
			TrailLocation,
			TrailRotation,
			TrailSize))
		{
			continue;
		}

		UNiagaraComponent* TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			MeshComp,
			NAME_None,
			TrailLocation,
			TrailRotation,
			EAttachLocation::KeepWorldPosition,
			true,
			false,
			ENCPoolMethod::None,
			false);

		if (!IsValid(TrailComponent))
		{
			UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
				TEXT("Failed to spawn sword trail Niagara system on mesh '%s' for sockets '%s' -> '%s'."),
				*GetNameSafe(MeshComp),
				*SocketPair.BladeBottomSocketName.ToString(),
				*SocketPair.BladeTopSocketName.ToString());
			continue;
		}

		FActiveTrailInstance NewInstance;
		NewInstance.Component = TrailComponent;
		NewInstance.BladeBottomSocketName = SocketPair.BladeBottomSocketName;
		NewInstance.BladeTopSocketName = SocketPair.BladeTopSocketName;
		NewTrailData.Instances.Add(NewInstance);

		UpdateTrailComponent(MeshComp, NewTrailData.Instances.Last());
		TrailComponent->Activate(true);
	}

	if (NewTrailData.Instances.Num() > 0)
	{
		ActiveTrails.Add(FObjectKey(MeshComp), MoveTemp(NewTrailData));
	}
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	if (!bUpdateEveryTick || !IsValid(MeshComp))
	{
		return;
	}

	FActiveTrailData* TrailData = ActiveTrails.Find(FObjectKey(MeshComp));
	if (!TrailData)
	{
		return;
	}

	for (int32 InstanceIndex = TrailData->Instances.Num() - 1; InstanceIndex >= 0; --InstanceIndex)
	{
		FActiveTrailInstance& TrailInstance = TrailData->Instances[InstanceIndex];
		if (!TrailInstance.Component.IsValid())
		{
			TrailData->Instances.RemoveAtSwap(InstanceIndex);
			continue;
		}

		UpdateTrailComponent(MeshComp, TrailInstance);
	}

	if (TrailData->Instances.Num() <= 0)
	{
		ActiveTrails.Remove(FObjectKey(MeshComp));
	}
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	EndTrailForMesh(MeshComp);
}

bool UPRAnimNotifyState_SwordTrailNiagara::ShouldRunForMesh(const USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		return false;
	}

	if (bSkipDedicatedServer && World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	if (!bAllowEditorPreview &&
		(World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
	{
		return false;
	}

	return true;
}

bool UPRAnimNotifyState_SwordTrailNiagara::BuildTrailTransform(const USkeletalMeshComponent* MeshComp,
	FName InBladeBottomSocketName,
	FName InBladeTopSocketName,
	FVector& OutLocation,
	FRotator& OutRotation,
	float& OutTrailSize) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	if (InBladeBottomSocketName.IsNone() || !MeshComp->DoesSocketExist(InBladeBottomSocketName))
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail bottom socket '%s' does not exist on mesh '%s'."),
			*InBladeBottomSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	if (InBladeTopSocketName.IsNone() || !MeshComp->DoesSocketExist(InBladeTopSocketName))
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail top socket '%s' does not exist on mesh '%s'."),
			*InBladeTopSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	const FVector BottomLocation = MeshComp->GetSocketLocation(InBladeBottomSocketName);
	const FVector TopLocation = MeshComp->GetSocketLocation(InBladeTopSocketName);
	const FVector BladeVector = TopLocation - BottomLocation;
	const float BladeLength = BladeVector.Size();

	if (BladeLength <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail sockets '%s' and '%s' overlap on mesh '%s'."),
			*InBladeBottomSocketName.ToString(),
			*InBladeTopSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	OutLocation = (BottomLocation + TopLocation) * 0.5f;
	OutRotation = FRotationMatrix::MakeFromZ(BladeVector).Rotator();
	OutTrailSize = FMath::Max(0.0f, BladeLength * TrailSizeScale + TrailSizeOffset);
	return true;
}

void UPRAnimNotifyState_SwordTrailNiagara::UpdateTrailComponent(const USkeletalMeshComponent* MeshComp,
	FActiveTrailInstance& TrailInstance) const
{
	UNiagaraComponent* TrailComponent = TrailInstance.Component.Get();
	if (!IsValid(MeshComp) || !IsValid(TrailComponent))
	{
		return;
	}

	FVector TrailLocation = FVector::ZeroVector;
	FRotator TrailRotation = FRotator::ZeroRotator;
	float TrailSize = 0.0f;
	if (!BuildTrailTransform(MeshComp,
		TrailInstance.BladeBottomSocketName,
		TrailInstance.BladeTopSocketName,
		TrailLocation,
		TrailRotation,
		TrailSize))
	{
		return;
	}

	TrailComponent->SetWorldLocationAndRotation(TrailLocation, TrailRotation);

	if (!TrailSizeParameterName.IsNone())
	{
		TrailComponent->SetVariableFloat(TrailSizeParameterName, TrailSize);
	}

	if (bSetUserTrailSize)
	{
		TrailComponent->SetVariableFloat(FName(TEXT("User.TrailSize")), TrailSize);
	}
}

void UPRAnimNotifyState_SwordTrailNiagara::EndTrailForMesh(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	FActiveTrailData TrailData;
	if (!ActiveTrails.RemoveAndCopyValue(FObjectKey(MeshComp), TrailData))
	{
		return;
	}

	for (const FActiveTrailInstance& TrailInstance : TrailData.Instances)
	{
		UNiagaraComponent* TrailComponent = TrailInstance.Component.Get();
		if (!IsValid(TrailComponent))
		{
			continue;
		}

		if (bDestroyImmediatelyOnEnd)
		{
			TrailComponent->DestroyComponent();
			continue;
		}

		if (bDeactivateOnEnd)
		{
			TrailComponent->Deactivate();
		}
	}
}
