// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (성장 특성 포인트 관련 데이터 타입 정의)
// Author: 배유찬 (온라인 세션, 패스트 트래블 및 장비 슬롯 데이터 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "PRGameTypes.generated.h"

/*~ 그래픽 설정 ~*/

// 전체 Scalability 그룹에 적용할 그래픽 품질 단계
UENUM(BlueprintType)
enum class EPRGraphicsQualityProfile : uint8
{
	// 낮음 프로필
	Low = 0,
	// 보통 프로필
	Medium = 1,
	// 높음 프로필
	High = 2,
	// 에픽 프로필
	Epic = 3,
	// 시네마틱 프로필
	Cinematic = 4,
};

/*~ 성장 / 특성 ~*/

// 특성 투자 대상 능력치 종류
UENUM(BlueprintType)
enum class EPRTraitStatType : uint8
{
	// 최대 체력
	MaxHealth,
	// 방어력
	Armor,
	// 이동속도
	MovementSpeed,
	// 플레이어 공격력
	AttackPower,
	// 최대 스태미너
	MaxStamina,
	// 치명타 확률
	CriticalHitChance,
	// 치명타 피해 배율
	CriticalDamageMultiplier
};

namespace PRStaticTexts
{
	const FText MaxHealth = FText::FromString(TEXT("최대 체력"));
	const FText Armor = FText::FromString(TEXT("방어력"));
	const FText MovementSpeed =  FText::FromString(TEXT("이동 속도"));
	const FText AttackPower = FText::FromString(TEXT("공격력 보너스"));
	const FText MaxStamina = FText::FromString(TEXT("최대 스태미너"));
	const FText CriticalHitChance = FText::FromString(TEXT("치명타 확률"));
	const FText CriticalDamageMultiplier = FText::FromString(TEXT("치명타 피해"));
	const FText Unknown = FText::FromString(TEXT("알 수 없음"));

	static FText GetTraitDisplayText(const EPRTraitStatType InType)
	{
		switch (InType)
		{
		case EPRTraitStatType::MaxHealth:
			return MaxHealth;
		case EPRTraitStatType::Armor:
			return Armor;
		case EPRTraitStatType::MovementSpeed:
			return MovementSpeed;
		case EPRTraitStatType::AttackPower:
			return AttackPower;
		case EPRTraitStatType::MaxStamina:
			return MaxStamina;
		case EPRTraitStatType::CriticalHitChance:
			return CriticalHitChance;
		case EPRTraitStatType::CriticalDamageMultiplier:
			return CriticalDamageMultiplier;
		default:
			return Unknown;
		}
	}
}

// 능력치별 특성 투자 포인트 내역
USTRUCT(BlueprintType)
struct FPRTraitInvestmentInfo
{
	GENERATED_BODY()

public:
	// 최대 체력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth = 0;

	// 방어력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Armor = 0;

	// 이동속도 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MovementSpeed = 0;

	// 플레이어 공격력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackPower = 0;

	// 최대 스태미너 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStamina = 0;

	// 치명타 확률 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalHitChance = 0;

	// 치명타 피해 배율 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalDamageMultiplier = 0;
};

// 캐릭터 스탯 업그레이드 정보
USTRUCT(BlueprintType)
struct FPRCharacterStatUpgradeInfo
{
	GENERATED_BODY()

public:
	// 미사용 특성 포인트. 로드 시 참고값이며 최종값은 서버가 레벨과 투자 내역으로 재계산
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UnspentTraitPoint = 0;

	// 능력치별 특성 투자 내역
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRTraitInvestmentInfo TraitInvestment;
};

// 레벨별 누적 필요 경험치 Row
USTRUCT(BlueprintType)
struct FPRLevelExperienceRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 해당 레벨에 도달하기 위한 누적 경험치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 RequiredTotalExperience = 0;

	// 해당 레벨업으로 지급할 특성 포인트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 TraitPointReward = 2;
};

// 특성별 투자 공식 Row
USTRUCT(BlueprintType)
struct FPRTraitStatRuleRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 적용할 특성 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPRTraitStatType TraitType = EPRTraitStatType::MaxHealth;

	// 보너스를 적용할 대상 Attribute
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute TargetAttribute;

	// 포인트당 증가량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ValuePerPoint = 0.0f;

	// 최대 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 MaxPoint = 0;
};

/*~ 세이브 포맷 버전 ~*/

