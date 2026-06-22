// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Attribute Set 성장 구현)
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAttributeSet_Growth.generated.h"

// 플레이어 성장 상태를 보유하는 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Growth : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

public:
	// 누적 경험치
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Experience, Category = "ProjectR|Attributes|Growth")
	FGameplayAttributeData Experience;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Growth, Experience)

	// 현재 레벨
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "ProjectR|Attributes|Growth")
	FGameplayAttributeData Level;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Growth, Level)

	// 미사용 특성 포인트
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TraitPoint, Category = "ProjectR|Attributes|Growth")
	FGameplayAttributeData TraitPoint;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Growth, TraitPoint)

	// 사용한 특성 포인트
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpentPoint, Category = "ProjectR|Attributes|Growth")
	FGameplayAttributeData SpentPoint;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Growth, SpentPoint)

protected:
	UFUNCTION()
	void OnRep_Experience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_TraitPoint(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SpentPoint(const FGameplayAttributeData& OldValue);
};
