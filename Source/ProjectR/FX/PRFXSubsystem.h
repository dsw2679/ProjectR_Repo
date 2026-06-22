// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (렌더 프리웜 및 프리로드용 이펙트 캐싱 시스템 구현)
// Author: 배유찬 (사격 궤적 및 데미지 이펙트 런타임 제어 구현)
// Author: 손승우 (보스 특수 연출용 FX 캐싱 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRFXTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "PRFXSubsystem.generated.h"

class AActor;
class UPRFXCue;
class UPRFXRegistryDataAsset;

// 로컬 FX 조회와 실행을 담당하는 월드 단위 서비스
UCLASS()
class PROJECTR_API UPRFXSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// 로컬에서만 FX를 재생
	void PlayLocalFX(FPRFXRequest Request);

	// 예측 키를 기록하고 로컬 FX를 즉시 재생
	FPRFXPredictionKey PlayPredictiveFX(FPRFXRequest Request);

	// 서버 확정 FX를 로컬에서 재생하되 예측 중복 여부를 검사
	void PlayConfirmedFX(FPRFXRequest Request);

	// 지속 FX 시작 시 생성된 Actor를 등록
	void RegisterPersistentFX(FPRFXPersistentId PersistentId, AActor* PersistentActor);

	// 지속 FX 종료 시 사용할 Actor 조회
	AActor* FindPersistentFX(FPRFXPersistentId PersistentId) const;

	// 지속 FX 종료 시 Actor 연결 제거
	void UnregisterPersistentFX(FPRFXPersistentId PersistentId);

	// Registry에서 FXTag에 해당하는 Entry 조회
	bool FindRegistryEntry(FGameplayTag FXTag, FPRFXRegistryEntry& OutEntry) const;

	// FXTag 목록에 필요한 Cue 클래스 경로 수집
	void CollectPreloadAssetPathsForTags(const FGameplayTagContainer& FXTags, TArray<FSoftObjectPath>& OutAssetPaths) const;

protected:
	// Registry에서 Cue 클래스들을 찾고 Instancing 정책에 맞는 Cue 목록 구성
	bool ResolveCues(FGameplayTag FXTag, TArray<UPRFXCue*>& OutCues);

	// Registry 에셋을 DeveloperSettings에서 로드
	UPRFXRegistryDataAsset* GetRegistry() const;

	// Cue 인스턴스에 공통 실행 요청 전달
	void ExecuteCue(const FPRFXRequest& Request, EPRFXPlaybackMode PlaybackMode);

	// Request와 재생 흐름을 Cue 실행 문맥으로 변환
	FPRFXCueContext BuildCueContext(const FPRFXRequest& Request, EPRFXPlaybackMode PlaybackMode) const;

	// 현재 월드에서 사용할 예측 키 생성
	FPRFXPredictionKey GeneratePredictionKey();

private:
	// DeveloperSettings에서 동기 로드한 Registry 에셋 캐시
	UPROPERTY(Transient)
	mutable TObjectPtr<UPRFXRegistryDataAsset> CachedRegistry;

	// InstancedPerPlayer 정책에서 클래스별로 재사용하는 Cue 인스턴스 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRFXCue>> PerPlayerCueInstances;

	// InstancedPerExecution 정책 Cue가 Timer나 Latent Action 동안 유지되도록 잡아두는 인스턴스 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRFXCue>> ExecutionCueInstances;

	// 로컬 예측 재생 후 서버 확정 FX 중복 제거에 사용할 키 집합
	TSet<FPRFXPredictionKey> PredictedFXKeys;

	// 지속 FX 시작과 종료를 연결하는 Actor 핸들
	TMap<FPRFXPersistentId, TWeakObjectPtr<AActor>> PersistentActors;

	// 현재 월드에서 발급하는 예측 키의 Source 식별자
	FGuid LocalPredictionSourceId;

	// 현재 월드에서 발급하는 예측 키 증가값
	int32 NextPredictionSequence = 1;
};
