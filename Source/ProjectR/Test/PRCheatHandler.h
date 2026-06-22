// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (패스트 트래블 및 테스트 환경 셋업 디버그 치트 구현)
// Author: 이건주 (무한 탄약 및 Mod 게이지 충전 디버그 치트 구현)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ActiveGameplayEffectHandle.h"
#include "PRCheatHandler.generated.h"

class APRPlayerController;
class UGameplayEffect;
class UPRAbilitySystemComponent;

/**
 * 치트 명령 수신 및 서버 권위에서 실제 작업 수행
 * PlayerController의 ReplicatedSubObject로 등록되어 Server RPC 라우팅 지원
 */
UCLASS(Blueprintable)
class PROJECTR_API UPRCheatHandler : public UObject
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;
	virtual bool IsSupportedForNetworking() const override { return true; }

	/*~ UPRCheatHandler Interface ~*/
	// 클라 요청 수신 후 서버에서 본인 폰을 재스폰
	UFUNCTION(Server, Reliable)
	void ServerCheatRespawn();

	// 클라 요청 수신 후 서버에서 양쪽 무기 슬롯 탄약을 최대치로 세팅
	UFUNCTION(Server, Reliable)
	void ServerCheatFillAmmo();

	// 클라 요청 수신 후 서버에서 무한 모드 GE 적용 또는 회수
	UFUNCTION(Server, Reliable)
	void ServerCheatInfiniteMode(bool bEnable);

	// 클라 요청 수신 후 서버에서 공격력 치트 보너스 누적 적용
	UFUNCTION(Server, Reliable)
	void ServerCheatAddAttackPower(float Amount);

	// 클라 요청 수신 후 서버에서 공격력 치트 보너스 초기화
	UFUNCTION(Server, Reliable)
	void ServerCheatResetAttackPower();

	// 클라 요청 수신 후 서버에서 본인 폰 플라이 모드 토글
	UFUNCTION(Server, Reliable)
	void ServerCheatFly(bool bEnable);

private:
	// 컨트롤러 소유 PlayerState의 ASC 조회
	UPRAbilitySystemComponent* GetOwningPlayerASC() const;

	// 공격력 치트 GE 적용
	bool ApplyAttackPowerCheatEffect();

	// 공격력 치트 GE 회수
	void RemoveAttackPowerCheatEffect();

protected:
	// 무한 모드용 GE 클래스. 에디터에서 Infinite Duration 타입으로 지정 필요
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UGameplayEffect> InfiniteModeEffectClass;

	// 공격력 치트용 GE 클래스. 에디터에서 Infinite Duration 및 SetByCaller Modifier 지정 필요
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UGameplayEffect> AttackPowerCheatEffectClass;

private:
	// 현재 적용 중인 무한 모드 GE 핸들 (서버 권위 상태)
	FActiveGameplayEffectHandle InfiniteModeEffectHandle;

	// 현재 적용 중인 공격력 치트 GE 핸들
	FActiveGameplayEffectHandle AttackPowerCheatEffectHandle;

	// 치트 명령으로 누적한 공격력 보너스
	float CheatAddedAttackPower = 0.0f;
};
