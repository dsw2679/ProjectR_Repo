// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (이펙트 영역 액터 기초 설계 및 리스폰 연동 구현)
// Author: 이건주 (드론 회복 및 Penitent 몬스터용 특수 이펙트 영역 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "PREffectArea.generated.h"

class USphereComponent;
struct FGameplayEffectSpecHandle;

DECLARE_LOG_CATEGORY_EXTERN(LogPREffectArea, Log, All);

UENUM(BlueprintType)
enum class EEffectAreaApplyPolicy : uint8
{
	Once, // 영역 활성화 후 하나의 액터에게 단 한번만 적용
	WhileOverlap, // 영역 내 액터에 ReapplyInterval 주기로 재적용. 이탈 또는 영역 종료 시 누적된 활성 GE 모두 해제
	KeepDuration, // 영역 내에서 지속 적용 (GE stacking policy 설정 필요), 영역 벗어나도 GE 해제 x (GE duration 설정 필요)
};

// WhileOverlap 정책에서 액터별 활성 GE 핸들 누적 보관용 래퍼
USTRUCT()
struct FPREffectAreaActiveHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> Handles;
	
	void Add(const FActiveGameplayEffectHandle& InHandle)
	{
		Handles.Add(InHandle);
	}
};

UCLASS()
class PROJECTR_API APREffectArea : public AActor
{
	GENERATED_BODY()

public:
	APREffectArea();

	/*~ APREffectArea Interface ~*/
	// 이펙트 영역 초기화 (Auth only)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void InitEffectArea(const FGameplayEffectSpecHandle& InEffectSpec,
		UAbilitySystemComponent* InSourceASC,
		float InDuration);

	// 지표면 스냅 대상 컴포넌트 등록 (BP ConstructionScript에서 호출)
	UFUNCTION(BlueprintCallable, Category = "ProjectR|EffectArea")
	void AddGroundSnapComponent(USceneComponent* Component);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ APREffectArea Interface ~*/
	// 오버랩 진입 시 이펙트 적용 처리
	UFUNCTION(BlueprintCallable)
	virtual void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 오버랩 이탈 시 이펙트 해제 처리 (WhileOverlap 정책 전용)
	UFUNCTION(BlueprintCallable)
	virtual void OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 등록된 컴포넌트들을 지표면 높이로 스냅
	void SnapComponentsToGround();

	// 차폐 확인 후 대상에게 이펙트 적용
	FActiveGameplayEffectHandle ApplyEffectToActorWithOcclusion(AActor* TargetActor);

	// 범위 중심과 대상 사이 차폐 확인
	bool IsAreaDamageBlocked(AActor* TargetActor, FHitResult& OutBlockHit) const;

private:
	// 영역 수명 만료 시 호출되어 잔여 GE 해제 후 액터 파괴
	void OnLifetimeExpired();

	// 현재 영역 내 모든 액터에 GE 재적용 (WhileOverlap 정책 전용)
	void ReapplyEffectToOverlappingActors();

	// 대상 액터에 이펙트 스펙 적용 후 핸들 반환
	FActiveGameplayEffectHandle ApplyEffectToActor(AActor* TargetActor) const;

	// ASC 미보유 전투 대상에게 대미지 컨텍스트 적용
	bool TryApplyEffectToCombatTarget(AActor* TargetActor, const FHitResult* HitResult) const;

	// 추적 중인 WhileOverlap 핸들 일괄 제거
	void RemoveAllTrackedEffects();

	// 특정 액터에 적용된 WhileOverlap 핸들 일괄 제거
	void RemoveTrackedEffectsForActor(AActor* TargetActor);

	// ApplyPolicy와 EffectSpec의 GE 설정 정합성 검증 후 경고 로그 출력
	void ValidatePolicyAgainstEffect() const;

public:
	// 이펙트 적용 정책
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|EffectArea")
	EEffectAreaApplyPolicy ApplyPolicy;

	// WhileOverlap 정책 재적용 주기 (초). 0 이하면 진입 시 1회만 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|EffectArea",
		meta = (ClampMin = "0.0", EditCondition = "ApplyPolicy == EEffectAreaApplyPolicy::WhileOverlap"))
	float ReapplyInterval = 0.5f;

protected:
	// 오버랩 감지용 구체 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|EffectArea")
	TObjectPtr<USphereComponent> CollisionComponent;

	// 적용할 이펙트 스펙 핸들
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|EffectArea")
	FGameplayEffectSpecHandle EffectSpecHandle;

	// 이펙트 소유자 ASC
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|EffectArea")
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 영역 잔여 시간 (초)
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|EffectArea")
	float RemainingTime;

private:
	// 지표면 스냅 대상 컴포넌트 목록
	UPROPERTY()
	TArray<USceneComponent*> GroundSnapComponents;

	// Once 정책 중복 적용 차단용 액터 집합
	UPROPERTY()
	TSet<AActor*> OnceAppliedActors;

	// WhileOverlap 정책 액터별 활성 GE 핸들 누적
	UPROPERTY()
	TMap<AActor*, FPREffectAreaActiveHandles> AppliedEffectHandles;

	// 영역 수명 타이머 핸들
	FTimerHandle LifetimeTimerHandle;

	// WhileOverlap 재적용 주기 타이머 핸들
	FTimerHandle ReapplyTimerHandle;
};
