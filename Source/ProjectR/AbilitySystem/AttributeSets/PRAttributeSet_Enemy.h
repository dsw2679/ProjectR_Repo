// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 관련 피해 배율 속성 구현)
// Author: 배유찬 (데미지 파이프라인 기반 적 속성 연동)
// Author: 손승우 (몬스터 그로기 임계치 및 상태 속성 구현)
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAttributeSet_Enemy.generated.h"

// 일반 적·보스 공통 그로기 게이지·피해 배율 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Enemy : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// GroggyGauge Max 도달 시 발행. bEntered == true면 진입, false면 해제
	UPROPERTY(BlueprintAssignable)
	FPRGroggyStateChangedSignature OnGroggyStateChanged;

	// 그로기 게이지. 0 도달 시 State.Groggy 부여
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GroggyGauge, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData GroggyGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, GroggyGauge)

	// 최대 그로기 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxGroggyGauge, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData MaxGroggyGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, MaxGroggyGauge)

	// 적 공격력
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData AttackPower;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, AttackPower)

protected:
	UFUNCTION() void OnRep_GroggyGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxGroggyGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

};
