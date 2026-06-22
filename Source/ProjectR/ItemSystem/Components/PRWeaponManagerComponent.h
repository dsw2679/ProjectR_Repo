// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기별 조준 반동 애니메이션 및 줌 제어 연동 구현)
// Author: 배유찬 (무기 교체(Swap) 메커니즘, 장전/탄약 관리 및 Mod 게이지 제어 로직 구현)
// Author: 이건주 (배리어 모드 장착 흐름 및 무기 상태 HUD 실시간 동기화 구현)
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponAnimationTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRWeaponManagerComponent.generated.h"

class UNiagaraSystem;
class APRWeaponActor;
class APRPlayerState;
class UAnimInstance;
class UGameplayEffect;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Weapon;
class UPRInventoryComponent;
class UPRItemInstance_Weapon;
class UPRWeaponDataAsset;
class UPRWeaponManagerComponent;
class UPRWeaponModDataAsset;
class UPRCrosshairConfig;
class APRCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRWeaponEquipmentChangedSignature, UPRWeaponManagerComponent*, WeaponManagerComponent, EPRWeaponSlotType, ChangedSlot);

// 슬롯 장착 상태가 유지하는 지속형 GameplayEffect 핸들 묶음이다.
USTRUCT()
struct FPREquipSlotEffectHandles
{
	GENERATED_BODY()

	// 슬롯 탄약 최대치 지속 효과 핸들
	FActiveGameplayEffectHandle AmmoMaxHandle;

	// 슬롯 Mod 최대 자원 지속 효과 핸들
	FActiveGameplayEffectHandle ModMaxHandle;
};

