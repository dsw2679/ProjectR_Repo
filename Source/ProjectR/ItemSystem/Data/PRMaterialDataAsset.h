// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재료 아이템 상점 가격 및 아이콘 리소스 정의)
// Author: 배유찬 (강화 재료 분류 및 기본 속성 데이터 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRItemDataAsset.h"
#include "PRMaterialDataAsset.generated.h"

// 강화와 제작 비용에 사용하는 재료 아이템 데이터다
UCLASS(BlueprintType)
class PROJECTR_API UPRMaterialDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	// 재료 Item 타입과 기본 스택 수량을 초기화한다
	UPRMaterialDataAsset();
};
