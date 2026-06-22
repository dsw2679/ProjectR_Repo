// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (소모품 사용 시 인벤토리 수량 차감 및 효과 연동 구현)
// Author: 이건주 (퀵슬롯 연동 및 실시간 HUD 개수 갱신 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRItemInstance.h"
#include "PRItemInstance_Consumable.generated.h"

class UPRConsumableDataAsset;

// 인벤토리가 소유하는 소비 아이템 1종의 보유 개수와 사용 처리를 담당
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Consumable : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UPRItemInstance Interface ~*/
	virtual bool ActivateItem(const FPRItemActivationContext& ActivationContext) override;

public:
	// 소비 아이템을 사용
	virtual bool UseItem(AActor* UserActor);

	// 현재 연결된 소비 아이템 데이터를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRConsumableDataAsset* GetConsumableData() const;

protected:
	// 사용 가능한 상태인지 확인
	virtual bool CanUseItem(AActor* UserActor) const;
};
