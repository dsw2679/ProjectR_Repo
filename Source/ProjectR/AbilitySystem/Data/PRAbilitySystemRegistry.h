// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (어빌리티 레지스트리 초기 구축 및 무기/상태 데이터 캐싱 구현)
// Author: 이건주 (Mod 버프 관련 무기 데이터 매핑 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAbilitySystemRegistry.generated.h"

class UGameplayEffect;

// 영구 저장 대상 Attribute 목록
USTRUCT(BlueprintType)
struct FPRPersistentAttributeList
{
	GENERATED_BODY()

	// Base 값 저장 대상 Attribute 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Registry|Save")
	TArray<FGameplayAttribute> Attributes;
};

// 프로젝트 단일 인스턴스 (DA_AbilitySystemRegistry). 역할별 DT와 Attribute 매핑 보유
UCLASS(BlueprintType)
class PROJECTR_API UPRAbilitySystemRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*~ UPrimaryDataAsset Interface ~*/
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// PropertyToAttribute 조회. 누락 시 비유효 Attribute 반환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	FGameplayAttribute FindAttribute(FName PropertyName) const;

	// Role별 StatTable 소프트 레퍼런스를 LoadSynchronous로 해석
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	UDataTable* GetStatTableSynchronous(EPRCharacterRole Role) const;

	// Role별 영구 저장 대상 Attribute 목록
	const TArray<FGameplayAttribute>& GetPersistentBaseAttributes(EPRCharacterRole Role) const;

public:
	// 역할별 스탯 DataTable 약참조
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<EPRCharacterRole, TSoftObjectPtr<UDataTable>> StatTables;
	
	// Row의 FFloatProperty 이름 -> 대응 Attribute 매핑 (PrimaryAttributes)
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<FName, FGameplayAttribute> PropertyToAttribute;

	// Role별 영구 저장 대상 Base Attribute 목록
	UPROPERTY(EditAnywhere, Category = "Registry|Save")
	TMap<EPRCharacterRole, FPRPersistentAttributeList> PersistentBaseAttributes;
	
	// 초기화 GE
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSubclassOf<UGameplayEffect>  InitializeGE;

	// 플레이어 생존 수치 초기화 GE
	UPROPERTY(EditAnywhere, Category = "Registry|Player")
	TSubclassOf<UGameplayEffect> PlayerInitializeGE;

	// ==== 데미지 적용 GE ====
	UPROPERTY(EditAnywhere, Category = "Registry|DamageGE")
	TSubclassOf<UGameplayEffect>  DamageGE_FromEnemy;
	
	UPROPERTY(EditAnywhere, Category = "Registry|DamageGE")
	TSubclassOf<UGameplayEffect>  DamageGE_FromWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Registry|DamageGE")
	TSubclassOf<UGameplayEffect>  DamageGE_FromMod;
	
	// ==== 무기 장착 GE ====
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_CurrentWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_PrimaryWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_PrimaryMod;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_Override_PrimaryAmmo;

	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_Override_PrimaryModResource;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_SecondaryWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_SecondaryMod;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_Override_SecondaryAmmo;
	
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect>  EquipGE_Override_SecondaryModResource;

	// 장비 공통 수치 보너스 GE
	UPROPERTY(EditAnywhere, Category = "Registry|EquipGE")
	TSubclassOf<UGameplayEffect> EquipGE_EquipmentCommonStats;

	// ==== 쿨다운 GE ====
	// 사격 어빌리티 공통 쿨다운 GE. SetByCaller.Cooldown으로 Duration 주입. Cooldown.Ability.Fire.Primary를 Granted Tag로 부여
	UPROPERTY(EditAnywhere, Category = "Registry|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownGE_Fire;
	
	// ==== Mod Cost GE ====
	UPROPERTY(EditAnywhere, Category = "Registry|Cost")
	TSubclassOf<UGameplayEffect> ModGaugeGrantGE;
	
	UPROPERTY(EditAnywhere, Category = "Registry|Cost")
	TSubclassOf<UGameplayEffect> ModStackCostGE_Primary;
	
	UPROPERTY(EditAnywhere, Category = "Registry|Cost")
	TSubclassOf<UGameplayEffect> ModStackCostGE_Secondary;
};
