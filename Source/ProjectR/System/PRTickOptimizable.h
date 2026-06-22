// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (드롭 액터 틱 최적화 연동)
// Author: 배유찬 (최적화 대상 틱 비활성화 인터페이스 구현)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRTickOptimizable.generated.h"

// 거리 기반 Tick 최적화 반경과 초기 상태 설정값
USTRUCT(BlueprintType)
struct PROJECTR_API FPRTickOptimizationConfig
{
	GENERATED_BODY()

	// 클라이언트 렌더 상태를 Tick 활성 근거로 허용하는 외부 활성 기준 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	float TickActivationRadius = 6000.0f;

	// 클라이언트 렌더 상태를 Tick 활성 근거로 유지하는 외부 비활성 기준 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	float TickDeactivationRadius = 6500.0f;

	// 클라이언트 렌더 상태와 무관하게 활성 상태로 전환되는 내부 기준 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	float TickAlwaysActiveActivationRadius = 3000.0f;

	// 클라이언트 렌더 상태와 무관하게 활성 상태를 유지하는 내부 기준 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	float TickAlwaysActiveDeactivationRadius = 3500.0f;

	// 등록 후 첫 거리 평가 전까지 Tick 활성 상태로 유지할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	bool bStartTickActive = true;

	// 대상이 런타임 조건으로 거리 평가 가능 여부를 직접 판단할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|TickOptimization")
	bool bAllowTargetRuntimeEvaluationGate = true;
};

// 월드 Tick 최적화 시스템이 구체 액터 타입에 의존하지 않도록 하는 인터페이스
UINTERFACE(MinimalAPI)
class UPRTickOptimizable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPRTickOptimizable
{
	GENERATED_BODY()

public:
	// Subsystem 등록에 사용할 Tick 최적화 설정 반환
	virtual FPRTickOptimizationConfig GetTickConfig() const = 0;

	// 플레이어 기준점과 비교할 월드 위치 반환
	virtual FVector GetTickLocation() const = 0;

	// 현재 대상이 거리 기반 Tick 평가를 받아도 되는지 여부 반환
	virtual bool CanOptimizeTick() const = 0;

	// 현재 대상의 Tick 활성 상태 반환
	virtual bool IsTickActive() const = 0;

	// 거리 조건으로 결정된 Tick 활성 상태 적용
	virtual void SetTickActive(bool bActive) = 0;
};
