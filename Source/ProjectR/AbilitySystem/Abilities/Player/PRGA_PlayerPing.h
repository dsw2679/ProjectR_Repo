// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 핑 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/UI/WorldMarker/PRWorldMarkerTypes.h"
#include "PRGA_PlayerPing.generated.h"

class APlayerController;

// 플레이어 핑 어빌리티
UCLASS()
class PROJECTR_API UPRGA_PlayerPing : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerPing();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	// 카메라 전방 핑 트레이스 수행
	bool TracePingTarget(FHitResult& OutHit) const;

	// 트레이스 결과에서 핑 대상 액터 해석
	AActor* ResolvePingTargetActor(const FHitResult& Hit) const;

	// 서버 요청 데이터 생성
	FPRWorldMarkerRequest BuildMarkerRequest(const FHitResult& Hit) const;

	// 서버 제출 처리
	void SubmitMarkerRequest(const FPRWorldMarkerRequest& Request);

	// 요청 컨트롤러 반환
	APlayerController* GetRequestingController() const;

	// 서버 핑 제출 요청
	UFUNCTION(Server, Reliable)
	void ServerSubmitPing(FPRWorldMarkerRequest Request);

protected:
	// 핑 트레이스 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ping", meta = (ClampMin = "0.0"))
	float MaxTraceDistance = 20000.0f;

	// 핑 트레이스 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ping")
	TEnumAsByte<ECollisionChannel> PingTraceChannel = PRCollisionChannels::ECC_PingMarker;

	// 디버그 라인 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ping|Debug")
	bool bDrawDebugTrace = false;

	// 디버그 라인 표시 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ping|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawDuration = 1.0f;
};
