// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (기본 재장전 연산 및 좌수 IK 보정 로직 구현)
// Author: 이건주 (Mod 버프에 따른 재장전 가중치 연산 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRReloadExecCalc.generated.h"

// 재장전 GE 실행 계산기.
// 슬롯의 Magazine·MaxMagazine·Reserve를 Source 캡처해 이동량을 산출하고,
// 한 번의 실행에서 Magazine은 가산, Reserve는 차감 두 모디파이어를 동시에 출력한다.
UCLASS()
class PROJECTR_API UPRReloadExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPRReloadExecCalc();

	/*~ UGameplayEffectExecutionCalculation Interface ~*/
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// 어느 슬롯의 자원을 이동할지 결정. 디자이너가 BP에서 Primary/Secondary용으로 분기 생성
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ammo")
	EPRAmmoType TargetAmmoType = EPRAmmoType::Primary;
};
