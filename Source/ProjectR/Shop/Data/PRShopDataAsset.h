// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 상품 목록 및 가격 데이터 정의)
// Author: 배유찬 (무기/장비 상점 데이터 연동 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "PRShopDataAsset.generated.h"

// 상점 판매 목록과 매입 조건을 담는 데이터 에셋
UCLASS(BlueprintType)
class PROJECTR_API UPRShopDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 상점 표시 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	FText DisplayName;

	// 플레이어 판매가 계산에 사용할 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerSellPriceRate = 0.4f;

	// 상점 판매 및 매입 Entry 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	TArray<FPRShopEntry> Entries;
};
