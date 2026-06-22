// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM/사운드, 로딩 화면 및 성장/특성 밸런스 설정 정의)
// Author: 배유찬 (패스트 트래블, 핑/마커 및 VFX/어빌리티 설정 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProjectR/UI/WorldMarker/PRWorldMarkerTypes.h"
#include "PRDeveloperSettings.generated.h"

enum class EPRCrosshairType : uint8;
enum class EPRFloatingTextType : uint8;
class UPRAbilitySystemRegistry;
class UPRImpactRegistry;
class UPRFXRegistryDataAsset;
class UPRFloatingTextWidget;
class UGameplayEffect;
class UUserWidget;
class UDataTable;
class APRRewardPickupActor;
class UPRCrosshairConfig;
class UPRBGMRegistryDataAsset;
class UPRUISoundDataAsset;
class UPRMapPreloadDataAsset;
class UPRRuntimePreloadDataAsset;
class UWorld;
class UPRWorldRegistry;

// 플로팅 텍스트 타입별 위젯 클래스 및 색상 설정
USTRUCT(BlueprintType)
struct FPRFloatingTextStyleConfig
{
	GENERATED_BODY()

	// 사용할 위젯 클래스
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	TSoftClassPtr<UPRFloatingTextWidget> WidgetClass;

	// 텍스트 색상
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	FLinearColor Color = FLinearColor::White;

	// 레이어 Z-Order 값
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	int32 LayerZOrder = 0;
};

// 위젯 클래스가 로드된 결과. Getter 반환용
struct FPRFloatingTextStyle
{
	// 로드 완료된 위젯 클래스
	TSubclassOf<UPRFloatingTextWidget> WidgetClass;

	// 텍스트 색상
	FLinearColor Color = FLinearColor::White;

	// 레이어 Z-Order 값
	int32 LayerZOrder = 0;
};

UENUM(BlueprintType)
enum class EPRWorldMarkerPreset : uint8
{
	Default,
	Enemy,
	Interactable
};

// 프로젝트 전역 Registry 에셋 지정용 DeveloperSettings. 프로젝트 설정 > Game > ProjectR 에서 편집
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectR"))
class PROJECTR_API UPRDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/*~ UDeveloperSettings Interface ~*/
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// CrosshairType에 따른 CrosshairWidgetClass를 동기 로드 후 반환
	TSubclassOf<UUserWidget> GetCrosshairWidgetSync(EPRCrosshairType CrosshairType) const;

	// 기본 CrosshairConfig를 동기 로드 후 반환
	const UPRCrosshairConfig* GetDefaultCrosshairConfigSync() const;

	// 기본 UI 사운드 데이터 에셋을 동기 로드 후 반환
	const UPRUISoundDataAsset* GetDefaultUISoundDataSync() const;

	// 기본 BGM Registry 데이터 에셋을 동기 로드 후 반환
	const UPRBGMRegistryDataAsset* GetDefaultBGMRegistrySync() const;

	// 기본 World Registry 데이터 에셋을 동기 로드 후 반환
	const UPRWorldRegistry* GetWorldRegistrySync() const;

	// FloatingTextType에 따른 스타일(위젯 클래스 + 색상)을 동기 로드 후 반환
	FPRFloatingTextStyle GetFloatingTextStyleSync(EPRFloatingTextType TextType) const;

	// 타입에 맞는 WorldMarker 프리셋 반환
	FPRWorldMarkerVisualData GetWorldMarkerPreset(EPRWorldMarkerPreset InPresetType) const;

