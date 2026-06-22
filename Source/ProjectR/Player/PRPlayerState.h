// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (경험치/레벨업, 성장 특성 포인트 및 재화(골드) 데이터 관리 구현)
// Author: 배유찬 (리스폰 정보, 온라인 세션 및 패스트 트래블 상태 데이터 영속화)
// Author: 이건주 (카메라 감도 설정 및 인벤토리 장비 스탯 데이터 저장 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRPlayerState.generated.h"

class UPRFXNetworkComponent;
struct FPRInventoryChangeEventData;
enum class EPRInventoryChangeReason : uint8;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UPRAttributeSet_Growth;
class UPRAttributeSet_Player;
class UPRAttributeSet_Weapon;
class UPRInventoryComponent;
class UPREquipmentManagerComponent;
class UPRWeaponManagerComponent;
class UPRQuickSlotComponent;
class UPRCurrencyComponent;
class UPRPlayerGrowthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMouseSensitivityChanged, float, NewSensitivity);

// 표시명 변경 이벤트
DECLARE_MULTICAST_DELEGATE_OneParam(FPRDisplayNameChangedSignature, const FString&);

// 다운 대기 시간 HUD 공유 상태
USTRUCT(BlueprintType)
struct FPRPlayerDownTimerInfo
{
	GENERATED_BODY()

	// 다운 타이머 활성 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Survival")
	bool bIsActive = false;

	// 다운 대기 총 시간
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Survival")
	float DurationSeconds = 0.0f;

	// 다운 종료 서버 월드 시간
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Survival")
	float EndServerWorldTimeSeconds = 0.0f;
};

