// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Boss Spawner 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AI/Boss/PRBossSpawnProviderInterface.h"
#include "PRBossSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class APRBossBaseCharacter;
struct FGameplayTag;
struct FInstancedStruct;

/**
 * 보스 스폰 방법
 *
 * 1. bAutoSpawn이 활성화된 경우 BeginPlay에서 서버 권한으로 자동 스폰
 * 2. SpawnBossCharacter를 직접 호출하는 경우 Blueprint 또는 C++에서 즉시 스폰
 * 3. EventManager가 Event.Boss.Spawn을 BroadcastEmpty로 발행한 경우 등록된 APRBossSpawner가 수신 후 스폰
 */
UCLASS()
class PROJECTR_API APRBossSpawner : public AActor, public IPRBossSpawnProviderInterface
{
	GENERATED_BODY()

public:
	APRBossSpawner();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ APRBossSpawner Interface ~*/
	// 보스 캐릭터 스폰 실행
	UFUNCTION(BlueprintCallable)
	APRBossBaseCharacter* SpawnBossCharacter();

	/*~ IPRBossSpawnProviderInterface ~*/
	// 인카운터 디렉터가 전투 시작 시 실제 보스를 스폰하도록 요청한다.
	virtual AActor* SpawnBossForEncounter_Implementation() override;

	// 인카운터 리셋 시 스폰된 보스를 정리한다.
	virtual void ResetBossForEncounter_Implementation(AActor* SpawnedBoss) override;
	
private:
	// 보스 스폰 이벤트 수신 처리
	void HandleBossSpawnEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Spawn")
	TSubclassOf<APRBossBaseCharacter> BossCharacterClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Spawn")
	bool bAutoSpawn = true;

	// 전역 Event.Boss.Spawn 요청을 구독할지 여부. 인카운터 전용 스포너는 Director가 직접 호출하므로 끈다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Spawn")
	bool bListenForGlobalBossSpawnEvent = true;
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Arrow;
	
	UPROPERTY(VisibleInstanceOnly)
	TWeakObjectPtr< APRBossBaseCharacter > SpawnedCharacter;

	// 보스 스폰 이벤트 구독 핸들
	FDelegateHandle BossSpawnEventHandle;
};
