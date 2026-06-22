// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Map Preload 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRMapPreloadDataAsset.generated.h"

class APawn;
class UNiagaraSystem;
class UPRShopDataAsset;
class USoundBase;
class UUserWidget;

UCLASS(BlueprintType)
class PROJECTR_API UPREnemyPreloadSetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 맵에서 출현 가능한 몬스터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Enemy")
	TArray<TSoftClassPtr<APawn>> ExpectedEnemyClasses;

	// 몬스터가 사용하는 추가 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Enemy")
	TArray<TSoftObjectPtr<UObject>> EnemyDataAssets;

	// 몬스터 연출에 필요한 FX 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	FGameplayTagContainer FXTags;

	// 렌더 프리웜 대상 Niagara
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	TArray<TSoftObjectPtr<UNiagaraSystem>> RenderPrewarmNiagaraSystems;

	// 첫 재생 지연을 줄일 SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Audio")
	TArray<TSoftObjectPtr<USoundBase>> SFX;
};

UCLASS(BlueprintType)
class PROJECTR_API UPRMapPreloadDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 로딩 중 상점 UI 생성과 렌더 프리웜 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|UI")
	bool bPrewarmShopUI = true;

	// 런타임 수집 외에 강제로 포함할 상점 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Shop")
	TArray<TObjectPtr<UPRShopDataAsset>> ExplicitShopDataAssets;

	// 레벨 진입 전 미리 로드할 추가 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|UI")
	TArray<TSoftClassPtr<UUserWidget>> ExtraPreloadWidgetClasses;

	// 레벨 공개 전까지 반드시 완료되어야 하는 추가 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload")
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;

	// 맵별 몬스터와 전투 연출 프리로드 묶음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Enemy")
	TArray<TObjectPtr<UPREnemyPreloadSetDataAsset>> EnemyPreloadSets;

	// 맵 공개 전 미리 준비할 FX 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	FGameplayTagContainer PreloadFXTags;

	// 태그 수집 외에 강제로 포함할 Niagara 시스템
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|FX")
	TArray<TSoftObjectPtr<UNiagaraSystem>> ExtraNiagaraSystems;

	// 태그 수집 외에 강제로 포함할 SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preload|Audio")
	TArray<TSoftObjectPtr<USoundBase>> ExtraSFX;
};