// 플레이어별 복제 데이터 소유. Player ASC + AttributeSet의 소유자
// Inventory/Equipment 컴포넌트는 각 시스템 구현 시 본 클래스에 부착 예정
UCLASS()
class PROJECTR_API APRPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APRPlayerState();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	
	/*~ APlayerState Interface ~*/
	virtual void CopyProperties(APlayerState* PlayerState) override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	// 초기 아이템 지급
	void GiveStartUpItems();
	
	// 프로젝트 ASC 타입으로 반환
	UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const { return AbilitySystemComponent; }

	// 공통 AttributeSet 조회
	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }

	// 플레이어 AttributeSet 조회
	UPRAttributeSet_Player* GetPlayerSet() const { return PlayerSet; }

	// 성장 AttributeSet 조회
	UPRAttributeSet_Growth* GetGrowthSet() const { return GrowthSet; }

	// 무기 자원 AttributeSet 조회
	UPRAttributeSet_Weapon* GetWeaponSet() const { return WeaponSet; }

	// 플레이어 인벤토리 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// 플레이어 장비 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Equipment")
	UPREquipmentManagerComponent* GetEquipmentManagerComponent() const { return EquipmentManagerComponent; }

	// 플레이어 무기 매니저 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const { return WeaponManagerComponent; }

	// 플레이어 퀵슬롯 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|QuickSlot")
	UPRQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

	// 플레이어 재화 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Currency")
	UPRCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }

	// 플레이어 성장 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	UPRPlayerGrowthComponent* GetGrowthComponent() const { return GrowthComponent; }

	// 지정 무기 슬롯의 캐시된 탄창과 예비탄 비율을 반환한다
	void GetCachedAmmoRatios(EPRWeaponSlotType SlotType, float& OutMagazineRatio, float& OutReserveRatio) const;

	// 지정 무기 슬롯의 탄창과 예비탄 비율을 캐시한다
	void SetCachedAmmoRatios(EPRWeaponSlotType SlotType, float MagazineRatio, float ReserveRatio);

	// 표시명 조회
	const FString& GetDisplayName() const { return DisplayName; }

	// 캐릭터 레벨 조회
	int32 GetCharacterLevel() const { return CharacterLevel; }

	// 캐릭터 업그레이드 정보 조회
	FPRCharacterStatUpgradeInfo GetStatUpgradeInfo() const { return StatUpgradeInfo; }

	// 플레이어 고정 인덱스 조회
	int32 GetPRPlayerIndex() const { return PRPlayerIndex; }

	// 플레이어 고정 인덱스 설정
	void SetPRPlayerIndex(int32 NewPlayerIndex);

	// 현재 플레이어가 전멸 판정에 포함되는 전투 참여자인지 확인한다.
	bool IsCombatParticipant() const;

	// 현재 플레이어가 다운 상태인지 확인한다.
	bool IsDown() const;

	// 현재 플레이어가 최종 사망 상태인지 확인한다.
	bool IsDead() const;

	// 현재 플레이어가 다운 또는 사망으로 전투에서 이탈했는지 확인한다.
	bool IsOutOfFight() const;

	// 자신을 제외한 전투 참여자 중 아직 전투 가능한 플레이어가 있는지 확인한다.
	bool HasFightCapableAllyExceptSelf() const;

	// 현재 플레이어 Pawn에게 생존 상태 전환 이벤트를 보낸다.
	void SendSurvivalGameplayEvent(const FGameplayTag& EventTag) const;

	// 다운 타이머 HUD 공유 상태 시작
	void StartDownTimerInfo(float DurationSeconds, float EndServerWorldTimeSeconds);

	// 다운 타이머 HUD 공유 상태 정리
	void ClearDownTimerInfo();

	// 다운 타이머 HUD 공유 상태 반환
	const FPRPlayerDownTimerInfo& GetDownTimerInfo() const { return DownTimerInfo; }

	// 서버 월드 시간 기준 남은 다운 시간 반환
	float GetDownRemainingSeconds(float ServerWorldTimeSeconds) const;

	// 서버 월드 시간 기준 남은 다운 비율 반환
	float GetDownRemainingPercent(float ServerWorldTimeSeconds) const;

	// 다운 타이머 HUD 공유 상태 활성 여부 확인
	bool IsDownTimerInfoActive() const { return DownTimerInfo.bIsActive; }
	
	// 리스폰 전 PlayerState와 소유 시스템의 런타임 상태 초기화
	void ResetState();

	// 리스폰 후 새 Pawn에서 저장 데이터 복원과 생존 수치 복구를 수행하도록 준비
	void PrepareStateForRespawn();
	
	// 현재 캐릭터 Pawn 기준 AbilitySet을 재부여한다
	void GrantCharacterAbilitySet(const UPRAbilitySet* InAbilitySet, UObject* InSourceObject = nullptr);
	
	// 저장 데이터 적용 대기 여부
	bool HasPendingSaveDataApply() const { return bPendingSaveDataApply; }

	// 서버가 캐릭터 페이로드를 이미 수락했는지 여부
	bool HasAcceptedCharacterPayload() const { return bCharacterPayloadAccepted; }

	// 서버 캐릭터 페이로드 수락 상태 기록
	void MarkCharacterPayloadAccepted();

	// 다음 PossessedBy 이후 리스폰 생존 수치 복구 필요 여부 소비
	bool ConsumePendingRespawnRecovery();

	// 현재 보관 중인 캐릭터 저장 데이터
	const FPRCharacterSaveData& GetCurrentSaveData() const { return CurrentSaveData; }
	
	// 기본 정보 적용 (맵 전환시 유지하기 위해)
	void InitializePrimaryInfoFromSaveData(const FPRCharacterSaveData& InSaveData);

	// 새 Pawn 준비 이후 저장 데이터 적용 예약
	void QueueSaveDataApply(const FPRCharacterSaveData& InSaveData, bool bMergeStartUpItems);

	// 예약된 저장 데이터 적용
	void ApplyPendingSaveData();

	void ApplySaveData(const FPRCharacterSaveData& InSaveData);
	FPRCharacterSaveData MakeSaveData() const;

	// 현재 런타임 상태를 맵 이동 복원용 저장 데이터로 보관
	void SnapshotCurrentSaveData();

	// 성장 Attribute 기준의 캐시 값을 기존 저장/표시 필드에 반영한다
	void SyncGrowthCache(int64 NewExperience, int32 NewLevel, const FPRCharacterStatUpgradeInfo& NewStats);
	
	// 마우스 감도
	float GetCameraSensitivity() const { return CameraSensitivity; }
	void SetCameraSensitivity(float Sensitivity);
	

