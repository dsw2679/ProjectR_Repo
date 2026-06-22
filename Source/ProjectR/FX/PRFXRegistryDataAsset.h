// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (이펙트 레지스트리 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRFXTypes.h"
#include "PRFXRegistryDataAsset.generated.h"

// GameplayTag와 FX Cue 실행 정책을 연결하는 Registry 에셋
UCLASS(BlueprintType)
class PROJECTR_API UPRFXRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 태그에 해당하는 Registry Entry 조회
	bool FindEntry(FGameplayTag FXTag, FPRFXRegistryEntry& OutEntry) const;

public:
	// FX 태그와 Cue 클래스 및 최소 정책 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|FX")
	TMap<FGameplayTag, FPRFXRegistryEntry> Entries;
};
