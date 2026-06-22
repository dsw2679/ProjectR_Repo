// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (UI Start Menu 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRStartMenuTypes.generated.h"

// 시작 메뉴의 세이브 슬롯 표시 데이터
USTRUCT(BlueprintType)
struct FPRStartMenuSaveSlotViewData
{
	GENERATED_BODY()

public:
	// 세이브 슬롯 번호
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|StartMenu")
	int32 SlotIndex = INDEX_NONE;

	// 세이브 파일 존재 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|StartMenu")
	bool bHasSave = false;

	// 현재 선택된 슬롯 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|StartMenu")
	bool bSelected = false;

	// UI에 표시할 슬롯 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|StartMenu")
	FText DisplayName;

	// UI에 표시할 캐릭터 레벨
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|StartMenu")
	int32 CharacterLevel = 1;
};
