// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 Pool Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRImpactTypes.h"
#include "PRImpactPoolActor.generated.h"

class UDecalComponent;
class UNiagaraComponent;
class USceneComponent;

// 하나의 월드에서 Impact용 Niagara Component와 Decal Component 풀을 보유하는 호스트 액터
UCLASS()
class PROJECTR_API APRImpactPoolActor : public AActor
{
	GENERATED_BODY()

public:
	// 풀에 들어갈 런타임 Component들이 붙을 기본 SceneRoot 구성
	APRImpactPoolActor();

public:
	// DeveloperSettings에 지정된 초기 수량과 최대 수량을 기준으로 Niagara와 Decal 슬롯 정책 구성
	void InitializePool(int32 InitialNiagaraCount, int32 InitialDecalCount, int32 MaxNiagaraCount, int32 MaxDecalCount);

	// 비어 있는 Niagara 슬롯을 찾아 재생에 사용할 Lease로 반환
	FPRImpactNiagaraComponentLease AcquireNiagaraComponent();

	// 비어 있는 Decal 슬롯을 찾아 표면 표시용 Lease로 반환
	FPRImpactDecalComponentLease AcquireDecalComponent();

	// 재생이 끝난 Niagara Component를 초기 상태로 되돌리고 슬롯 점유 해제
	void ReleaseNiagaraComponent(FPRImpactNiagaraComponentLease Lease);

	// 표시 시간이 끝난 Decal Component를 초기 상태로 되돌리고 슬롯 점유 해제
	void ReleaseDecalComponent(FPRImpactDecalComponentLease Lease);

	// 풀링 비활성화 CVar 상태에서 한 번만 쓰고 제거할 Niagara Component 생성
	UNiagaraComponent* CreateTransientNiagaraComponent();

	// 풀링 비활성화 CVar 상태에서 한 번만 쓰고 제거할 Decal Component 생성
	UDecalComponent* CreateTransientDecalComponent();

protected:
	// 재사용 가능한 Niagara 슬롯을 찾고 없으면 새 슬롯 생성
	int32 AcquireNiagaraSlot();

	// 재사용 가능한 Decal 슬롯을 찾고 없으면 새 슬롯 생성
	int32 AcquireDecalSlot();

	// Pool Actor를 Outer로 사용해 풀에 보관할 Niagara Component 생성
	UNiagaraComponent* CreatePooledNiagaraComponent();

	// Pool Actor를 Outer로 사용해 풀에 보관할 Decal Component 생성
	UDecalComponent* CreatePooledDecalComponent();

	// Niagara Component를 다음 Impact에서 다시 쓸 수 있도록 에셋과 재생 상태 초기화
	void ResetNiagaraComponent(UNiagaraComponent* Component);

	// Decal Component를 다음 Impact에서 다시 쓸 수 있도록 머티리얼과 표시 상태 초기화
	void ResetDecalComponent(UDecalComponent* Component);

private:
	// 풀 Component들이 월드 Transform을 직접 가지기 전에 기본적으로 붙어 있을 루트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Impact")
	TObjectPtr<USceneComponent> SceneRoot;

	// 재사용 가능한 Niagara Component와 각 슬롯의 점유 상태 목록
	UPROPERTY(Transient)
	TArray<FPRImpactNiagaraSlot> NiagaraSlots;

	// 재사용 가능한 Decal Component와 각 슬롯의 점유 상태 목록
	UPROPERTY(Transient)
	TArray<FPRImpactDecalSlot> DecalSlots;

	// Niagara 풀이 가질 수 있는 최대 슬롯 수. -1이면 빈 슬롯이 없을 때 계속 확장
	UPROPERTY(Transient)
	int32 MaxNiagaraPoolSize = -1;

	// Decal 풀이 가질 수 있는 최대 슬롯 수. -1이면 빈 슬롯이 없을 때 계속 확장
	UPROPERTY(Transient)
	int32 MaxDecalPoolSize = -1;

	// 풀 크기 제한에 도달했을 때 오래된 사용 슬롯을 찾기 위한 증가 순번
	UPROPERTY(Transient)
	int32 NextAcquireSequence = 1;
};
