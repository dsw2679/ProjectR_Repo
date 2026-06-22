// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Item Spawner Actor 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "PRItemSpawnerActor.generated.h"

class APRRewardPickupActor;
class UBoxComponent;
class UPRAmmoDataAsset;
class UPREventManagerSubsystem;
class UPRRespawnSubsystem;
struct FInstancedStruct;

// 지정 영역의 NavMesh 위에 보상 픽업을 주기적으로 생성하는 스포너
UCLASS(Blueprintable)
class PROJECTR_API APRItemSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	// 아이템 스포너 기본 구성
	APRItemSpawnerActor();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 서버 스폰 타이머 시작
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|ItemSpawner")
	void StartSpawning();

	// 서버 스폰 타이머 정지
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|ItemSpawner")
	void StopSpawning();

	// 서버 즉시 스폰 1회 실행
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|ItemSpawner")
	bool SpawnItemOnce();

protected:
	// 활성 이벤트 태그 구독 등록
	void BindActivateEvents();

	// 활성 이벤트 태그 구독 해제
	void UnbindActivateEvents();

	// 리스폰 준비 이벤트 구독 등록
	void BindPrepareRespawnEvent();

	// 리스폰 준비 이벤트 구독 해제
	void UnbindPrepareRespawnEvent();

	// 활성 이벤트 수신 시 스폰 타이머 시작
	void HandleActivateEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 리스폰 준비 시점 스포너 비활성화
	void HandlePrepareRespawn();

	// 타이머 기반 스폰 처리
	void HandleSpawnTimerElapsed();

	// 현재 유효하지 않은 픽업 참조 제거
	void PruneAlivePickups();

	// 스폰 가능 상태 확인
	bool CanSpawnItem() const;

	// 스폰 대상 탄약 데이터 선택
	UPRAmmoDataAsset* PickAmmoData() const;

	// 탄약 데이터 기반 확정 보상 생성
	FPRResolvedDropReward BuildAmmoReward(UPRAmmoDataAsset* AmmoData) const;

	// 박스 내부 NavMesh 스폰 위치 선택
	bool FindSpawnLocation(FVector& OutLocation) const;

	// 월드 위치의 스폰 박스 내부 포함 여부 확인
	bool IsInsideSpawnBounds(const FVector& WorldLocation) const;

protected:
	// 스폰 가능 영역을 정의하는 박스 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner")
	TObjectPtr<UBoxComponent> SpawnBounds;

	// BeginPlay 시 서버 자동 스폰 타이머 시작 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner")
	bool bAutoStart = true;

	// 스포너 동작 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner")
	bool bSpawnEnabled = true;

	// 수신 시 스폰 타이머를 시작할 월드 이벤트 태그 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner|Event", meta = (Categories = "Event"))
	TArray<FGameplayTag> ActivateEventTags;

	// 첫 스폰까지 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "0.0"))
	float InitialSpawnDelay = 0.0f;

	// 반복 스폰 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 20.0f;

	// 동시에 유지할 최대 픽업 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "1"))
	int32 MaxAlivePickups = 3;

	// 위치 선택 재시도 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "1"))
	int32 SpawnLocationMaxAttempts = 16;

	// 랜덤 선택 가능한 탄약 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner")
	TArray<TObjectPtr<UPRAmmoDataAsset>> SpawnableAmmoItems;

	// 탄약 보상 최소 백분율 (MaxMagazineAmmo 기준. 90 = 탄창의 90%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "1"))
	int32 MinQuantity = 90;

	// 탄약 보상 최대 백분율 (MaxMagazineAmmo 기준. 90 = 탄창의 90%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner", meta = (ClampMin = "1"))
	int32 MaxQuantity = 90;

	// 픽업 Claim 시 보상 분배 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|ItemSpawner")
	EPRRewardDistributionRule DistributionRule = EPRRewardDistributionRule::Personal;

private:
	// 반복 스폰 타이머 핸들
	FTimerHandle SpawnTimerHandle;

	// EventManager에 등록한 활성 이벤트 수신 핸들 목록
	TMap<FGameplayTag, FDelegateHandle> ActivateEventHandles;

	// RespawnSubsystem 리스폰 준비 이벤트 수신 핸들
	FDelegateHandle PrepareRespawnHandle;

	// 현재 스포너가 생성한 살아있는 픽업 목록
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APRRewardPickupActor>> AlivePickups;
};
