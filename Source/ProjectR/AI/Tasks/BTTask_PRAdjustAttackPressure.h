// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Adjust 공격 Pressure 비헤이비어 트리 태스크 구현)
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRAdjustAttackPressure.generated.h"

class UBlackboardComponent;
class APawn;

UENUM(BlueprintType)
enum class EPRAttackPressureAdjustmentMode : uint8
{
	Set			UMETA(DisplayName = "Set"),
	Add			UMETA(DisplayName = "Add"),
	Subtract	UMETA(DisplayName = "Subtract")
};

// Blackboard의 attack_pressure 값을 공용 규칙으로 조정하는 BT Task다.
UCLASS()
class PROJECTR_API UBTTask_PRAdjustAttackPressure : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 기본 Blackboard 키와 회피 패턴용 압박 소모 값을 초기화한다.
	UBTTask_PRAdjustAttackPressure();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 Pawn의 CombatData에서 attack_pressure 최대값을 가져온다.
	bool ResolveMaxPressure(const APawn* ControlledPawn, float& OutMaxPressure) const;

	// 현재 값과 설정 모드를 기준으로 변경 전 pressure 값을 계산한다.
	float CalculateAdjustedPressure(float CurrentPressure) const;

	// 최소/최대 pressure 정책을 적용한다.
	float ClampAdjustedPressure(const APawn* ControlledPawn, float AdjustedPressure) const;

protected:
	// 조정할 attack_pressure Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// pressure 값을 변경하는 방식이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pressure")
	EPRAttackPressureAdjustmentMode AdjustmentMode = EPRAttackPressureAdjustmentMode::Subtract;

	// Set/Add/Subtract에 사용할 pressure 값이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pressure", meta = (ClampMin = "0.0"))
	float Value = 2.0f;

	// pressure가 내려갈 수 있는 최소값이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pressure")
	float MinPressure = 0.0f;

	// CombatDataAsset의 MaxPressure를 상한으로 사용할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pressure")
	bool bClampToEnemyCombatDataMax = true;

	// 0보다 크면 CombatDataAsset 대신 이 값을 최대 pressure로 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Pressure", meta = (ClampMin = "0.0"))
	float MaxPressureOverride = 0.0f;

	// Blackboard 키가 없을 때 Task를 실패시킬지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	bool bFailIfKeyMissing = true;
};
