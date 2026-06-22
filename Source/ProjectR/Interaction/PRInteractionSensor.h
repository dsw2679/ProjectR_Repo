// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (Interaction Sensor 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "PRInteractionSensor.generated.h"

class UPRInteractorComponent;
class APlayerController;
class APawn;

/**
 * PlayerController에 부착되어 주변 Interactable을 감지하고 Interactor에 포커스 후보를 전달.
 * 화면 중앙 우선 + 거리 차선 + 거리 히스테리시스 적용.
 * bInteractionBlocked가 true면 모든 포커스를 해제 (추후 GameplayTag 기반 갱신 예정).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRInteractionSensor : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInteractionSensor();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 상호작용 차단 플래그 설정. true면 즉시 포커스 해제 */
	void SetInteractionBlocked(bool bInBlocked);

	/** 상호작용 차단 여부 반환 */
	bool IsInteractionBlocked() const { return bInteractionBlocked; }

	/** 현재 포커스 갱신 1회 수행 (외부에서 강제 호출용) */
	void UpdateFocus();

protected:
	/** 후보 트레이스 수행. 결과는 OutCandidates에 누적 (Interface 미구현 액터는 제외) */
	virtual void CollectCandidates(const FVector& Origin, float Radius, TArray<AActor*>& OutCandidates) const;

	/** 화면 중앙 + ScreenCenterRadius 이내 후보 중 중심에 가장 가까운 액터 반환. 없으면 nullptr */
	virtual AActor* FindScreenCenterCandidate(const TArray<AActor*>& Candidates) const;

	/** 후보 중 Pawn 거리 최단 액터 반환. 없으면 nullptr */
	virtual AActor* FindNearestCandidate(const TArray<AActor*>& Candidates) const;

	/** 기존 포커스가 히스테리시스 거리 안에 있고 여전히 인터랙터블이면 유지 */
	virtual bool ShouldRetainFocus(AActor* Actor) const;

	/** Pawn의 발/시야 두 지점 중 한쪽에서라도 Target까지 가시성이 통하면 true */
	virtual bool HasLineOfSight(const AActor* Target) const;

	/** Interactor 컴포넌트 조회 (캐싱). 소유 PlayerController 또는 빙의 Pawn에서 검색 */
	UPRInteractorComponent* GetInteractor() const;

	/** 소유 PlayerController 반환 */
	APlayerController* GetOwningPlayerController() const;

public:
	// 후보 트레이스 반경 (cm). 진입 거리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float FocusableRange = 400.f;

	// 화면 중앙으로부터 후보 인정 반경 (뷰포트 짧은변 대비 비율, 0~1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenCenterRadius = 0.3f;

	// 히스테리시스 배수. 이탈 임계 = FocusableRange * 본 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "1.0"))
	float HysteresisFactor = 1.1f;

	// 후보 트레이스 시 사용할 ObjectType 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	// 트레이스에서 제외할 클래스 필터 (선택). 비어 있으면 모든 액터 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TSubclassOf<AActor> CandidateClassFilter;

private:
	bool bInteractionBlocked = false;

	mutable TWeakObjectPtr<UPRInteractorComponent> CachedInteractor;

	// 현재 포커스 액터 (히스테리시스 판단용)
	TWeakObjectPtr<AActor> FocusedActor;
};
