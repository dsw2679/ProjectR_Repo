// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (피격 상태에 따른 재장전 캔슬 연동)
// Author: 배유찬 (재장전 시퀀스 및 좌수 IK 보정 구현)
// Author: 이건주 (재장전 시 무기 자원 충전 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRGA_Reload.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;
class UPRWeaponDataAsset;
struct FGameplayEventData;

/**
 * 재장전 어빌리티.
 * 활성 무기의 ReloadMontage를 재생하고, 몽타주 노티파이가 송신하는
 * Event.Player.ReloadCommit 이벤트 시점에 슬롯 탄약을 예비탄에서 탄창으로 이동한다.
 * 자원 이동은 서버 권위에서만 수행한다.
 */
UCLASS()
class PROJECTR_API UPRGA_Reload : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Reload();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                 const FGameplayAbilityActorInfo* ActorInfo,
	                                 const FGameplayTagContainer* SourceTags,
	                                 const FGameplayTagContainer* TargetTags,
	                                 FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	// 취소 종료 시 무기 재장전 애니메이션 정리
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	// 재장전 노티파이 이벤트 수신 시 자원 이동을 트리거한다 (서버 권위에서만 실제 적용)
	UFUNCTION()
	void OnReloadCommitEvent(FGameplayEventData EventData);

	// 재장전 몽타주가 정상 종료된 경우 어빌리티 종료
	UFUNCTION()
	void OnReloadMontageCompleted();

	// 재장전 몽타주가 인터럽트/취소된 경우 어빌리티 cancel 종료
	UFUNCTION()
	void OnReloadMontageInterrupted();

	// 슬롯 탄약을 예비탄에서 탄창으로 이동한다
	void ExecuteReload();

	// 현재 활성 슬롯의 무기 데이터를 반환한다
	UPRWeaponDataAsset* GetActiveWeaponData() const;

	// 현재 활성 슬롯에 대응하는 탄약 타입을 반환한다
	bool TryGetActiveAmmoType(EPRAmmoType& OutAmmoType) const;

protected:
	// 주무기 슬롯 재장전 GE. 실행 계산으로 이동량을 산출해 탄창 가산과 예비탄 차감을 적용한다
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ammo")
	TSubclassOf<UGameplayEffect> ReloadAmmoGE_Primary;

	// 보조무기 슬롯 재장전 GE
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ammo")
	TSubclassOf<UGameplayEffect> ReloadAmmoGE_Secondary;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveCommitEventTask;
};
