// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Mod 게이지/스택 관리 및 투사체 발사형 Mod 기반 구현)
// Author: 이건주 (GE 기반 무기 자원 갱신 및 Mod 공용 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/AbilitySystem/Abilities/Player/PRFireAbilityTypes.h"
#include "PRGA_Mod.generated.h"

class UPRWeaponModDataAsset;
class UPRWeaponManagerComponent;
class APRWeaponActor;

// ========= Mod Base  =============
class UPRItemInstance_Weapon;
class UAbilitySystemComponent;
enum class EPRWeaponSlotType : uint8;

// 모드 어빌리티의 공통 비용 처리 방식
UENUM(BlueprintType)
enum class EPRModCostPolicy : uint8
{
	None,
	Stack,
	GaugeDuration
};

// 플레이어 모드 스킬 어빌리티 베이스다.
// 데미지와 그로기 데미지는 SetByCaller로 전달하며, DamageGE_FromMod ExecCalc를 사용한다.
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Mod();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	// ========= Mod Base  =============
	// 모드 공통 비용 조건을 검사한다
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 모드 공통 비용을 적용하고 적용된 GE 핸들을 반환
	FActiveGameplayEffectHandle ApplyModCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const;

	// 모드 공통 비용을 적용한다
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// Instance된 어빌리티의 SourceObject(ItemInstance)로부터 ModData를 반환
	UPRWeaponModDataAsset* GetCurrentModData() const;

	// 코스트 정책
	EPRModCostPolicy GetModCostPolicy() const {return ModCostPolicy;}
	
public:
	UFUNCTION(BlueprintCallable)
	UPRWeaponManagerComponent* GetWeaponManager();

protected:
	// 모드 스킬 데미지 GE를 타겟에 적용, Damage와 GroggyDamage를 SetByCaller로 전달
	void ApplyDamage(AActor* TargetActor, float Damage, float GroggyDamage = 0.0f, const FHitResult* HitResult = nullptr);

	// ========= Mod Base  =============
	// 모드 비용 처리 방식을 설정한다
	void SetModCostPolicy(EPRModCostPolicy InModCostType);

	// 마지막으로 적용된 모드 비용 GE 핸들을 반환
	FActiveGameplayEffectHandle GetLastAppliedModCostHandle() const;

	// 현재 슬롯의 모드 게이지 Attribute를 반환
	FGameplayAttribute GetCurrentModGaugeAttribute(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 어빌리티 회수 시 런타임 상태 정리
	virtual void CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec);

private:
	// ========= Mod Base  =============
	bool TryGetCurrentModCostContext(const FGameplayAbilityActorInfo* ActorInfo, EPRWeaponSlotType& OutSlotType, UAbilitySystemComponent*& OutASC, UPRItemInstance_Weapon*& OutWeaponInstance) const;
	// 스택 코스트 확인
	bool HasModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 게이지가 가득 찬 상태인지 확인
	bool HasFullModGaugeCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 현재 슬롯의 지속시간형 Mod 게이지 비용이 적용 중인지 확인
	bool HasActiveModGaugeLock(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 스택 소모 코스트 적용
	FActiveGameplayEffectHandle ApplyModStackCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const;
	FActiveGameplayEffectHandle ApplyModGaugeDurationCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const;
	FGameplayAttribute GetModGaugeAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetMaxModGaugeAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetModStackAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetMaxModStackAttribute(EPRWeaponSlotType SlotType) const;
	
protected:
	// 모드 비용 처리 방식
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod")
	EPRModCostPolicy ModCostPolicy = EPRModCostPolicy::None;
	
	// 지속시간형 모드 효과의 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Cost", meta = (ClampMin = "0.0"))
	float ModDuration = 0.0f;
	
	// 활성 무기 캐시
	TWeakObjectPtr<APRWeaponActor> CurrentWeapon;
	
	// 무기 매니저 캐시
	TWeakObjectPtr<UPRWeaponManagerComponent> CachedWeaponManager;

	// 마지막으로 적용된 모드 비용 GE 핸들
	FActiveGameplayEffectHandle LastAppliedModCostHandle;

	// 적용된 모드 비용 GE 핸들 목록
	TArray<FActiveGameplayEffectHandle> ActiveModCostHandles;
};
