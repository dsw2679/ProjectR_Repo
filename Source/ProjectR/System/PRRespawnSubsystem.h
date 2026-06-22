// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Respawn 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PRRespawnSubsystem.generated.h"

class AActor;
class AController;
class APlayerState;
class APRSpawnPoint;

DECLARE_MULTICAST_DELEGATE(FPROnPrepareRespawnSignature);

// 리스폰 서브시스템의 현재 처리 단계
UENUM(BlueprintType)
enum class EPRRespawnSystemState : uint8
{
	Idle,
	PrepareRespawn,
	RespawningWorldObjects,
	RespawningPlayers
};

// 현재 월드에서 재생성할 액터의 등록 시점 정보
USTRUCT(BlueprintType)
struct FPRRespawnActorInfo
{
	GENERATED_BODY()

	// 현재 월드에 존재하는 리스폰 대상 액터
	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	// 리스폰 시 생성할 액터 클래스
	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	// 리스폰 기준 Transform
	UPROPERTY()
	FTransform SpawnTransform;
};

// 월드 오브젝트와 플레이어 Pawn 복구를 서버 권위로 처리하는 월드 서브시스템
UCLASS()
class PROJECTR_API UPRRespawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 월드 오브젝트 또는 플레이어 리스폰 준비 단계 알림
	FPROnPrepareRespawnSignature OnPrepareRespawn;

public:
	// 현재 액터를 월드 오브젝트 리스폰 대상 목록에 등록
	UFUNCTION(BlueprintCallable)
	void RegisterRespawnableActor(AActor* InActor);

	UFUNCTION(BlueprintCallable)
	void UnregisterRespawnableActor(AActor* InActor);
	
	// 월드 오브젝트 리스폰 시 파괴할 일회성 액터 등록
	UFUNCTION(BlueprintCallable)
	void RegisterDisposableActor(AActor* InActor);

	// 일회성 액터 파괴 대상 목록에서 제거
	UFUNCTION(BlueprintCallable)
	void UnregisterDisposableActor(AActor* InActor);

	// 등록된 월드 오브젝트 리스폰 목록 초기화
	UFUNCTION(BlueprintCallable)
	void ClearRegisteredActors();

	// 등록된 월드 오브젝트를 등록 정보 기준으로 재생성
	UFUNCTION(BlueprintCallable)
	bool RespawnWorldObjects();

	// 플레이어 상태와 Pawn을 지정 SpawnPoint 기준으로 복구
	UFUNCTION(BlueprintCallable)
	bool RespawnPlayers(FGameplayTag SpawnPointId);

	// 월드 오브젝트와 플레이어를 순서대로 복구
	UFUNCTION(BlueprintCallable)
	bool RespawnWorldAndPlayers(FGameplayTag SpawnPointId);

	// 현재 월드에서 SpawnPoint 태그와 일치하는 SpawnPoint 검색
	UFUNCTION(BlueprintCallable)
	APRSpawnPoint* FindSpawnPointById(FGameplayTag SpawnPointId) const;

	// GameState PlayerArray 기준 플레이어 인덱스 산출
	int32 ResolvePlayerIndex(const AController* Controller) const;

	// 일반 스폰에서 사용할 PlayerStart 또는 SpawnPoint Actor 산출
	AActor* ResolvePlayerStartActor(AController* Controller, FGameplayTag SpawnPointId) const;

	// 플레이어별 리스폰 Transform 산출
	bool ResolvePlayerRespawnTransform(AController* Controller, FGameplayTag SpawnPointId, FTransform& OutTransform) const;

protected:
	// 루트 리스폰 처리 시작과 준비 단계 알림 발행
	bool BeginRespawn();

	// 루트 리스폰 처리 종료
	void EndRespawn();

	// 등록된 월드 오브젝트 재생성 실제 처리
	bool RespawnWorldObjectsInternal();

	// 플레이어 Pawn 복구 실제 처리
	bool RespawnPlayersInternal(FGameplayTag SpawnPointId);

	// 리스폰 서브시스템 상태 변경
	void SetRespawnSystemState(EPRRespawnSystemState NewState);

	// 서버 또는 Standalone 월드 여부 확인
	bool IsServerWorld() const;

	// PlayerState에서 현재 소유 컨트롤러 조회
	AController* ResolveControllerFromPlayerState(APlayerState* PlayerState) const;

	// 기존 Pawn 연결 해제와 파괴 처리
	void DestroyControllerPawn(AController* Controller) const;

	// 리스폰 완료 전환 연출 알림
	void NotifyRespawnComplete(AController* Controller) const;

private:
	// 등록된 월드 오브젝트 리스폰 정보 목록
	UPROPERTY()
	TArray<FPRRespawnActorInfo> RegisteredActors;

	// 월드 오브젝트 리스폰 시 파괴할 일회성 액터 목록
	TSet<TWeakObjectPtr<AActor>> DisposableActors;

	// 현재 리스폰 처리 단계
	EPRRespawnSystemState RespawnSystemState = EPRRespawnSystemState::Idle;
};
