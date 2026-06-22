// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (프로젝타일 경로 프리뷰 파라미터 키 분리)
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/Projectile/PRProjectileTypes.h"
#include "PRFirePreviewTypes.generated.h"

class APRProjectileBase;

// 경로 프리뷰가 참조할 현재 발사 모드
UENUM(BlueprintType)
enum class EPRFirePreviewMode : uint8
{
	BaseFire,
	ModFire,
};

// 무기 슬롯과 발사 모드로 구성된 경로 프리뷰 선택 키
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFirePreviewKey
{
	GENERATED_BODY()

	// 무기 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	EPRWeaponSlotType WeaponSlot = EPRWeaponSlotType::None;

	// 발사 모드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	EPRFirePreviewMode FireMode = EPRFirePreviewMode::BaseFire;

	// 키 유효성
	bool IsValid() const
	{
		return WeaponSlot == EPRWeaponSlotType::Primary || WeaponSlot == EPRWeaponSlotType::Secondary;
	}

	bool operator==(const FPRFirePreviewKey& Other) const
	{
		return WeaponSlot == Other.WeaponSlot && FireMode == Other.FireMode;
	}
};

FORCEINLINE uint32 GetTypeHash(const FPRFirePreviewKey& Key)
{
	return HashCombine(::GetTypeHash(static_cast<uint32>(Key.WeaponSlot)), ::GetTypeHash(static_cast<uint32>(Key.FireMode)));
}

// 경로 프리뷰 키에 등록되는 투사체 프리뷰 데이터
USTRUCT()
struct PROJECTR_API FPRFirePreviewEntry
{
	GENERATED_BODY()

	// 예측 경로 파라미터
	FPRProjectilePreviewParams Params;

	// 등록 어빌리티 식별자
	FGameplayAbilitySpecHandle AbilitySpecHandle;

	// 등록 원본 오브젝트
	TWeakObjectPtr<UObject> SourceObject;

	// 투사체 클래스
	TSubclassOf<APRProjectileBase> ProjectileClass;
};
