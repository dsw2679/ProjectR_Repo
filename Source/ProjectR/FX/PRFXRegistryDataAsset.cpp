// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (이펙트 레지스트리 데이터 에셋 구현)
#include "PRFXRegistryDataAsset.h"

bool UPRFXRegistryDataAsset::FindEntry(FGameplayTag FXTag, FPRFXRegistryEntry& OutEntry) const
{
	if (!FXTag.IsValid())
	{
		return false;
	}

	if (const FPRFXRegistryEntry* FoundEntry = Entries.Find(FXTag))
	{
		// 호출 측의 Registry 내부 저장소 수정을 막기 위한 값 복사 반환
		OutEntry = *FoundEntry;
		return true;
	}

	return false;
}
