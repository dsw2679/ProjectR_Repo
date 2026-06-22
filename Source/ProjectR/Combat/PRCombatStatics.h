// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 보정용 정적 함수 구현)
// Author: 배유찬 (약점 판정 및 기본 피해 계산 로직 구현)
// Author: 손승우 (적 AI 상태별 피해 보정 정적 함수 구현)
// Author: 이건주 (Penitent 몬스터용 특수 피해 계산식 구현)
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "PRCombatStatics.generated.h"

struct FGameplayEffectSpecHandle;
class UAbilitySystemComponent;
class UGameplayEffect;


// 전투/피해 처리에서 공통으로 쓰는 헬퍼 함수 모음이다.
UCLASS()
class PROJECTR_API UPRCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// IAbilitySystemInterface를 구현한 Actor에서 ASC를 찾는다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static UAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor);

	// 액터의 진영을 반환한다. IPRCombatInterface 미구현 시 Neutral 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static EPRTeam GetActorTeam(const AActor* Actor);

	// 두 액터가 같은 진영인지 판정한다. 한쪽이라도 Neutral이면 false 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static bool IsFriendly(const AActor* SourceActor, const AActor* TargetActor);

	// 부위 보정·프렌들리 감쇠를 적용한 최종 데미지·그로기 데미지를 계산한다.
	static FPRDamageOutputs ComputeDamage(const FPRDamageInputs& Inputs, const FHitResult& HitResult, const AActor* TargetActor);

	// 공격력(BaseDamage)에 비례하여 기본 그로기 피해량을 산출한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static float CalculateBaseGroggyDamage(float BaseDamage);
	
	// ASC 미보유 전투 대상용 단순 피해 컨텍스트 생성
	static FPRDamageAppliedContext BuildSimpleDamageAppliedContext(
		const UAbilitySystemComponent* SourceAbilitySystemComponent,
		const FGameplayEffectSpecHandle& DamageEffectSpecHandle,
		const FHitResult* HitResult);
};
