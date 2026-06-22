// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 매니저 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PRImpactTypes.h"
#include "PRImpactManagerSubsystem.generated.h"

class APRImpactPoolActor;
class UDecalComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class UPRImpactRegistry;

// 총기 충돌 결과를 표면별 Niagara와 Decal 재생으로 바꾸는 월드 단위 Impact 서비스
UCLASS()
class PROJECTR_API UPRImpactManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	// 월드 시작 시 Registry 캐시와 Pool Actor 참조를 비운 상태로 준비함
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 월드 종료 시 활성 반환 타이머와 Pool Actor를 정리함
	virtual void Deinitialize() override;

public:
	// 사격 또는 투사체 HitResult에서 표면 위치, 법선, Impact 태그를 계산해 Impact VFX와 선택적 Decal 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Impact")
	void PlayImpactFromHit(const FHitResult& HitResult, bool bUseDecal = true);

	// 이미 결정된 Impact 태그와 표면 위치를 사용해 Registry Definition 기반 Impact VFX와 선택적 Decal 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Impact")
	void PlayImpactAtLocation(FGameplayTag ImpactTag, FVector ImpactLocation, FVector ImpactNormal, bool bUseDecal = true);

protected:
	// Hit Component, Hit Actor, Physical Surface 순서로 Impact 태그 결정
	FGameplayTag ResolveImpactTag(const FHitResult& HitResult, const FPRImpactContext& Context) const;

	// Impact 위치와 표면 법선만으로 Niagara와 Decal에 공통 적용할 월드 Transform 구성
	FTransform BuildImpactTransform(FVector ImpactLocation, FVector ImpactNormal) const;

	// CVar가 켜져 있을 때 Impact 위치에서 표면 법선 방향 확인용 화살표 표시
	void DrawImpactNormalDebug(FVector ImpactLocation, FVector ImpactNormal) const;

	// CVar 풀링 설정에 따라 풀 슬롯 또는 임시 Niagara Component 획득
	FPRImpactNiagaraComponentLease GetNiagaraComponent();

	// CVar 풀링 설정에 따라 풀 슬롯 또는 임시 Decal Component 획득
	FPRImpactDecalComponentLease GetDecalComponent();

	// Lease 출처가 풀 슬롯이면 반환하고 임시 Component이면 제거함
	void ReleaseNiagaraComponent(FPRImpactNiagaraComponentLease Lease);

	// Lease 출처가 풀 슬롯이면 반환하고 임시 Component이면 제거함
	void ReleaseDecalComponent(FPRImpactDecalComponentLease Lease);

	// 현재 월드에서 Impact Component를 소유할 Pool Actor를 지연 생성해 반환
	APRImpactPoolActor* GetOrCreatePoolActor();

	// 디버그 CVar가 현재 풀링 사용을 허용하는지 확인함
	bool IsPoolingEnabled() const;

	// DeveloperSettings에 지정된 Impact Registry를 동기 로드 후 캐시함
	UPRImpactRegistry* GetRegistry() const;

	// Impact Context를 유지한 상태로 Registry Definition 조회와 VFX, Decal 재생 처리
	void PlayImpactWithContext(FGameplayTag ImpactTag, const FPRImpactContext& Context, bool bUseDecal);

	// Impact Definition의 Niagara System을 Component Lease에 설정하고 종료 반환 경로 등록
	void PlayNiagara(const FPRImpactDefinition& Definition, const FTransform& ImpactTransform, const FPRImpactContext& Context);

	// Impact Definition의 Decal Material과 크기를 Component Lease에 설정하고 고정 수명 반환 등록
	void PlayDecal(const FPRImpactDefinition& Definition, const FTransform& ImpactTransform, const FPRImpactContext& Context);

	// Impact Definition의 Niagara 후보 중 유효한 에셋 하나를 무작위 선택
	UNiagaraSystem* ChooseNiagaraSystem(const FPRImpactDefinition& Definition) const;

	// Impact Definition의 Decal 후보 중 유효한 에셋 하나를 무작위 선택
	UMaterialInterface* ChooseDecalMaterial(const FPRImpactDefinition& Definition) const;

	// HitResult에서 인터페이스 질의와 Transform 정렬에 필요한 최소 Impact Context 구성
	FPRImpactContext BuildImpactContext(const FHitResult& HitResult) const;

	// 인터페이스 결과가 없을 때 Physical Material의 SurfaceType으로 fallback Impact 태그 조회
	FGameplayTag ResolveFallbackImpactTag(const FHitResult& HitResult) const;

private:
	// Niagara System 재생 완료 델리게이트를 받아 해당 Component Lease 반환
	UFUNCTION()
	void HandleNiagaraSystemFinished(UNiagaraComponent* FinishedComponent);

private:
	// DeveloperSettings에서 처음 로드한 Impact Registry를 재사용하기 위한 캐시
	UPROPERTY(Transient)
	mutable TObjectPtr<UPRImpactRegistry> CachedRegistry;

	// 현재 월드에서 Niagara와 Decal Component 풀을 소유하는 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRImpactPoolActor> PoolActor;

	// Niagara 종료 이벤트나 강제 반환 타이머에서 올바른 Lease를 찾기 위한 활성 목록
	TMap<TWeakObjectPtr<UNiagaraComponent>, FPRImpactNiagaraComponentLease> ActiveNiagaraLeases;

	// Niagara 종료 이벤트가 오지 않을 때 MaxVFXLifeTime으로 반환하기 위한 타이머 목록
	TMap<TWeakObjectPtr<UNiagaraComponent>, FTimerHandle> ActiveNiagaraTimers;

	// DecalLifeTime 타이머에서 올바른 Lease를 찾기 위한 활성 목록
	TMap<TWeakObjectPtr<UDecalComponent>, FPRImpactDecalComponentLease> ActiveDecalLeases;

	// Decal을 고정 시간 뒤 반환하기 위한 타이머 목록
	TMap<TWeakObjectPtr<UDecalComponent>, FTimerHandle> ActiveDecalTimers;
};
