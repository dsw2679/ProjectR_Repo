// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 획득 가능 Weapon 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/World/PRInteractableActor.h"
#include "PRPickableWeapon.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UPRWeaponDataAsset;

/**
 * 개발용 월드에 배치하는 무기 픽업 액터.
 * 상호작용 시 UPROPERTY로 지정한 WeaponData를 인벤토리에 추가하고 활성 슬롯에 장착.
 */
UCLASS()
class PROJECTR_API APRPickableWeapon : public APRInteractableActor
{
	GENERATED_BODY()

public:
	APRPickableWeapon();

#if WITH_EDITOR
	/*~ UObject Interface ~*/
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// WeaponData의 WeaponActorClass CDO에서 메시를 가져와 표시 메시에 반영
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon")
	void RefreshVisual();

	// 픽업이 부여할 무기 데이터 반환
	UFUNCTION(BlueprintPure, Category = "Weapon")
	UPRWeaponDataAsset* GetWeaponData() const { return WeaponData; }

	// 장착 성공 시 액터 제거 여부 반환
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool ShouldDestroyOnPickup() const { return bDestroyOnPickup; }

protected:
	// 상호작용 감지용 구체 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionCollision;

	// 픽업의 외형 메시. WeaponActor CDO의 스켈레탈 메시를 반영
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// 상호작용 시 부여할 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UPRWeaponDataAsset> WeaponData = nullptr;

	// 장착 성공 후 액터 자동 제거 여부. 개발 중 반복 픽업이 필요하면 false
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bDestroyOnPickup = false;
};
