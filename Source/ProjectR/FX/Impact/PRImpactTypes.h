// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "PRImpactTypes.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogPRImpact, Log, All);

// 피격 대상이 Impact 표면 인터페이스를 제공하지 않을 때 물리 표면 타입을 프로젝트 Impact 태그로 바꾸기 위한 fallback 항목
USTRUCT(BlueprintType)
struct PROJECTR_API FPRImpactSurfaceType
{
	GENERATED_BODY()

	// 물리 머티리얼에서 판정된 엔진 Physical Surface 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact")
	TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

	// SurfaceType에 대응하여 Impact Definition 조회에 사용할 프로젝트 GameplayTag
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact")
	FGameplayTag ImpactTag;
};

// Impact 태그 하나가 재생할 Niagara와 Decal의 에셋 및 수명 정책
USTRUCT(BlueprintType)
struct PROJECTR_API FPRImpactDefinition
{
	GENERATED_BODY()

	// 탄착 위치에서 한 번 재생할 Niagara System 후보 목록. 유효 후보 중 하나를 무작위 선택
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact")
	TArray<TObjectPtr<UNiagaraSystem>> NiagaraSystems;

	// 탄착 표면에 일정 시간 남겨 둘 Decal Material 후보 목록. 유효 후보 중 하나를 무작위 선택
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact")
	TArray<TObjectPtr<UMaterialInterface>> DecalMaterials;

	// Decal Component의 투영 박스에 적용할 월드 단위 크기
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact", meta = (ClampMin = "0.0", Units = "cm"))
	FVector DecalSize = FVector(8.0f, 8.0f, 8.0f);

	// 작은 탄흔에서도 표면이 Decal 투영 볼륨 밖으로 빠지지 않도록 보정할 최소 투영 깊이
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact", meta = (ClampMin = "0.0", Units = "cm"))
	float MinDecalProjectionDepth = 16.0f;

	// 작은 Decal이 화면에서 작게 보일 때 엔진의 화면 크기 기반 fade out을 조절하는 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact", meta = (ClampMin = "0.0"))
	float DecalFadeScreenSize = 0.0f;

	// Decal Component가 표면에 남은 뒤 풀로 돌아갈 때까지의 고정 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact", meta = (ClampMin = "0.0", Units = "s"))
	float DecalLifeTime = 8.0f;

	// Niagara 종료 이벤트가 오지 않는 에셋 실수나 루프 설정을 막기 위한 강제 반환 최대 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectR|Impact", meta = (ClampMin = "0.0", Units = "s"))
	float MaxVFXLifeTime = 2.0f;
};

// 피격 대상이 Impact 태그를 고를 때 필요한 최소 충돌 위치와 표면 방향 문맥
USTRUCT(BlueprintType)
struct PROJECTR_API FPRImpactContext
{
	GENERATED_BODY()

	// Niagara와 Decal을 배치할 충돌 표면의 월드 위치
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	FVector ImpactLocation = FVector::ZeroVector;

	// Niagara 방향과 Decal 투영 방향을 표면에 맞추기 위한 월드 법선
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	FVector ImpactNormal = FVector::UpVector;

	// Impact Niagara가 피격 대상 이동을 따라가야 할 때 부착할 Scene Component. Component 소유권은 이동하지 않고 부착 부모만 사용
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	TObjectPtr<USceneComponent> AttachComponent = nullptr;

	// AttachComponent가 Socket 또는 Bone을 지원할 때 Niagara 부착 기준으로 사용할 이름
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	FName AttachSocketName = NAME_None;
};

// Surface 인터페이스가 현재 피격 표면에 맞는 Impact 태그를 Manager에 알려 주기 위한 결과
USTRUCT(BlueprintType)
struct PROJECTR_API FPRImpactResult
{
	GENERATED_BODY()

	// ImpactTag가 실제 인터페이스 판정 결과인지 fallback으로 넘겨야 하는 빈 결과인지 구분하는 값
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	bool bHasImpactTag = false;

	// Registry에서 Niagara와 Decal 재생 데이터를 찾을 때 사용할 Impact 태그
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectR|Impact")
	FGameplayTag ImpactTag;
};

