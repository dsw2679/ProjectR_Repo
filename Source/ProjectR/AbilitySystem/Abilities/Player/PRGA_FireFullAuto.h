// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (연사 반동 애니메이션 및 경직 제어 구현)
// Author: 배유찬 (연사 투사체 사격 제어 및 탄약 소모 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGA_Fire.h"
#include "PRGA_FireFullAuto.generated.h"

/**
 * 풀오토 사격 어빌리티. 인풋 유지 동안 FireInterval 간격으로 FireOneShot 반복.
 */
UCLASS()
class PROJECTR_API UPRGA_FireFullAuto : public UPRGA_Fire
{
	GENERATED_BODY()

public:
	UPRGA_FireFullAuto();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 인풋 릴리즈 콜백. 어빌리티 종료
	UFUNCTION()
	void HandleInputRelease(float TimeHeld);

protected:
	// 활성화 즉시 1발 발사 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|FullAuto")
	bool bFireOnActivate = true;
};
