// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Moving 타겟 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "PRMovingTarget.generated.h"

class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UBoxComponent;

/**
 * 테스트용 움직이는 과녁. 설정한 간격사이 좌우를 왔다 갔다하되 클라측에서는 rep된 movement로 interpolation하며 움직인다.
 * 데미지 표시를 위해 IAbilitySystemInterface와 IPRCombatInterface를 구현한다.
 */
UCLASS()
class PROJECTR_API APRMovingTarget : public AActor, public IAbilitySystemInterface, public IPRCombatInterface, public IPRPingMarkerTargetInterface
{
	GENERATED_BODY()
	
public:	
	APRMovingTarget();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Enemy; }
	virtual FPRDamageRegionInfo GetDamageRegionInfo(FName BoneName) const override;
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;
	
	/*~ IPRPingMarkerTargetInterface ~*/
	virtual FVector GetPingMarkerWorldLocation_Implementation() const override;
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const override;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Components")
	TObjectPtr<UBoxComponent> RootCollision;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	// 이동 거리 (시작점에서 양방향으로 이 거리만큼 왕복 이동)
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement")
	float MoveDistance = 500.0f;

	// 이동 속도
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement")
	float MoveSpeed = 200.0f;

	// 클라이언트 보간 속도
	UPROPERTY(EditAnywhere, Category = "ProjectR|Movement")
	float InterpSpeed = 40.0f;

	// 서버에서 클라이언트로 위치 동기화
	UPROPERTY(ReplicatedUsing = OnRep_ServerLocation)
	FVector ServerLocation;

	UFUNCTION()
	void OnRep_ServerLocation();

private:
	FVector StartLocation;
	float CurrentDistance = 0.0f;
	int32 MoveDirection = 1;
};
