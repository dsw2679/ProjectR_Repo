// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 Weapon Niagara 이펙트 윈도우/트리거 노티파이 구현)
#include "PRAnimNotify_WeaponNiagaraEffect.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"

FString UPRAnimNotify_WeaponNiagaraEffect::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("PR Weapon FX: %s"), *EffectName.ToString());
}

void UPRAnimNotify_WeaponNiagaraEffect::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp) || !IsValid(NiagaraSystem))
	{
		return;
	}

	const FVector SpawnScale = Scale.ComponentMax(FVector::ZeroVector);
	if (SpawnMode == EPRWeaponNiagaraSpawnMode::AttachedToSocket)
	{
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			MeshComp,
			SocketName,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true);

		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->SetRelativeScale3D(SpawnScale);
		}

		return;
	}

	const FTransform SocketTransform = SocketName.IsNone()
		? MeshComp->GetComponentTransform()
		: MeshComp->GetSocketTransform(SocketName, RTS_World);
	const FTransform OffsetTransform(RotationOffset, LocationOffset, SpawnScale);
	const FTransform SpawnTransform = OffsetTransform * SocketTransform;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MeshComp,
		NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		SpawnTransform.GetScale3D(),
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);
}
