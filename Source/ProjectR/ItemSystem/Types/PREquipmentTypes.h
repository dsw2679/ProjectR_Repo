// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Equipment 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PREquipmentTypes.generated.h"

// 장비 슬롯 종류
UENUM(BlueprintType)
enum class EPREquipmentSlotType : uint8
{
	None,
	Head,		// 머리 방어구
	Body,		// 몸통 방어구
	Hands,		// 손 방어구
	Legs,		// 다리 방어구
	Amulet,		// 목걸이
	Ring1,		// 반지 슬롯 1
	Ring2		// 반지 슬롯 2
};

class UPREquipmentDataAsset;

// 장비 시각적 공개 정보
USTRUCT(BlueprintType)
struct FPRReplicatedEquipmentInfo
{
	GENERATED_BODY()

	// 장착 슬롯
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	// 장비 정적 데이터
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TSoftObjectPtr<UPREquipmentDataAsset> EquipmentData;
};
