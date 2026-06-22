// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (UI FPR Weapon Status View 데이터 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "FPRWeaponStatusViewData.generated.h"

class UTexture2D;

// 무기 상태 위젯 하나에 전달할 표시 전용 데이터다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponStatusViewData
{
	GENERATED_BODY()

public:
	// 주무기와 보조무기 위젯을 같은 클래스로 재사용하기 위한 슬롯 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 현재 활성 슬롯 강조 여부를 결정한다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	bool bIsCurrentWeaponSlot = false;

	// 무기가 없는 슬롯을 빈 상태로 표시하기 위해 사용한다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	bool bHasWeapon = false;

	// 무기 아이콘 표시용 텍스처
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTexture2D> WeaponIcon = nullptr;

	// 탄창 용량 텍스트 원본 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	float MagazineAmmo = 0.0f;

	// 잔탄 용량 텍스트 원본 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	float ReserveAmmo = 0.0f;

	// Mod가 없는 슬롯을 빈 상태로 표시하기 위해 사용한다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	bool bHasMod = false;

	// Mod 아이콘 표시용 텍스처
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTexture2D> ModIcon = nullptr;

	// 원형 게이지 바에 반영할 0.0부터 1.0 사이 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	float ModGaugePercent = 0.0f;

	// Mod 스택 개수 텍스트 원본 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	float ModStackCount = 0.0f;

	// 지속시간형 Mod 표시 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	bool bUsesModDuration = false;

	// 지속시간형 Mod의 남은 시간
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon")
	float ModRemainingDurationSeconds = 0.0f;
};