protected:
	void BindAutoRegisterQuickSlotEvent();

	// 저장 데이터와 선택적 기본 지급 아이템 적용
	void ApplySaveDataInternal(const FPRCharacterSaveData& InSaveData);
	
	UFUNCTION()
	void OnInventoryChanged(UPRInventoryComponent* InInventory, const FPRInventoryChangeEventData& EventData);

	// 표시명 복제 콜백
	UFUNCTION()
	void OnRep_DisplayName();

	UFUNCTION()
	void OnRep_DownTimerInfo();
	
public:
	FOnMouseSensitivityChanged OnMouseSensitivityChanged;

	// 표시명 변경 이벤트
	FPRDisplayNameChangedSignature OnDisplayNameChanged;
	
	
protected:
	// ===== Configs ======
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Player")
	TArray<FPRItemSaveEntry> StartUpItems;
	
	// ===== Information =====
	// 표시명. 모든 클라에게 복제 (타 플레이어 HUD 표시)
	UPROPERTY(ReplicatedUsing = OnRep_DisplayName)
	FString DisplayName;

	// 캐릭터 레벨. 타 클라에도 표시될 수 있으므로 전체 복제
	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	// 캐릭터 누적 경험치. 게스트 보상 커밋 경로에서 누적
	UPROPERTY(Replicated)
	int64 Experience = 0;

	// 스탯 업그레이드 정보
	UPROPERTY(Replicated)
	FPRCharacterStatUpgradeInfo StatUpgradeInfo;

	// 맵 이동과 리스폰에서 PlayerStart 선택에 사용하는 고정 플레이어 인덱스
	UPROPERTY(Replicated)
	int32 PRPlayerIndex = INDEX_NONE;

	// 다운 대기 시간 HUD 공유 상태
	UPROPERTY(ReplicatedUsing = OnRep_DownTimerInfo, BlueprintReadOnly, Category = "ProjectR|Survival")
	FPRPlayerDownTimerInfo DownTimerInfo;
	
	// ===== Components =====
	// 플레이어 ASC. PlayerState에 부착
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	// 공통 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	// 플레이어 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Player> PlayerSet;

	// 무기 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Weapon> WeaponSet;

	// 성장 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Growth> GrowthSet;

	// 플레이어가 소유한 Item 인스턴스 정본 컨테이너
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Inventory")
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 플레이어 장비 상태 확장용 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Equipment")
	TObjectPtr<UPREquipmentManagerComponent> EquipmentManagerComponent;

	// 무기 장착과 관리를 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 플레이어 소비 아이템 퀵슬롯 상태 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRQuickSlotComponent> QuickSlotComponent;

	// 플레이어가 보유한 고철 재화 컨테이너
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Currency")
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;

	// 플레이어 성장 상태 컨테이너
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Growth")
	TObjectPtr<UPRPlayerGrowthComponent> GrowthComponent;

	// 주무기 슬롯 탄창 보존 비율
	float CachedPrimaryMagazineAmmoRatio = 1.0f;

	// 주무기 슬롯 예비탄 보존 비율
	float CachedPrimaryReserveAmmoRatio = 1.0f;

	// 보조무기 슬롯 탄창 보존 비율
	float CachedSecondaryMagazineAmmoRatio = 1.0f;

	// 보조무기 슬롯 예비탄 보존 비율
	float CachedSecondaryReserveAmmoRatio = 1.0f;

	// 현재 캐릭터 Pawn에서 부여한 AbilitySet 핸들
	FPRAbilitySetHandles CharacterAbilitySetHandles;
	
	/** 카메라 감도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Camera")
	float CameraSensitivity = 0.5f;
private:
	UPROPERTY()
	FPRCharacterSaveData CurrentSaveData;
	
	// PossessedBy 이후 적용할 저장 데이터 존재 여부
	bool bPendingSaveDataApply = false;
	//
	// // 예약 저장 데이터 적용 후 기본 지급 아이템 병합 여부
	// bool bPendingStartUpItemsMerge = false;

	// 클라이언트 로컬 세이브 재제출로 서버 최신 상태를 덮지 않기 위한 수락 여부
	bool bCharacterPayloadAccepted = false;

	// 리스폰 복원 SaveData 적용 이후 생존 수치 초기화 GE 재적용 여부
	bool bPendingRespawnRecovery = false;
};
