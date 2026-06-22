// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 레지스트리 구현)
#include "PRImpactRegistry.h"

bool UPRImpactRegistry::FindImpactDefinition(FGameplayTag ImpactTag, FPRImpactDefinition& OutDefinition) const
{
	// 유효하지 않은 GameplayTag로 Registry를 조회하는 호출 방지
	if (!ImpactTag.IsValid())
	{
		return false;
	}

	if (const FPRImpactDefinition* FoundDefinition = ImpactDefinitions.Find(ImpactTag))
	{
		// 호출자가 Registry 내부 저장소를 직접 수정하지 못하도록 값 복사 반환
		OutDefinition = *FoundDefinition;
		return true;
	}

	return false;
}

bool UPRImpactRegistry::FindImpactTagBySurface(EPhysicalSurface SurfaceType, FGameplayTag& OutImpactTag) const
{
	// 같은 SurfaceType이 여러 번 등록된 경우 에셋 배열에서 먼저 나온 매핑 우선 사용
	for (const FPRImpactSurfaceType& SurfaceMapping : SurfaceTypes)
	{
		if (SurfaceMapping.SurfaceType == SurfaceType && SurfaceMapping.ImpactTag.IsValid())
		{
			OutImpactTag = SurfaceMapping.ImpactTag;
			return true;
		}
	}

	return false;
}