// Niagara Component를 풀에서 빌렸는지 임시 생성했는지와 반환 시 검증할 슬롯 정보를 함께 보관하는 핸들
USTRUCT()
struct PROJECTR_API FPRImpactNiagaraComponentLease
{
	GENERATED_BODY()

	// 현재 Impact VFX 재생에 사용할 Niagara Component
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> Component = nullptr;

	// true이면 Pool Actor 슬롯으로 반환하고 false이면 수명 종료 후 Component 제거
	UPROPERTY(Transient)
	bool bPooled = false;

	// Pool Actor 내부 NiagaraSlots 배열에서 빌린 슬롯 위치
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	// 같은 슬롯이 재사용된 뒤 늦게 도착한 타이머나 종료 콜백을 무시하기 위한 순번
	UPROPERTY(Transient)
	int32 Generation = 0;

	// Lease가 가리키는 Niagara Component를 아직 사용할 수 있는지 확인함
	bool IsValid() const;
};

// Decal Component를 풀에서 빌렸는지 임시 생성했는지와 반환 시 검증할 슬롯 정보를 함께 보관하는 핸들
USTRUCT()
struct PROJECTR_API FPRImpactDecalComponentLease
{
	GENERATED_BODY()

	// 현재 Impact Decal 표시로 사용할 Decal Component
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Component = nullptr;

	// true이면 Pool Actor 슬롯으로 반환하고 false이면 수명 종료 후 Component 제거
	UPROPERTY(Transient)
	bool bPooled = false;

	// Pool Actor 내부 DecalSlots 배열에서 빌린 슬롯 위치
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	// 같은 슬롯이 재사용된 뒤 늦게 도착한 타이머 반환을 무시하기 위한 순번
	UPROPERTY(Transient)
	int32 Generation = 0;

	// Lease가 가리키는 Decal Component를 아직 사용할 수 있는지 확인함
	bool IsValid() const;
};

// 풀에 보관된 Niagara Component 하나의 사용 상태와 반환 타이머를 추적하는 슬롯
USTRUCT()
struct PROJECTR_API FPRImpactNiagaraSlot
{
	GENERATED_BODY()

	// 풀에서 반복 재사용하는 Niagara Component 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> Component = nullptr;

	// 현재 Component가 Impact 재생에 사용 중인지 나타내는 점유 상태
	UPROPERTY(Transient)
	bool bInUse = false;

	// 동일 슬롯 재사용 이후 이전 재생의 지연 콜백을 구분하기 위한 순번
	UPROPERTY(Transient)
	int32 Generation = 0;

	// 풀 크기 제한에 도달했을 때 가장 오래 사용 중인 슬롯을 고르기 위한 획득 순번
	UPROPERTY(Transient)
	int32 LastAcquireSequence = 0;

	// Niagara 종료 이벤트가 누락될 때 MaxVFXLifeTime 기준으로 강제 반환하기 위한 타이머
	FTimerHandle ReturnTimerHandle;
};

// 풀에 보관된 Decal Component 하나의 사용 상태와 반환 타이머를 추적하는 슬롯
USTRUCT()
struct PROJECTR_API FPRImpactDecalSlot
{
	GENERATED_BODY()

	// 풀에서 반복 재사용하는 Decal Component 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Component = nullptr;

	// 현재 Component가 Impact Decal 표시로 사용 중인지 나타내는 점유 상태
	UPROPERTY(Transient)
	bool bInUse = false;

	// 동일 슬롯 재사용 이후 이전 DecalLifeTime 타이머를 구분하기 위한 순번
	UPROPERTY(Transient)
	int32 Generation = 0;

	// 풀 크기 제한에 도달했을 때 가장 오래 사용 중인 슬롯을 고르기 위한 획득 순번
	UPROPERTY(Transient)
	int32 LastAcquireSequence = 0;

	// DecalLifeTime이 끝났을 때 슬롯을 풀로 되돌리기 위한 타이머
	FTimerHandle ReturnTimerHandle;
};
