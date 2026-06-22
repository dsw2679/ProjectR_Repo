// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (동료 소생 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_Revive.generated.h"

class UPRConsumableDataAsset;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteraction_Revive : public UPRInteractionAction
{
	GENERATED_BODY()
	
public:
	virtual void OnHoldStart_Implementation(AActor* Interactor) override;
	virtual void OnHoldCanceled_Implementation(AActor* Interactor) override;
	virtual void OnHoldFinished_Implementation(AActor* Interactor) override;
	virtual void Execute_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	
private:
	void FinishReviveAbility(AActor* Interactor, bool bCanceled);
	
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UPRConsumableDataAsset> CostItem;

	// 소생 시 Owner(다운된 캐릭터)에게 적용할 체력 회복 GE 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> HealEffectClass;
};
