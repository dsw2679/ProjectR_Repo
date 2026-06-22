// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 스폰 Pattern Actors 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRGameplayAbility_BossSpawnPatternActors.generated.h"

class APRBossPatternActor;
class UEnvQuery;
class USceneComponent;

// 보스 패턴 Helper Actor를 어느 기준점에 생성할지 정의한다.
UENUM(BlueprintType)
enum class EPRBossPatternSpawnOrigin : uint8
{
	Boss	UMETA(DisplayName = "Boss"),
	Target	UMETA(DisplayName = "Target")
};

// 보스 패턴 Helper Actor의 위치 선정 방식이다.
UENUM(BlueprintType)
enum class EPRBossPatternSpawnLocationMode : uint8
{
	OriginOffset	UMETA(DisplayName = "OriginOffset"),
	EnvQuery		UMETA(DisplayName = "EnvQuery")
};

// 보스 패턴 Helper Actor 하나의 생성 설정이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossPatternActorSpawnConfig
{
	GENERATED_BODY()

	// 생성할 공용 Helper Actor 또는 BP 파생 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TSubclassOf<APRBossPatternActor> PatternActorClass;

	// Helper Actor 위치 선정 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	EPRBossPatternSpawnLocationMode SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;

	// 생성 기준점이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	EPRBossPatternSpawnOrigin SpawnOrigin = EPRBossPatternSpawnOrigin::Target;

	// 값이 있으면 SpawnOrigin Actor 안의 해당 SceneComponent를 스폰/부착 기준으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	FName OriginComponentName = NAME_None;

	// 기준 Actor의 로컬 좌표계에서 적용할 위치 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	FVector LocalOffset = FVector::ZeroVector;

	// true면 LocalOffset을 Origin 회전에 영향받지 않는 월드 좌표 오프셋으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	bool bUseWorldSpaceOffset = false;

	// 계산된 회전에 추가로 적용할 회전 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// true면 RotationOffset을 Origin 회전에 영향받지 않는 월드 회전으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	bool bUseWorldSpaceRotation = false;

	// 생성 위치에서 타겟을 바라보도록 회전을 보정할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	bool bFaceTarget = true;

	// 타겟 바라보기 회전에서 Pitch/Roll을 제거해 수평 회전만 사용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss", meta = (EditCondition = "bFaceTarget"))
	bool bUseYawOnlyFacing = true;

	// EnvQuery 위치 선정에 사용할 EQS 템플릿이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TObjectPtr<UEnvQuery> SpawnQueryTemplate = nullptr;

	// EQS 실행 방식이다. 후보 랜덤 선택을 쓰면 내부적으로 AllMatching으로 실행한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TEnumAsByte<EEnvQueryRunMode::Type> SpawnQueryRunMode = EEnvQueryRunMode::SingleResult;

	// EQS 후보 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS Named Float Param 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 값이 있으면 같은 키를 쓰는 EnvQuery SpawnConfig들이 한 번 계산한 기준 위치를 공유한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	FName SharedEnvQueryResultKey = NAME_None;

	// 상위 후보 최대 선택 개수다. 0 이하면 개수 제한이 없다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery", ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery", ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;

	// EQS 실패 시 OriginOffset 방식으로 대체할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	bool bFallbackToOriginOffsetWhenQueryFails = true;

	// true면 스폰 직후 Origin Actor에 KeepWorldTransform으로 부착해 함께 움직이게 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Attach")
	bool bAttachToOriginAfterSpawn = false;
};

// 보스 패턴 Helper Actor들을 설정 배열 기준으로 생성하는 공용 Ability다.
// PortalPairSequence, SwordHazard 사전 배치처럼 지속형 Actor가 필요한 패턴의 첫 공용 진입점으로 사용한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_BossSpawnPatternActors : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossSpawnPatternActors();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 지정한 설정으로 Helper Actor 생성 Transform을 계산한다.
	bool BuildPatternActorSpawnTransform(const FPRBossPatternActorSpawnConfig& SpawnConfig, FTransform& OutSpawnTransform);

	// OriginOffset 방식으로 Helper Actor 생성 Transform을 계산한다.
	bool BuildOriginOffsetSpawnTransform(const FPRBossPatternActorSpawnConfig& SpawnConfig, FTransform& OutSpawnTransform) const;

	// EQS 방식으로 Helper Actor 생성 위치를 계산한다.
	bool RunSpawnLocationQuery(const FPRBossPatternActorSpawnConfig& SpawnConfig, FVector& OutSpawnLocation) const;

	// EnvQuery 기준 위치를 얻고, 공유 키가 있으면 같은 실행 안에서 재사용한다.
	bool ResolveEnvQueryBaseLocation(const FPRBossPatternActorSpawnConfig& SpawnConfig, FVector& OutBaseLocation);

	// 기준 위치에 SpawnConfig의 offset을 적용한다.
	FVector ApplySpawnLocationOffset(
		const FPRBossPatternActorSpawnConfig& SpawnConfig,
		const FVector& BaseLocation,
		const FRotator& OffsetRotation) const;

	// SpawnConfig의 최종 회전을 계산한다.
	FRotator BuildSpawnRotation(
		const FPRBossPatternActorSpawnConfig& SpawnConfig,
		const FVector& SpawnLocation,
		const FRotator& BaseRotation) const;

	// Helper Actor를 하나 생성하고 초기화한다.
	APRBossPatternActor* SpawnPatternActor(const FPRBossPatternActorSpawnConfig& SpawnConfig);

	// SpawnConfig 기준 Origin Actor를 반환한다.
	AActor* ResolveSpawnOriginActor(const FPRBossPatternActorSpawnConfig& SpawnConfig) const;

	// SpawnConfig 기준 Origin SceneComponent를 반환한다.
	USceneComponent* ResolveSpawnOriginComponent(const FPRBossPatternActorSpawnConfig& SpawnConfig) const;

	// SpawnConfig에 따라 스폰 직후 부착 처리를 수행한다.
	void ApplyPostSpawnAttachment(APRBossPatternActor* SpawnedActor, const FPRBossPatternActorSpawnConfig& SpawnConfig) const;

	// Ability 실행마다 공유 EQS 기준 위치 캐시를 초기화한다.
	void ResetSharedEnvQueryResultCache();

	// Helper Actor 생성이 끝난 뒤 BP 후처리를 연결하는 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss")
	void BP_OnPatternActorsSpawned(const TArray<APRBossPatternActor*>& SpawnedActors);

protected:
	// 이 Ability가 생성할 Helper Actor 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TArray<FPRBossPatternActorSpawnConfig> PatternActorSpawnConfigs;

	// 이번 활성화에서 생성된 Helper Actor 목록이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TArray<TObjectPtr<APRBossPatternActor>> SpawnedPatternActors;

	// 같은 실행 안에서 공유하는 EQS 기준 위치 캐시다.
	UPROPERTY(Transient)
	TMap<FName, FVector> SharedEnvQueryResultLocations;
};
