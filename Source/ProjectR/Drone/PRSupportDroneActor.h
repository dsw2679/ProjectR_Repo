// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Support Drone Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"
#include "PRSupportDroneActor.generated.h"

class APRProjectileBase;
class UPRSupportDroneDataAsset;
class UAbilitySystemComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

// 드론 Dissolve 방향
UENUM(BlueprintType)
enum class EPRDroneDissolveMode : uint8
{
	Disappear,
	Appear,
};

// 플레이어를 따라다니며 적에게 미사일을 발사하는 보조 드론이다
UCLASS()
class PROJECTR_API APRSupportDroneActor : public AActor
{
	GENERATED_BODY()

public:
	// 드론 컴포넌트 기본 구성
	APRSupportDroneActor();

	/*~ AActor Interface ~*/
	// 서버 타이머와 이벤트 구독 시작
	virtual void BeginPlay() override;
	// 서버 추종 이동 갱신
	virtual void Tick(float DeltaSeconds) override;
	// 서버 타이머와 이벤트 구독 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// 복제 필드 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버에서 드론 소유자와 런타임 데이터를 초기화한다
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|Drone")
	void InitializeDrone(APawn* InOwningPlayer, UPRSupportDroneDataAsset* InDroneData);

	// 외부 시스템이 플레이어 집중 공격 대상을 갱신한다
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|Drone")
	void SetAssistTarget(AActor* InTarget);

	// 서버 드론 Dissolve 연출 시작
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|Drone|Dissolve")
	void StartDissolve(float Duration, EPRDroneDissolveMode DissolveMode);

	// 현재 공격 대상 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Drone")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	// 소유 플레이어 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Drone")
	APawn* GetOwningPlayer() const { return OwningPlayer.Get(); }

protected:
	// 서버에서 현재 공격 대상을 갱신한다
	void RefreshTarget();

	// 서버에서 미사일 발사를 시도한다
	void TryFireMissile();

	// 플레이어 최근 공격 이벤트를 처리한다
	void HandlePlayerAttackTargetEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 현재 추종 목표 위치를 계산한다
	FVector CalculateFollowLocation(float WorldTimeSeconds) const;

	// 가장 가까운 유효 적을 찾는다
	AActor* FindClosestEnemyTarget() const;

	// 공격 가능한 대상인지 확인한다
	bool IsValidDroneTarget(const AActor* TargetActor) const;

	// 대상까지 시야가 확보됐는지 확인한다
	bool HasLineOfSightToTarget(const AActor* TargetActor) const;

	// 미사일 발사 위치를 반환한다
	FVector GetMissileSpawnLocation() const;

	// 미사일 피해 Spec을 생성한다
	FGameplayEffectSpecHandle BuildMissileEffectSpec() const;

	// 서버 타이머를 시작한다
	void StartServerTimers();

	// 서버 타이머를 정리한다
	void StopServerTimers();

	// 이벤트 구독을 시작한다
	void BindAttackTargetEvent();

	// 이벤트 구독을 정리한다
	void UnbindAttackTargetEvent();

	// 로컬 드론 Dissolve 연출 시작
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartDissolve(float Duration, EPRDroneDissolveMode DissolveMode);

	// 로컬 Dissolve 머티리얼 준비
	void PrepareDissolveMaterialsLocal();

	// 로컬 Dissolve 값 갱신
	void TickDissolveLocal();

	// 로컬 Dissolve 연출 완료
	void CompleteDissolveLocal();

	// 로컬 Dissolve 값 머티리얼 적용
	void ApplyDissolveValueLocal(float DissolveValue);

protected:
	// 루트 컴포넌트다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<USceneComponent> SceneRoot;

	// 드론 시각 메시다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<UStaticMeshComponent> DroneMeshComponent;

	// 미사일 발사 기준점이다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<USceneComponent> MissileSpawnComponent;

	// 기본 드론 데이터다
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<UPRSupportDroneDataAsset> DroneData;

	// 소유 플레이어다
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<APawn> OwningPlayer;

	// 현재 공격 대상이다
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|Drone")
	TObjectPtr<AActor> CurrentTarget;

private:
	TWeakObjectPtr<AActor> AssistTarget;
	float AssistTargetExpireWorldTimeSeconds = 0.0f;
	uint32 NextMissileProjectileId = 1;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DissolveDynamicMaterials;

	FTimerHandle TargetRefreshTimerHandle;
	FTimerHandle AttackTimerHandle;
	FTimerHandle DissolveTickTimerHandle;
	FDelegateHandle AttackTargetEventHandle;

	float DissolveDuration = 0.0f;
	float DissolveElapsedTime = 0.0f;
	float DissolveStartValue = 0.0f;
	float DissolveEndValue = 1.0f;
	EPRDroneDissolveMode ActiveDissolveMode = EPRDroneDissolveMode::Disappear;
};
