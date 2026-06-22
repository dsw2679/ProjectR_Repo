// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 비 포털 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPortalSequence.h"
#include "PRGameplayAbility_FaerinRainPortalSequence.generated.h"

class APRBossPatternActor;
class APRFaerinRainProjectileManager;

// Faerin 3페이즈 이후 고공에서 아래로 투사체를 쏟는 Rain Portal 시퀀스다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinRainPortalSequence : public UPRGameplayAbility_BossPortalSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinRainPortalSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Rain Portal 배치 설정을 이번 실행용 SpawnConfig로 만든 뒤 포털 시퀀스를 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 생성된 Rain Portal을 그룹 단위 지연으로 켠다.
	virtual void StartSpawnedPortals() override;

	// 현재 설정값으로 고공 Rain Portal SpawnConfig 배열을 다시 만든다.
	void RebuildRainPortalSpawnConfigs();

	// 지정된 인덱스의 Rain Portal 위치 오프셋을 계산한다.
	FVector BuildRainPortalOffset(int32 PortalIndex, int32 ResolvedPortalCount) const;

	// 데이터 플래그 또는 CVar(pr.Faerin.RainPortal.UseLightweight)로 경량 투사체 사용 여부를 해석한다.
	bool ResolveUseLightweightRainProjectile() const;

	// 경량 rain 투사체 매니저를 1개 스폰한다. (서버 권위)
	APRFaerinRainProjectileManager* SpawnRainProjectileManager();

protected:
	// Rain Portal로 생성할 포털 Actor 또는 BP class다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal")
	TSubclassOf<APRBossPatternActor> RainPortalActorClass;

	// Rain Portal 배치의 기준 Actor다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal")
	EPRBossPatternSpawnOrigin RainPortalSpawnOrigin = EPRBossPatternSpawnOrigin::Target;

	// 한 번에 설치할 Rain Portal 수다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal", meta = (ClampMin = "1"))
	int32 PortalCount = 20;

	// Rain Portal이 배치될 수 있는 외곽 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal", meta = (ClampMin = "0.0"))
	float SpawnAreaRadius = 1700.0f;

	// 기준 위치에서 Rain Portal을 띄울 높이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal")
	float SpawnHeight = 1800.0f;

	// 각 포털의 아래 방향 발사를 위해 적용할 월드 회전값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal")
	FRotator DownwardPortalRotation = FRotator::ZeroRotator;

	// 몇 개 단위로 telegraph 시작을 묶을지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal", meta = (ClampMin = "1"))
	int32 PortalStartGroupSize = 5;

	// telegraph 시작 묶음 사이의 지연 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal", meta = (ClampMin = "0.0"))
	float PortalStartGroupInterval = 0.25f;

	// 경량 rain 투사체(ISM 매니저) 경로 사용 여부다. 기본 false면 기존 액터 투사체 경로를 그대로 사용한다.
	// CVar pr.Faerin.RainPortal.UseLightweight 1로도 강제 활성화할 수 있다. (런타임 롤백용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal|Lightweight")
	bool bUseLightweightRainProjectile = false;

	// 경량 경로에서 스폰할 매니저 클래스다. (메시/머터리얼 등은 이 BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal|Lightweight")
	TSubclassOf<APRFaerinRainProjectileManager> RainProjectileManagerClass;

	// 경량 rain 투사체 수명(초)이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|RainPortal|Lightweight", meta = (ClampMin = "0.1"))
	float LightweightProjectileLifetime = 3.0f;
};
