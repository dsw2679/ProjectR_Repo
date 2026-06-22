// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRAmmoDataAsset.generated.h"

// 드롭 보상으로 지급할 탄약 아이템 데이터다
UCLASS(BlueprintType)
class PROJECTR_API UPRAmmoDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	// 탄약 Item 타입과 기본 스택 수량을 초기화한다
	UPRAmmoDataAsset();

	// 지급할 예비탄 타입을 반환한다
	EPRAmmoType GetAmmoType() const { return AmmoType; }

protected:
	// 지급할 예비탄 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Ammo")
	EPRAmmoType AmmoType = EPRAmmoType::Primary;
};
