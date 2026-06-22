// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRPlayerHitSoundDataAsset.generated.h"

class USoundBase;

// 플레이어 체력 피해 피드백 사운드 설정
UCLASS(BlueprintType)
class PROJECTR_API UPRPlayerHitSoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 공격자 CharacterID에 맞는 체력 피해 사운드 반환
	USoundBase* ResolveHealthDamageSound(FName SourceCharacterID) const;

public:
	// CharacterID별 체력 피해 2D 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|Hit")
	TMap<FName, TObjectPtr<USoundBase>> HealthDamageSoundsByCharacterID;

	// CharacterID 매핑이 없을 때 사용할 기본 체력 피해 2D 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|Hit")
	TObjectPtr<USoundBase> DefaultHealthDamageSound;

	// 체력 피해 사운드 최소 재생 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|Hit", meta = (ClampMin = "0.0", Units = "s"))
	float PlaybackCooldown = 0.1f;
};
