// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (사격 시 총기 슬라이드 작동 및 발사 애니메이션 구현)
// Author: 배유찬 (재장전 탄창 애니메이션 및 장전 시퀀스 구현)
#include "PRWeaponAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"

void UPRWeaponAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	if (APRWeaponActor* OwnerActor =Cast<APRWeaponActor>(OwnerComponent->GetOwner()))
	{
		WeaponActor = OwnerActor;
	}
}

void UPRWeaponAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 재생중인 몽타주가 없는데 State가 Idle이 아닌 경우
	if (!CheckAnimationState(EPRWeaponAnimationState::Idle) && !IsWeaponMontagePlaying())
	{
		SetToIdle();
	}
}


void UPRWeaponAnimInstance::SetToIdle()
{
	AnimationState = EPRWeaponAnimationState::Idle;
	
	Montage_Stop(0.0f);
	
	if (IsValid(WeaponActor))
	{
		WeaponActor->SetIsIKSuppressed(false);
	}
}

void UPRWeaponAnimInstance::PlayShoot()
{
	AnimationState = EPRWeaponAnimationState::Shoot;
	
	if (IsValid(ShootMontage))
	{
		Montage_Play(ShootMontage);
		
		if (IsValid(WeaponActor))
		{
			WeaponActor->SetIsIKSuppressed(false);
		}
	}
}

void UPRWeaponAnimInstance::PlayReload()
{
	AnimationState = EPRWeaponAnimationState::Reload;
	
	if (IsValid(ReloadMontage))
	{
		Montage_Play(ReloadMontage);
		
		if (IsValid(WeaponActor))
		{
			WeaponActor->SetIsIKSuppressed(true);
		}
	}
}

bool UPRWeaponAnimInstance::CheckAnimationState(EPRWeaponAnimationState InState) const
{
	return AnimationState == InState;
}

bool UPRWeaponAnimInstance::IsWeaponMontagePlaying() const
{
	const bool IsReloadPlaying = IsValid(ReloadMontage) &&  Montage_IsPlaying(ReloadMontage);
	const bool IsShootPlaying = IsValid(ShootMontage) && Montage_IsPlaying(ShootMontage);
	
	return IsReloadPlaying || IsShootPlaying;
}