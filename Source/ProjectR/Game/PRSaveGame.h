// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Save Game 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRSaveGame.generated.h"

// ProjectR 로컬 캐릭터 저장 파일 형식
UCLASS(BlueprintType)
class PROJECTR_API UPRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 저장 파일에 기록된 캐릭터 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Save")
	FPRCharacterSaveData CharacterSaveData;

	// 저장 파일에 기록된 월드 진행 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Save")
	FPRWorldSaveData WorldSaveData;
};
