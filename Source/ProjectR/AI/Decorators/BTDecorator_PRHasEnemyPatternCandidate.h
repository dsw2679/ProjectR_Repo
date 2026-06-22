// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Has Enemy Pattern Candidate 조건 판정용 비헤이비어 트리 데코레이터 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTDecorator_PRHasEnemyPatternCandidate.generated.h"

// 현재 문맥에서 특정 패턴 카테고리 후보가 존재하는지 검사하는 데코레이터
UCLASS()
class PROJECTR_API UBTDecorator_PRHasEnemyPatternCandidate : public UBTDecorator
{
	GENERATED_BODY()

public:
	// 데코레이터 기본 설정
	UBTDecorator_PRHasEnemyPatternCandidate();

	/*~ UBTDecorator Interface ~*/
	// 현재 Blackboard/PatternDataAsset 기준으로 조건 만족 여부 계산
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/*~ UBTNode Interface ~*/
	// 에디터 설명 문자열 반환
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// LOS Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// 돌진 경로 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	// 전술 상태 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 공격 압력 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 검사할 패턴 카테고리
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	EPRPatternCategory PatternCategoryFilter = EPRPatternCategory::Any;

	// 비어 있지 않으면 해당 원작 패턴군 태그와 맞는 후보만 검사한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	FGameplayTag PatternGroupFilter;

	// Ability 활성화 가능 여부까지 함께 검사할지 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	bool bCheckAbilityCanActivate = true;

	// 거리까지 포함한 완전 일치인지, 거리만 무시할지 선택
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pattern")
	EPRPatternContextMatchMode MatchMode = EPRPatternContextMatchMode::FullMatch;
};
