// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Fire Game Mode 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PRTestFireGameMode.generated.h"

class APRTestFireShooter;
class APRTestLoadActor;

// 발사 RPC 부하 테스트용 GameMode
// 접속한 각 PlayerController 별로 APRTestFireShooter 를 생성하고 소유권 부여
// 추가로 InitGame 시점에 더미 부하 액터를 일괄 스폰하여 실전에 가까운 트래픽 환경 조성
UCLASS()
class PROJECTR_API APRTestFireGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRTestFireGameMode();

	/*~ AGameModeBase Interface ~*/
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	// 더미 부하 액터를 LoadActorCount 만큼 스폰
	void SpawnLoadActors();

protected:
	// 스폰할 Shooter 클래스. BP 파생도 허용
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Shooter")
	TSubclassOf<APRTestFireShooter> ShooterClass;

	// Shooter 분당 발사 수 설정값
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Shooter")
	float ShooterFireRateRPM = 900.f;

	// Shooter 자동 발사 시작 설정값
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Shooter")
	bool bShooterAutoStart = true;

	// Shooter Reliable RPC 사용 설정값
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Shooter")
	bool bShooterUseReliable = true;

	// Shooter 테스트 시간 설정값. Shooter AutoStopAfterSeconds 로 전달
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Shooter")
	float TestTime = 0.f;

	// 스폰할 더미 부하 액터 클래스. BP 파생도 허용
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Load")
	TSubclassOf<APRTestLoadActor> LoadActorClass;

	// 생성할 더미 부하 액터 수. Montage/Cue/Attribute 트래픽 모사용
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Load")
	int32 LoadActorCount = 8;

	// 더미 액터 Reliable Multicast 송신 주기(초). 0 이하면 송신 안 함
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Load")
	float LoadMulticastIntervalSeconds = 0.5f;

	// 더미 액터 Replicated 프로퍼티 변경 주기(초). 0 이하면 변경 안 함
	UPROPERTY(EditDefaultsOnly, Category = "PR Test|Load")
	float LoadPropertyTickIntervalSeconds = 0.1f;
};
