// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 레지스트리 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRImpactTypes.h"
#include "PRImpactRegistry.generated.h"

// Impact 태그별 재생 데이터와 물리 표면 fallback 매핑을 에셋으로 보관하는 Registry
UCLASS(BlueprintType)
class PROJECTR_API UPRImpactRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	// Impact 태그에 해당하는 Niagara와 Decal 재생 데이터 조회
	bool FindImpactDefinition(FGameplayTag ImpactTag, FPRImpactDefinition& OutDefinition) const;

	// 인터페이스 결과가 없을 때 사용할 물리 표면 타입 기반 Impact 태그 조회
	bool FindImpactTagBySurface(EPhysicalSurface SurfaceType, FGameplayTag& OutImpactTag) const;

public:
	// 인터페이스 결과 또는 fallback 태그로 조회하는 Impact 재생 데이터 모음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact")
	TMap<FGameplayTag, FPRImpactDefinition> ImpactDefinitions;

	// Physical Surface fallback 결과를 프로젝트 Impact 태그로 바꾸기 위한 매핑 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact")
	TArray<FPRImpactSurfaceType> SurfaceTypes;

	// 인터페이스와 물리 표면 매핑이 모두 실패했을 때 사용할 기본 Impact 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact")
	FGameplayTag DefaultImpactTag;

	// Impact Pool Actor가 월드에서 처음 만들어질 때 미리 생성해 둘 Niagara Component 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact|Pool", meta = (ClampMin = "0"))
	int32 InitialImpactNiagaraPoolSize = 24;

	// Impact Pool Actor가 월드에서 처음 만들어질 때 미리 생성해 둘 Decal Component 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact|Pool", meta = (ClampMin = "0"))
	int32 InitialImpactDecalPoolSize = 12;

	// Niagara 풀이 가질 수 있는 최대 Component 수. 0 또는 -1이면 빈 슬롯이 없을 때 계속 확장
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact|Pool", meta = (ClampMin = "-1"))
	int32 MaxImpactNiagaraPoolSize = 24;

	// Decal 풀이 가질 수 있는 최대 Component 수. 0 또는 -1이면 빈 슬롯이 없을 때 계속 확장
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Impact|Pool", meta = (ClampMin = "-1"))
	int32 MaxImpactDecalPoolSize = 12;
};
