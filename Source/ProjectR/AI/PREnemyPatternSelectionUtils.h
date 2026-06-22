// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy Pattern Selection Utils 정적 유틸리티 함수 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/PREnemyAITypes.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UPRAbilitySystemComponent;
class UPRPatternDataAsset;
struct FPRPatternRule;

class AActor;
class APawn;
class IPREnemyInterface;

// 패턴 후보 계산에 필요한 런타임 문맥 묶음
struct FPREnemyPatternQueryRuntime
{
	// 현재 제어 중인 Pawn
	APawn* ControlledPawn = nullptr;

	// 현재 사용할 패턴 데이터 자산
	UPRPatternDataAsset* PatternDataAsset = nullptr;

	// Ability 활성화 가능 여부 검사에 사용하는 ASC
	UPRAbilitySystemComponent* AbilitySystemComponent = nullptr;

	// 현재 타겟
	AActor* CurrentTarget = nullptr;

	// 패턴 규칙과 비교할 문맥 값
	FPRPatternContext PatternContext;
};

namespace PREnemyPatternSelectionUtils
{
	// Blackboard 키가 실제로 준비되어 있는지 반환
	bool HasValidBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName);

	// 카테고리 필터와 패턴 규칙이 호환되는지 반환
	bool MatchesPatternCategory(const FPRPatternRule& PatternRule, EPRPatternCategory CategoryFilter);

	// 패턴 그룹 태그 필터와 패턴 규칙이 호환되는지 반환
	bool MatchesPatternGroupTag(const FPRPatternRule& PatternRule, FGameplayTag PatternGroupFilter);

	// BT/Blackboard/Pawn 기준으로 패턴 선택 문맥을 구성
	bool BuildPatternQueryRuntime(
		UBehaviorTreeComponent& OwnerComp,
		const FName CurrentTargetKey,
		const FName HasLOSKey,
		const FName ChargePathClearKey,
		const FName TacticalModeKey,
		const FName AttackPressureKey,
		FPREnemyPatternQueryRuntime& OutRuntime);

	// 조건을 만족하는 패턴 후보를 수집하고, 필요 시 총 가중치도 계산
	void CollectMatchingPatternRules(
		const FPREnemyPatternQueryRuntime& Runtime,
		EPRPatternCategory CategoryFilter,
		FGameplayTag PatternGroupFilter,
		bool bCheckAbilityCanActivate,
		EPRPatternContextMatchMode MatchMode,
		TArray<const FPRPatternRule*>& OutMatchedRules,
		float* OutTotalWeight = nullptr);
}