class UPRItemDataAsset;
class UPRConsumableDataAsset;
class UPRMaterialDataAsset;
class UPREquipmentDataAsset;
class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;
// 세이브 파일 포맷 버전. Join 시 호스트-게스트 간 호환성 체크에 사용
// 불일치 시 접속 거부. 상향은 마이그레이션 구현 후에만 허용
UENUM(BlueprintType)
enum class EPRSaveVersion : uint8
{
	// 미지정 (기본값)
	None = 0,
	// 최초 포맷
	V1   = 1,
};

/*~ 세션 상태 ~*/

// 세션 진행 단계. UI 로딩/에러 표시 분기용
UENUM(BlueprintType)
enum class EPRSessionState : uint8
{
	// 메뉴 상태(세션 없음)
	None,
	// 호스트 개시 진행 중(맵 로딩)
	Hosting,
	// 호스트 완료(인게임)
	Hosted,
	// 세션 검색 진행 중
	Finding,
	// 참가 진행 중(접속 시도)
	Joining,
	// 참가 완료(인게임)
	Joined,
	// 종료 처리 중
	Leaving,
};

// 세션 실패 사유 분류. UI 로컬라이즈 키 매핑에 사용
UENUM(BlueprintType)
enum class EPRSessionFailReason : uint8
{
	// 알 수 없음
	Unknown,
	// 주소 형식 오류
	InvalidAddress,
	// 네트워크 레이어 실패(연결 거부, 타임아웃 등)
	NetworkFailure,
	// 맵 로딩 실패
	MapLoadFailed,
	// 세이브 포맷 버전 불일치
	VersionMismatch,
	// 캐릭터 페이로드 검증 실패
	CharacterRejected,
	// 세션 검색 실패
	SessionSearchFailed,
	// 참가 가능한 세션 없음
	NoSessionFound,
};

/*~ 월드 오브젝트 상태 ~*/

// 문·게이트 등 환경 오브젝트의 상태 열거. WorldSave에 매핑되어 세션 동안 유지
UENUM(BlueprintType)
enum class EPRWorldObjectState : uint8
{
	// 초기 상태(잠김/닫힘)
	Default,
	// 열림/활성화됨
	Opened,
	// 제거됨(일회성 오브젝트)
	Consumed,
};

/*~ 인벤토리 / 장비 (추후 구현) ~*/

// [확장 및 이전 예정] 인벤토리 단일 슬롯 엔트리. Inventory 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPRItemSaveEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount = 0;
};

// [확장 및 이전 예정] 세이브/네트워크 전송용 인벤토리 직렬화 DTO. Inventory 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPRInventorySnapshot
{
	GENERATED_BODY()
};

// [확장 및 이전 예정] 장착 중인 무기/장비 슬롯 매핑. Equipment 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPREquipmentState
{
	GENERATED_BODY()
};

/*~ 캐릭터 스펙 ~*/

// 무기 인스턴스 고유 상태
USTRUCT(BlueprintType)
struct FPRWeaponInstanceState
{
	GENERATED_BODY()

	// 강화 단계
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EnhancementLevel = 0;
};

// Mod 인스턴스 고유 상태
USTRUCT(BlueprintType)
struct FPRModInstanceState
{
	GENERATED_BODY()

	// 강화 단계
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EnhancementLevel = 0;
};

// 무기 아이템 저장 엔트리
USTRUCT(BlueprintType)
struct FPRWeaponItemSaveEntry
{
	GENERATED_BODY()

	// 무기 데이터 소프트 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPRWeaponDataAsset> WeaponData;

	// 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackCount = 1;

	// 장착 Mod 인덱스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EquippedModIndex = INDEX_NONE;

	// 무기 인스턴스 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRWeaponInstanceState State;
};

// Mod 아이템 저장 엔트리
USTRUCT(BlueprintType)
struct FPRModItemSaveEntry
{
	GENERATED_BODY()

	// Mod 데이터 소프트 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPRWeaponModDataAsset> ModData;
	
	// Mod 인스턴스 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRModInstanceState State;
	
	// 모드 아이템의 ItemStack은 1로 고정임
};

// 소비 아이템 저장 엔트리
USTRUCT(BlueprintType)
struct FPRConsumableSaveEntry
{
	GENERATED_BODY()

	// 소비 아이템 데이터 소프트 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPRConsumableDataAsset> ConsumableData;

	// 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackCount = 0;
};

// 재료 아이템 저장 엔트리
USTRUCT(BlueprintType)
struct FPRMaterialSaveEntry
{
	GENERATED_BODY()

	// 재료 데이터 소프트 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPRMaterialDataAsset> MaterialData;

	// 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackCount = 0;
};

// 장비 아이템 저장 엔트리
USTRUCT(BlueprintType)
struct FPREquipmentItemSaveEntry
{
	GENERATED_BODY()