// 캐릭터 기준 무기 장착 공개 상태와 슬롯별 로컬 Actor 생명주기를 관리하는 허브다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 무기 장착 허브 컴포넌트를 초기화한다
	UPRWeaponManagerComponent();

	/*~ UActorComponent Interface ~*/
	// 종료 시점에 슬롯별 로컬 Actor를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 복제할 공개 상태를 등록한다
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 현재 활성 무기 슬롯을 반환한다
	EPRWeaponSlotType GetCurrentWeaponSlot() const { return CurrentWeaponSlot; }

	// 현재 활성 무기의 공개 비주얼 정보를 반환한다
	const FPRWeaponVisualInfo& GetCurrentWeaponVisualInfo() const;

	// 현재 활성 무기의 크로스헤어 설정을 반환한다
	const UPRCrosshairConfig* GetActiveCrosshairConfig() const;

	// 현재 무장 상태를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	EPRArmedState GetArmedState() const { return ArmedState; }

	// PlayerState와 ASC, 인벤토리 연결 캐시를 초기화한다
	void InitializeRuntimeLinks();

	// 새 폰(Pawn)이 배정될 때 호출하여 무기 부착과 애니메이션을 갱신한다
	void InitializeWithPawn(APRCharacterBase* InPawn);

	// 리스폰 전 기존 Pawn 무기 Actor와 표현 상태 정리
	void ResetSystem();

	// Save Data
	FPRWeaponManagerSaveData MakeSaveData() const;
	void ApplySaveData(const FPRWeaponManagerSaveData& InSaveData);
	
	// 현재 캐시된 폰(Pawn) 소유자를 반환한다
	APRCharacterBase* GetPawnOwner() const { return CachedPawnOwner.Get(); }

	// 대상 슬롯에 연결된 무기 Item 원본을 반환한다
	UPRItemInstance_Weapon* GetWeaponInstanceBySlotType(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯의 현재 무기 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponDataAsset* GetWeaponDataBySlotType(EPRWeaponSlotType SlotType) const;

	// 현재 활성 무기의 강화 단계가 반영된 기본 피해량을 반환한다
	float GetCurrentWeaponBaseDamage() const;

	// 대상 슬롯의 현재 공개 비주얼 정보를 반환한다
	const FPRWeaponVisualInfo& GetVisualInfoBySlotType(EPRWeaponSlotType SlotType) const;

	// 인벤토리 인덱스에 있는 무기를 데이터가 지정한 슬롯에 연결한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool EquipInventoryWeaponAtIndex(int32 InventoryIndex);

	// 인벤토리 소유 무기를 데이터가 지정한 슬롯에 연결한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool EquipWeapon(UPRItemInstance_Weapon* WeaponItem);

	// 지정 슬롯의 무기 연결만 해제한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool UnequipWeaponFromSlot(EPRWeaponSlotType TargetSlot);

	// 대상 슬롯 무기에 Mod를 장착하거나 교체한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

	// 활성 무기 슬롯을 지정 슬롯으로 전환한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 현재 무장 상태를 변경한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SetWeaponArmedState(EPRArmedState NewArmedState);

	// 현재 슬롯의 사격 모드 태그를 갱신한다
	void RefreshCurrentWeaponFireModeTags();

	// 현재 활성 슬롯에 대응하는 로컬 무기 Actor를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	APRWeaponActor* GetActiveWeaponActor() const;
	
	// 애니메이션용 공개 Getter 함수
	// 04.29 김동석 수정 현재 AimOffset 선택에 사용할 무장 슬롯을 반환한다
	// UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	// EPRWeaponSlotType GetAimOffsetWeaponSlot() const;

	// 지정 무기 Item이 현재 매니저의 슬롯 원본인지 확인한다
	bool IsManagingWeaponItem(const UPRItemInstance_Weapon* WeaponItem) const;

	// 인벤토리에서 변경된 Mod 상태를 현재 관리 중인 무기 슬롯에 반영한다
	void HandleInventoryWeaponModChanged(UPRItemInstance_Weapon* WeaponItem);

	// 강화된 무기가 현재 활성 무기라면 전투 스탯 GE를 즉시 갱신한다
	void RefreshCurrentWeaponUpgradeState(UPRItemInstance_Weapon* WeaponItem);

	// 무기 장착 변경 델리게이트를 반환한다
	FPRWeaponEquipmentChangedSignature& GetOnWeaponEquipmentChanged() { return OnWeaponEquipmentChanged; }
  
	// 활성 슬롯 무기 메시 애니메이션 상태 요청. 각 머신의 로컬 WeaponActor에서 실행
	void RequestWeaponAnimation(EPRWeaponAnimationState AnimationState);

	// 신뢰 멀티캐스트. 상태 전이가 누락되면 안 되는 케이스(Reload/Idle 복귀 등)에 사용
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RequestWeaponAnimation_Reliable(EPRWeaponAnimationState AnimationState);

	// 비신뢰 멀티캐스트. 빈도 높고 누락 허용되는 케이스에 사용
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_RequestWeaponAnimation_Unreliable(EPRWeaponAnimationState AnimationState);
	
protected:
	// 서버 권위에서 슬롯 원본과 공개 상태를 갱신한다
	bool EquipWeaponInternal(UPRItemInstance_Weapon* WeaponItem);

	// 서버 권위에서 슬롯 원본 제거와 공개 상태를 갱신한다
	bool UnequipWeaponFromSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 슬롯 Mod 장착 결과를 갱신한다
	bool AttachModToSlotInternal(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

	// 서버 권위에서 현재 무기 슬롯과 공개 상태를 갱신한다
	void SetCurrentWeaponSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 무장 상태와 공개 상태를 갱신한다
	void SetWeaponArmedStateInternal(EPRArmedState NewArmedState);

	// 대상 슬롯의 로컬 무기 Actor를 생성 또는 갱신한다
	void RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 두 슬롯의 로컬 무기 Actor를 현재 공개 상태 기준으로 갱신한다
	void RefreshAllWeaponActors();

	// 대상 슬롯의 현재 휴대 상태에 맞춰 부착 소켓을 최신화한다
	void RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType);

	// 새로 복제된 활성 무기 슬롯을 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_CurrentWeaponSlot(EPRWeaponSlotType OldCurrentWeaponSlot);

	// 새로 복제된 주무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_PrimaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo);

	// 새로 복제된 보조무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_SecondaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo);

	// 새로 복제된 무장 상태를 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_ArmedState(EPRArmedState OldArmedState);

	// 새로 복제된 주무기 원본 Item 상태를 기준으로 UI 갱신 신호를 발행한다
	UFUNCTION()
	void OnRep_PrimaryWeaponInstance();

	// 새로 복제된 보조무기 원본 Item 상태를 기준으로 UI 갱신 신호를 발행한다
	UFUNCTION()
	void OnRep_SecondaryWeaponInstance();

	// 클라이언트 Item 장착 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_EquipWeapon(UPRItemInstance_Weapon* WeaponItem);

	// 클라이언트 장비 해제 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_UnequipWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 슬롯 전환 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 무장 상태 변경 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SetWeaponArmedState(EPRArmedState NewArmedState);

	// 클라이언트 Mod 장착 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

protected:
	// 슬롯 원본에서 공개 비주얼 정보를 구성한다
	FPRWeaponVisualInfo BuildVisualInfo(EPRWeaponSlotType SlotType, const UPRItemInstance_Weapon* WeaponItem) const;

	// 현재 슬롯이 무장 상태인지 비무장 상태인지 계산한다
	EPRArmedState GetSlotWeaponArmedState(EPRWeaponSlotType SlotType) const;

	// 슬롯과 무장 상태에 맞는 부착 소켓 이름을 반환한다
	FName GetAttachTargetSocketName(EPRWeaponSlotType SlotType, EPRArmedState SlotArmedState) const;

	// 현재 원본 슬롯 데이터 기준으로 공개 비주얼 정보를 다시 구성한다
	void RefreshVisualInfosFromCurrentState();

	// 현재 장착 상태를 기준으로 다음 활성 슬롯을 결정한다
	EPRWeaponSlotType ResolveNextWeaponSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const;

	// 잘못된 슬롯 입력을 차단한다
	bool IsSupportedSlot(EPRWeaponSlotType SlotType) const;

	// 현재 활성 무기 데이터를 기준으로 캐릭터 애니메이션 레이어를 갱신한다
	void RefreshAnimLayer();

	// 무기 데이터의 탄약 최대값을 지속형 장착 GE로 적용하고 현재 탄약을 캐시 비율로 복원한다
	void ApplyEquipAmmoGE(const UPRWeaponDataAsset* WeaponData, UObject* SourceObject);

	// 현재 활성 무기의 전투 스탯을 지속형 GE SetByCaller 값으로 ASC에 적용한다
	void ApplyCurrentWeaponGE(UObject* SourceObject);

	// 대상 슬롯의 Mod 자원 최대값을 지속형 GE SetByCaller 값으로 ASC에 적용한다
	void ApplyEquipModGE(EPRWeaponSlotType SlotType, const UPRWeaponModDataAsset* ModData, UObject* SourceObject);

	// 대상 슬롯의 현재 탄약 값을 즉시 Override GE로 적용한다
	void ApplyOverrideAmmoGE(EPRWeaponSlotType SlotType, float MagazineAmmo, float ReserveAmmo, UObject* SourceObject) const;

	// 대상 슬롯의 현재 Mod 자원 값을 즉시 Override GE로 적용한다
	void ApplyOverrideModResourceGE(EPRWeaponSlotType SlotType, float ModGauge, float ModStack, UObject* SourceObject) const;

private:
	// 대상 슬롯 원본을 수정 가능한 참조로 반환한다
	TObjectPtr<UPRItemInstance_Weapon>& GetMutableWeaponInstanceBySlot(EPRWeaponSlotType SlotType);

	// 대상 슬롯의 현재 로컬 Actor를 반환한다
	APRWeaponActor* GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯 로컬 Actor를 수정 가능한 참조로 반환한다
	TObjectPtr<APRWeaponActor>& GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType);

	// 대상 슬롯의 현재 로컬 Actor를 안전하게 정리한다
	void DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 지정 무기 Item이 연결된 슬롯 타입을 반환한다
	EPRWeaponSlotType ResolveWeaponItemSlot(const UPRItemInstance_Weapon* WeaponItem) const;

	// 현재 슬롯 상태 태그를 갱신한다
	void UpdateCurrentWeaponSlotTags(EPRWeaponSlotType OldSlot, EPRWeaponSlotType NewSlot);

	// 현재 소유자의 PlayerState를 반환한다
	APRPlayerState* GetOwnerPlayerState() const;

	// 대상 슬롯의 지속형 장착 GE 핸들 묶음을 수정 가능한 참조로 반환한다
	FPREquipSlotEffectHandles& GetMutableEquipEffectHandlesBySlot(EPRWeaponSlotType SlotType);

	// 지속형 GameplayEffect 핸들을 제거하고 비운다
	void RemoveEquipEffectHandle(FActiveGameplayEffectHandle& Handle);

	// 대상 슬롯에 연결된 지속형 장착 GE를 모두 제거한다
	void RemoveSlotEquipEffects(EPRWeaponSlotType SlotType);

	// 모든 슬롯과 현재 무기 지속형 장착 GE를 제거한다
	void RemoveAllEquipEffects();

	// 대상 슬롯의 현재 탄약 비율을 PlayerState에 저장한다
	void CacheAmmoRatiosForSlot(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯의 현재 탄약을 PlayerState 캐시 비율로 Override GE를 통해 복원한다
	void RestoreAmmoFromCachedRatios(EPRWeaponSlotType SlotType, UObject* SourceObject) const;

	// 대상 슬롯의 현재 탄약 값을 Override GE로 비운다
	void ClearAmmoAttributesForSlot(EPRWeaponSlotType SlotType, UObject* SourceObject) const;

protected:
	// 현재 무장 상태
	UPROPERTY(ReplicatedUsing = OnRep_ArmedState, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;

	// 현재 활성 무기 슬롯
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponSlot, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	EPRWeaponSlotType CurrentWeaponSlot = EPRWeaponSlotType::None;

	// 주무기 슬롯 공개 비주얼 정보
	UPROPERTY(ReplicatedUsing = OnRep_PrimaryVisualInfo, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualInfo PrimaryVisualInfo;

	// 보조무기 슬롯 공개 비주얼 정보
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryVisualInfo, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualInfo SecondaryVisualInfo;

	// 주무기 슬롯에 연결된 무기 Item 원본
	UPROPERTY(ReplicatedUsing = OnRep_PrimaryWeaponInstance, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> PrimaryWeaponInstance;

	// 보조무기 슬롯에 연결된 무기 Item 원본
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeaponInstance, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> SecondaryWeaponInstance;

	// 현재 캐릭터 메시와 연결된 무기 애니메이션 레이어 클래스
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CurrentLinkedAnimLayerClass;

private:
	// 현재 빙의된 폰 소유자. 부착과 애니메이션에 사용
	TWeakObjectPtr<APRCharacterBase> CachedPawnOwner = nullptr;

	// 현재 머신에서만 유지하는 주무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> PrimaryWeaponActor;

	// 현재 머신에서만 유지하는 보조무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> SecondaryWeaponActor;

	// 현재 PlayerState에 연결된 ASC 캐시
	TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC = nullptr;

	// 현재 PlayerState에 연결된 무기 슬롯 자원 캐시
	TWeakObjectPtr<UPRAttributeSet_Weapon> CachedWeaponSet = nullptr;

	// 현재 PlayerState에 연결된 인벤토리 캐시
	TWeakObjectPtr<UPRInventoryComponent> CachedInventory = nullptr;

	// 주무기 슬롯 지속형 장착 GE 핸들
	FPREquipSlotEffectHandles PrimaryEquipEffectHandles;

	// 보조무기 슬롯 지속형 장착 GE 핸들
	FPREquipSlotEffectHandles SecondaryEquipEffectHandles;

	// 현재 활성 무기 전투 스탯 지속형 GE 핸들
	FActiveGameplayEffectHandle CurrentWeaponEffectHandle;

	// 무기 슬롯 장착 상태가 변경되었을 때 알린다
	FPRWeaponEquipmentChangedSignature OnWeaponEquipmentChanged;
};
