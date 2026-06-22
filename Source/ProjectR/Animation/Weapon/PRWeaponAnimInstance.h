// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (사격 시 총기 슬라이드 작동 및 발사 애니메이션 구현)
// Author: 배유찬 (재장전 탄창 애니메이션 및 장전 시퀀스 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ProjectR/ItemSystem/Types/PRWeaponAnimationTypes.h"
#include "PRWeaponAnimInstance.generated.h"

class APRCharacterBase;
class UPRAnimInstance;
class APRWeaponActor;
class USkeletalMeshComponent;

/**
 * 무기 스켈레탈 메시 ABP가 공통으로 읽는 애니메이션 상태를 관리하는 AnimInstance
 */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/*~ UAnimInstance Interface ~*/
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	// 무기 애니메이션 상태를 Idle로 되돌림
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void SetToIdle();

	// 발사 몽타주를 재생하고 Shoot 상태로 전환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void PlayShoot();

	// 재장전 몽타주를 재생하고 Reload 상태로 전환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void PlayReload();
	
	// 현재 무기 애니메이션 상태가 지정 상태와 같은지 확인
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Animation")
	bool CheckAnimationState(EPRWeaponAnimationState InState) const;

protected:
	// 무기 몽타주가 재생 중인지 확인
	virtual bool IsWeaponMontagePlaying() const;
	
public:
	// 재장전 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;
	
	// 발사 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ShootMontage;
	
	// 이 AnimInstance가 붙어 있는 무기 Actor
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<APRWeaponActor> WeaponActor;
	
	// 현재 무기 애니메이션 상태
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	EPRWeaponAnimationState AnimationState = EPRWeaponAnimationState::Idle;
};