	// 장비 데이터 소프트 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UPREquipmentDataAsset> EquipmentData;

	// 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackCount = 1;
};

// 인벤토리 저장 데이터
USTRUCT(BlueprintType)
struct FPRInventorySaveData
{
	GENERATED_BODY()

	// 무기 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRWeaponItemSaveEntry> Weapons;

	// Mod 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRModItemSaveEntry> Mods;

	// 소비 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRConsumableSaveEntry> Consumables;

	// 재료 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRMaterialSaveEntry> Materials;

	// 장비 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPREquipmentItemSaveEntry> Equipments;
};

// 무기 매니저 저장 데이터
USTRUCT(BlueprintType)
struct FPRWeaponManagerSaveData
{
	GENERATED_BODY()

	// 주무기 인벤토리 인덱스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PrimaryWeaponIndex = INDEX_NONE;

	// 보조무기 인벤토리 인덱스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SecondaryWeaponIndex = INDEX_NONE;

	// 현재 무기 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRWeaponSlotType CurrentWeaponSlot = EPRWeaponSlotType::None;

	// 주무기 현재 탄창 탄약
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PrimaryMagazineAmmo = 0.0f;

	// 주무기 현재 예비 탄약
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PrimaryReserveAmmo = 0.0f;

	// 주무기 현재 Mod 게이지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PrimaryModGauge = 0.0f;

	// 주무기 현재 Mod 스택
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PrimaryModStack = 0.0f;

	// 보조무기 현재 탄창 탄약
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SecondaryMagazineAmmo = 0.0f;

	// 보조무기 현재 예비 탄약
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SecondaryReserveAmmo = 0.0f;

	// 보조무기 현재 Mod 게이지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SecondaryModGauge = 0.0f;

	// 보조무기 현재 Mod 스택
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SecondaryModStack = 0.0f;
};

// 퀵슬롯 저장 데이터
USTRUCT(BlueprintType)
struct FPRQuickSlotSaveData
{
	GENERATED_BODY()

	// 슬롯별 등록 소비 아이템 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSoftObjectPtr<UPRConsumableDataAsset>> RegisteredConsumables;
};

// 장비 슬롯 저장 엔트리
USTRUCT(BlueprintType)
struct FPREquipmentSlotSaveEntry
{
	GENERATED_BODY()

	// 장착 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	// 인벤토리 내 장비 아이템 인덱스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EquipmentItemIndex = INDEX_NONE;
};

// 비무기 장비 저장 데이터
USTRUCT(BlueprintType)
struct FPREquipmentSaveData
{
	GENERATED_BODY()

	// 슬롯별 장착 장비 인덱스 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPREquipmentSlotSaveEntry> EquippedSlots;
};

// 재화 저장 데이터
USTRUCT(BlueprintType)
struct FPRCurrencySaveData
{
	GENERATED_BODY()

	// 고철 보유량
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Scrap = 0;
};

// Attribute Base 저장 엔트리
USTRUCT(BlueprintType)
struct FPRAttributeBaseEntry
{
	GENERATED_BODY()

	// 저장 대상 Attribute
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute Attribute;

	// Base 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseValue = 0.0f;
};

// Attribute Base 스냅샷
USTRUCT(BlueprintType)
struct FPRAttributeBaseSnapshot
{
	GENERATED_BODY()

	// Attribute별 Base 값 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRAttributeBaseEntry> Entries;
};

// 게스트가 Join 시 호스트에 전송하는 캐릭터 스펙 번들
// 호스트는 수신 즉시 Validate -> PlayerState에 주입 후 복제
USTRUCT(BlueprintType)
struct FPRCharacterSaveData
{
	GENERATED_BODY()

	// 세이브 포맷 버전. 불일치 시 호스트가 접속 거부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRSaveVersion Version = EPRSaveVersion::V1;

	// 플레이어 표시명. 텍스트 최대 길이 제한(서버 검증)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	// 캐릭터 레벨. 서버는 현재 허용된 상한과 비교하여 초과 시 클램프
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	// 누적 경험치. Level과의 정합성도 함께 검증
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Experience = 0;

	// 스탯 배분 결과. 각 필드는 Level 기반 상한 검증 대상
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRCharacterStatUpgradeInfo Stats;

	// 인벤토리 저장 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRInventorySaveData Inventory;

	// 무기 매니저 저장 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRWeaponManagerSaveData WeaponManager;

	// 비무기 장비 저장 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPREquipmentSaveData Equipment;

