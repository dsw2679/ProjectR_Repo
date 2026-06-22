// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Runtime Preload 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRRuntimePreloadDataAsset.generated.h"

class UNiagaraSystem;
class USoundBase;

UCLASS(BlueprintType)
class PROJECTR_API UPRRuntimePreloadDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 플레이어 런타임 진입 전 미리 로드할 공통 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload")
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;

	// 플레이어가 어느 맵에서든 사용할 수 있는 공통 FX 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	FGameplayTagContainer PreloadFXTags;

	// 플레이어 공통 PSO 프리웜 대상 Niagara
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	TArray<TSoftObjectPtr<UNiagaraSystem>> RenderPrewarmNiagaraSystems;

	// 렌더 프리웜 Niagara를 화면에 유지할 프레임 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX", meta = (ClampMin = "1"))
	int32 RenderPrewarmFrameCount = 8;

	// 렌더 프리웜 직후 강제로 진행할 Niagara 시뮬레이션 틱 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX", meta = (ClampMin = "0"))
	int32 RenderPrewarmAdvanceTickCount = 4;

	// 강제 Niagara 시뮬레이션 틱 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX", meta = (ClampMin = "0.001", Units = "s"))
	float RenderPrewarmAdvanceTickDelta = 0.016667f;

	// 플레이어 공통 첫 재생 지연 감소 대상 SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Audio")
	TArray<TSoftObjectPtr<USoundBase>> SFX;
};
