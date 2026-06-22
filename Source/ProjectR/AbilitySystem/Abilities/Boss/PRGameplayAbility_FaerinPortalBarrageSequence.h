// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 탄막 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPortalSequence.h"
#include "PRGameplayAbility_FaerinPortalBarrageSequence.generated.h"

class APRBossPatternActor;

UENUM(BlueprintType)
enum class EPRFaerinPortalBarrageLineMode : uint8
{
	Horizontal	UMETA(DisplayName = "Horizontal"),
	Vertical	UMETA(DisplayName = "Vertical"),
	Random		UMETA(DisplayName = "Random"),
	Alternating	UMETA(DisplayName = "Alternating")
};

// Faerin 2페이즈 가로/세로 라인 포탈 패턴의 C++ 부모 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPortalBarrageSequence : public UPRGameplayAbility_BossPortalSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPortalBarrageSequence();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void StartSpawnedPortals() override;

	// 이번 실행에서 사용할 라인 방향을 결정한다.
	EPRFaerinPortalBarrageLineMode ResolveLineModeForActivation();

	// 결정된 라인 방향을 기준으로 포탈 스폰 설정을 다시 만든다.
	void RebuildLinePortalSpawnConfigs(EPRFaerinPortalBarrageLineMode ResolvedLineMode);

protected:
	// 라인 포탈에 사용할 Actor 또는 BP class다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal")
	TSubclassOf<APRBossPatternActor> LinePortalActorClass;

	// 가로/세로 라인 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal")
	EPRFaerinPortalBarrageLineMode LineMode = EPRFaerinPortalBarrageLineMode::Random;

	// 한 번에 배치할 라인 포탈 수다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal", meta = (ClampMin = "1"))
	int32 LinePortalCount = 12;

	// 라인 위 포탈 사이 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal", meta = (ClampMin = "0.0"))
	float LinePortalSpacing = 260.0f;

	// 타깃 위치에서 발사 시작선까지 떨어뜨릴 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal", meta = (ClampMin = "0.0"))
	float LineSideDistance = 1250.0f;

	// 라인 포탈을 지면에서 띄우는 높이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal")
	float LinePortalHeight = 260.0f;

	// 몇 개 단위로 telegraph 시작을 묶을지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal", meta = (ClampMin = "1"))
	int32 PortalStartGroupSize = 4;

	// telegraph 묶음 사이 시간 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal", meta = (ClampMin = "0.0"))
	float PortalStartGroupInterval = 0.35f;

	// true면 같은 라인 방향에서도 발사 시작 방향을 매 실행마다 반대로 바꾼다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Portal")
	bool bAlternateFireSide = true;

private:
	bool bNextAlternatingLineUsesHorizontal = true;
	int32 NextFireSideSign = 1;
};

