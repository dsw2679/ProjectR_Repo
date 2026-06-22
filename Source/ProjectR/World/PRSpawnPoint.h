// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 스폰 Point 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "PRSpawnPoint.generated.h"

class APlayerStart;

UCLASS()
class PROJECTR_API APRSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APRSpawnPoint();

public:
	// SpawnPoint 식별 태그 반환
	FGameplayTag GetSpawnPointId() const;

	// 지정 태그와 SpawnPoint 식별자 일치 여부 반환
	bool MatchesSpawnPointId(FGameplayTag InSpawnPointId) const;

	// 플레이어 인덱스 기준 연결 PlayerStart 반환
	APlayerStart* GetLinkedPlayerStart(int32 PlayerIndex) const;

	// 플레이어 인덱스 기준 스폰 Transform 반환
	FTransform GetSpawnTransform(int32 PlayerIndex) const;

protected:
	// SpawnPoint 식별 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "SpawnPoint"), Category = "ProjectR|SpawnPoint")
	FGameplayTag SpawnPointId;

	// 플레이어 인덱스별 레벨 에디터 연결 PlayerStart
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|SpawnPoint")
	TArray<TObjectPtr<APlayerStart>> LinkedPlayerStarts;
};
