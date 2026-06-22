// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRFaerinEncounterBoundaryActor.generated.h"

class APRFaerinEncounterDirector;
class APRPlayerCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EFaerinBoundaryMode : uint8
{
	PreIntro,
	Intro,
	Negotiation,
	PreFight,
	Combat,
	Defeated
};

// Faerin arena 내부 플레이어 감지와 추적만 담당한다. 물리 차단은 OneWayBossWall 계열이 담당한다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinEncounterBoundaryActor : public AActor
{
	GENERATED_BODY()

public:
	APRFaerinEncounterBoundaryActor();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

public:
	// 플레이어가 arena 내부에 들어왔음을 등록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RegisterArenaEnter(APRPlayerCharacter* Player);

	// 플레이어가 arena 내부에서 나갔음을 등록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RegisterArenaExit(APRPlayerCharacter* Player);

	// 현재 arena 내부로 추적 중인 유효 플레이어를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void GetArenaInsidePlayers(TArray<APRPlayerCharacter*>& OutPlayers) const;

	// 인카운터 경계 상태를 초기화한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ClearEncounterState();

	// 현재 인카운터 단계에 맞는 감지 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void SetBoundaryMode(EFaerinBoundaryMode Mode);

	// 지정 플레이어가 arena 내부로 기록되어 있는지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool IsPlayerInsideArena(APRPlayerCharacter* Player) const;

	// 서버 전용 추적 목록 대신 ArenaVolume의 실제 위치와 크기로 플레이어 포함 여부를 판정한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool IsPlayerPhysicallyInsideArena(APRPlayerCharacter* Player) const;

protected:
	UFUNCTION()
	void HandleArenaBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleArenaEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	APRPlayerCharacter* ResolvePlayerCharacter(AActor* Actor) const;
	bool ContainsPlayer(const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player) const;
	void RemovePlayer(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player);
	void CleanupInvalidPlayers();

public:
	// arena 내부 플레이어를 추적하는 영역
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<UBoxComponent> ArenaVolume;

	// Combat 중 arena 입장을 Director에 전달하기 위한 참조
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<APRFaerinEncounterDirector> EncounterDirector = nullptr;

protected:
	// Actor 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	EFaerinBoundaryMode BoundaryMode = EFaerinBoundaryMode::PreIntro;

	TSet<TWeakObjectPtr<APRPlayerCharacter>> ArenaInsidePlayers;
};
