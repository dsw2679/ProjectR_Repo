// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Enemy 인터페이스 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PREnemyInterface.generated.h"

class UBehaviorTree;
class UPRAbilitySystemComponent;
class UPRCombatMoveDataAsset;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPREnemyThreatComponent;
class UBlackboardComponent;
struct FPREnemyMovePresentationConfig;
struct FPREnemyTargetingConfig;

// AIController와 BT Task가 구체 Pawn 클래스에 직접 의존하지 않도록 만드는 공용 인터페이스다.
UINTERFACE(MinimalAPI)
class UPREnemyInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPREnemyInterface
{
	GENERATED_BODY()

public:
	// 서버에서 Ability를 실행하고 태그/속성을 조회할 ASC다.
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const = 0;

	// 현재 공격 대상과 위협 목록을 관리하는 컴포넌트다.
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const = 0;

	// BT가 패턴을 고를 때 사용할 패턴 데이터 자산이다.
	virtual UPRPatternDataAsset* GetPatternDataAsset() const = 0;

	// 전투 이동과 표현 문맥을 담은 데이터 자산이다. 일반 적은 enemy combat data, 보스는 boss combat data를 반환한다.
	virtual UPRCombatMoveDataAsset* GetCombatDataAsset() const = 0;

	// AI Perception 설정값을 담은 데이터 자산이다.
	virtual UPRPerceptionConfig* GetPerceptionConfig() const = 0;

	// AIController가 CombatDataAsset의 타겟팅 설정을 적용하기 전에 몬스터별 보정값을 주입한다.
	virtual void CustomizeEnemyTargetingConfig(FPREnemyTargetingConfig& InOutTargetingConfig) const {}

	// Possess 시 실행할 BehaviorTree 자산이다.
	virtual UBehaviorTree* GetBehaviorTreeAsset() const = 0;

	// 복귀 행동에서 사용할 스폰/기준 위치다.
	virtual FVector GetHomeLocation() const = 0;

	// AIController가 BehaviorTree 실행 전에 몬스터별 Blackboard 초기값을 주입할 수 있게 한다.
	virtual void InitializeEnemyBlackboard(UBlackboardComponent* BlackboardComponent) const {}

	// 전투 이동 표현 문맥을 적용한다.
	virtual void ApplyCombatMovePresentationContext(const FPREnemyMovePresentationConfig& PresentationConfig) = 0;

	// 전투 이동 표현 문맥을 해제한다.
	virtual void ClearCombatMovePresentationContext() = 0;
};
