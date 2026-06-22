// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Input 설정 데이터 에셋 설정 클래스 정의)
#include "PRInputConfigDataAsset.h"

const UInputAction* UPRInputConfigDataAsset::FindInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FPRInputActionBinding& Binding : AbilityInputBindings)
	{
		if (Binding.InputAction != nullptr && Binding.InputTag == InputTag)
		{
			return Binding.InputAction;
		}
	}
	return nullptr;
}