	// 퀵슬롯 저장 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRQuickSlotSaveData QuickSlot;

	// 재화 저장 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRCurrencySaveData Currency;

	// Attribute Base 스냅샷
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRAttributeBaseSnapshot AttributeBaseSnapshot;
};

/*~ 월드 세이브 ~*/

// 월드와 Waypoint를 함께 식별하는 진행 상태 키
USTRUCT(BlueprintType)
struct FPRWaypointKey
{
	GENERATED_BODY()

public:
	// 에셋 경로와 무관한 논리 월드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "World"))
	FGameplayTag WorldId;

	// 목적지 맵의 SpawnPoint와 매칭되는 Waypoint 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "SpawnPoint"))
	FGameplayTag WaypointId;

	// 저장·복제·서버 요청에서 사용할 수 있는 완전한 키 여부
	bool IsValid() const
	{
		return WorldId.IsValid() && WaypointId.IsValid();
	}

	bool operator==(const FPRWaypointKey& Other) const
	{
		return WorldId == Other.WorldId && WaypointId == Other.WaypointId;
	}
};

// Travel 가능한 Waypoint 해금 기록
USTRUCT(BlueprintType)
struct FPRUnlockedWaypointEntry
{
	GENERATED_BODY()

public:
	// 해금된 Waypoint 목적지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRWaypointKey WaypointKey;

	bool operator==(const FPRUnlockedWaypointEntry& Other) const
	{
		return WaypointKey == Other.WaypointKey;
	}
};

// 월드와 SpawnPoint를 함께 식별하는 실제 스폰 위치 키
USTRUCT(BlueprintType)
struct FPRSpawnPointKey
{
	GENERATED_BODY()

public:
	// 에셋 경로와 무관한 논리 월드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "World"))
	FGameplayTag WorldId;

	// 실제 플레이어 소환에 사용한 SpawnPoint 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "SpawnPoint"))
	FGameplayTag SpawnPointId;

	// 저장·복제·시작 위치 복원에 사용할 수 있는 완전한 키 여부
	bool IsValid() const
	{
		return WorldId.IsValid() && SpawnPointId.IsValid();
	}

	bool operator==(const FPRSpawnPointKey& Other) const
	{
		return WorldId == Other.WorldId && SpawnPointId == Other.SpawnPointId;
	}
};

// 호스트 세이브에서 로드되어 GameState에 복제되는 월드 진행 상태
USTRUCT(BlueprintType)
struct FPRWorldSaveData
{
	GENERATED_BODY()

	// 세이브 포맷 버전
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRSaveVersion Version = EPRSaveVersion::V1;

	// Travel UI 빠른 이동에 표시할 마지막 방문 Waypoint
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRWaypointKey LastVisitedWaypoint;

	// 전멸 리스폰에 사용할 마지막 활성화 Waypoint
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRWaypointKey LastActivatedWaypoint;

	// 시작 메뉴에서 이어하기 시 사용할 마지막 실제 스폰 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRSpawnPointKey SavedSpawnPoint;

	// 활성화되어 Travel 가능한 Waypoint 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRUnlockedWaypointEntry> UnlockedWaypoints;

	// 처치 완료된 보스 ID 집합. 재도전 시 상태 초기화 분기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> DefeatedBosses;
};

/*~ 세션 연결 파라미터 ~*/

// Host 개시 시 GameInstance로 전달되는 파라미터
USTRUCT(BlueprintType)
struct FPRHostSessionParams
{
	GENERATED_BODY()

	// 호스팅할 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MapName;

	// 허용 최대 인원. GameMode의 MaxPlayers로 반영
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers = 3;

	// 비공개 여부. 현재는 표시용. OSS 단계에서 세션 검색 필터에 사용 예정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPrivate = false;
};

// Join 시도 시 전달되는 파라미터
USTRUCT(BlueprintType)
struct FPRJoinSessionParams
{
	GENERATED_BODY()

	// 접속 주소. 비어 있으면 OSS Null 검색 결과의 첫 번째 세션 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Address;
};

/*~ 보상 Grant ~*/

// 즉시 지급 단위. 보상 이벤트 발생 시 서버가 구성하여 오너 클라로 푸시
// 세션 종료 시 정산하는 배치 경로는 사용하지 않는다 (Logout은 Reliable RPC 도달 보장 X)
USTRUCT(BlueprintType)
struct FPRRewardGrant
{
	GENERATED_BODY()

	// 획득 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Experience = 0;

	// 획득 재화 (기본 통화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Currency = 0;

	// 아이템 드롭·재료 등은 Inventory 시스템 구현 시 필드 추가
};
