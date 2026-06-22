// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Item 인스턴스 Material 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRItemInstance.h"
#include "PRItemInstance_Material.generated.h"

class UPRMaterialDataAsset;

// 인벤토리가 소유하는 재료 아이템의 스택 인스턴스다
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Material : public UPRItemInstance
{
	GENERATED_BODY()

public:
	// 현재 연결된 재료 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRMaterialDataAsset* GetMaterialData() const;
};
