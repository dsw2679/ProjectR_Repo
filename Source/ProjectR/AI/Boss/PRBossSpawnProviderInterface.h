// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRBossSpawnProviderInterface.generated.h"

class AActor;

// 레벨에 배치된 Blueprint BossSpawner가 인카운터용 보스 스폰 책임을 제공하기 위한 인터페이스다.
UINTERFACE(BlueprintType)
class PROJECTR_API UPRBossSpawnProviderInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPRBossSpawnProviderInterface
{
	GENERATED_BODY()

public:
	// 인카운터 전투 시작 시 실제 전투용 보스 액터를 생성하거나 반환한다.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|AI|Boss|Spawn")
	AActor* SpawnBossForEncounter();
	virtual AActor* SpawnBossForEncounter_Implementation() { return nullptr; }

	// 파티 전멸이나 인카운터 리셋 시 스폰된 보스 액터를 정리한다.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|AI|Boss|Spawn")
	void ResetBossForEncounter(AActor* SpawnedBoss);
	virtual void ResetBossForEncounter_Implementation(AActor* SpawnedBoss) {}
};