public:
	// AbilitySystem 전용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRAbilitySystemRegistry"))
	TSoftObjectPtr<UPRAbilitySystemRegistry> AbilitySystemRegistry;

	// FX 태그와 Cue 클래스를 연결하는 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRFXRegistryDataAsset"))
	TSoftObjectPtr<UPRFXRegistryDataAsset> FXRegistry;

	// 총기 Impact 태그별 Niagara와 Decal 재생 데이터 및 Physical Surface fallback 매핑을 보관하는 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRImpactRegistry"))
	TSoftObjectPtr<UPRImpactRegistry> ImpactRegistry;

	// WorldId 기반 월드 데이터 조회용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRWorldRegistry"))
	TSoftObjectPtr<UPRWorldRegistry> WorldRegistry;

	// 몬스터별 드롭 보상 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRMonsterDropTableRow"))
	TSoftObjectPtr<UDataTable> MonsterDropTable;

	// 레벨별 누적 필요 경험치 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRLevelExperienceRow"))
	TSoftObjectPtr<UDataTable> LevelExperienceTable;

	// 특성별 투자 공식 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRTraitStatRuleRow"))
	TSoftObjectPtr<UDataTable> TraitStatRuleTable;

	// 특성 보너스 적용에 사용할 에디터 작성 GameplayEffect
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> TraitBonusEffectClass;

	// 드롭 보상 픽업에 사용할 액터 클래스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRRewardPickupActor"))
	TSoftClassPtr<APRRewardPickupActor> RewardPickupActorClass;

	// EPRCrosshairType : CrosshairWidgetClass 매핑
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TMap<EPRCrosshairType, TSoftClassPtr<UUserWidget>> CrosshairWidgets;

	// 무기 데이터에 크로스헤어 설정이 없을 때 사용할 기본 설정
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TSoftObjectPtr<UPRCrosshairConfig> DefaultCrosshairConfig;

	// 모든 PR 위젯 버튼에 자동 적용할 기본 UI 사운드 설정
	UPROPERTY(EditAnywhere, Config, Category = "UI|Sound", meta = (AllowedClasses = "/Script/ProjectR.PRUISoundDataAsset"))
	TSoftObjectPtr<UPRUISoundDataAsset> DefaultUISoundData;

	// 레벨별 BGM Entry를 보관하는 기본 Registry 설정
	UPROPERTY(EditAnywhere, Config, Category = "Audio|BGM", meta = (AllowedClasses = "/Script/ProjectR.PRBGMRegistryDataAsset"))
	TSoftObjectPtr<UPRBGMRegistryDataAsset> DefaultBGMRegistry;

	// EPRFloatingTextType : 스타일(위젯 + 색상) 매핑
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TMap<EPRFloatingTextType, FPRFloatingTextStyleConfig> FloatingTextStyles;

	// 기본 월드 마커 스타일
	UPROPERTY(EditDefaultsOnly, Config,  Category = "UI|WorldMarker")
	TMap<EPRWorldMarkerPreset, FPRWorldMarkerVisualData> WorldMarkerPresets;

	// 기본 월드 마커 유지 시간
	UPROPERTY(EditDefaultsOnly, Config,  Category = "UI|WorldMarker", meta = (ClampMin = "0.0"))
	float DefaultWorldMarkerDuration = 12.0f;

	// 거리 텍스트 표시 시작 거리
	UPROPERTY(EditDefaultsOnly, Config, Category = "UI|WorldMarker", meta = (ClampMin = "0.0", Units = "m"))
	float WorldMarkerDistanceVisibleMinMeters = 20.0f;

	// 로딩 화면 시스템 사용 여부
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen")
	bool bEnableLoadingScreenSystem = true;

	// 새 월드 생성 후 Viewport에 표시할 로딩 위젯
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen")
	TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass;

	// 최소 로딩 화면 표시 시간
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen", meta = (ClampMin = "0.0"))
	float MinimumLoadingScreenSeconds = 0.25f;

	// Required 큐 기본 타임아웃
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen", meta = (ClampMin = "0.0"))
	float RequiredPreloadTimeoutSeconds = 15.0f;

	// Soft Gate 기본 타임아웃
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen", meta = (ClampMin = "0.0"))
	float SoftPreloadTimeoutSeconds = 2.0f;

	// 맵별 프리로드 데이터 매핑
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen")
	TMap<TSoftObjectPtr<UWorld>, TSoftObjectPtr<UPRMapPreloadDataAsset>> MapPreloadDataAssets;

	// 플레이어와 전역 런타임 공통 프리로드 데이터
	UPROPERTY(EditDefaultsOnly, Config, Category = "LoadingScreen")
	TSoftObjectPtr<UPRRuntimePreloadDataAsset> RuntimePreloadDataAsset;
};
