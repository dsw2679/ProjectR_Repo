// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (UI 사운드 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRUISoundDataAsset.generated.h"

class USoundBase;

// UI 버튼 기본 상호작용 사운드 설정
UCLASS(BlueprintType)
class PROJECTR_API UPRUISoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 버튼에 마우스를 올렸을 때 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|UI|Sound")
	TObjectPtr<USoundBase> HoveredSound;

	// 버튼을 클릭했을 때 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|UI|Sound")
	TObjectPtr<USoundBase> ClickedSound;

	// 같은 UI 상호작용 사운드의 최소 재생 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|UI|Sound", meta = (ClampMin = "0.0", Units = "s"))
	float PlaybackCooldown = 0.0f;
};
