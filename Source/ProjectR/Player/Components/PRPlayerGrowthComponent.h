// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (경험치 획득 기반 레벨업 처리 및 특성 투자 시스템 구현)
// Author: 배유찬 (사망 리스폰 시 경험치 페널티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerGrowthComponent.generated.h"

class UDataTable;
class UGameplayEffect;
class UPRAbilitySystemComponent;

UENUM(BlueprintType)
enum class EPRTraitInvestmentFailReason : uint8
{
	None,
	InvalidPlayerState,
	InvalidAbilitySystem,
	InvalidGrowthComponent,
	InvalidDataTable,
	NegativePoint,
	ExceedTraitMaxPoint,
	ExceedTotalPoint,
	InvalidEffect
};

USTRUCT(BlueprintType)
struct FPRTraitInvestmentResult
{
	GENERATED_BODY()

public:
	// 요청 성공 여부
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	// 실패 사유
	UPROPERTY(BlueprintReadOnly)
	EPRTraitInvestmentFailReason FailReason = EPRTraitInvestmentFailReason::None;

	// 적용 후 투자 내역
	UPROPERTY(BlueprintReadOnly)
	FPRTraitInvestmentInfo InvestmentInfo;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRTraitInvestmentChangedSignature, const FPRTraitInvestmentInfo&, InvestmentInfo);

// 플레이어 경험치, 레벨, 특성 투자 상태를 서버 권위로 관리한다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRPlayerGrowthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRPlayerGrowthComponent();

	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버에서 경험치를 지급하고 레벨과 특성 포인트를 보정한다
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Growth")
	bool AddExperience(int32 Amount);

	// 서버에서 원하는 투자 내역을 검증하고 적용한다
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Growth")
	FPRTraitInvestmentResult ConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 서버에서 모든 투자 포인트를 초기화한다
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Growth")
	FPRTraitInvestmentResult ResetTraitInvestment();

	// 현재 투자 내역을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	const FPRTraitInvestmentInfo& GetTraitInvestmentInfo() const { return TraitInvestmentInfo; }

	// 특성 종류에 해당하는 투자 규칙을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	bool GetTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const;

	// 성장 저장 데이터를 적용한다
	void ApplyGrowthSaveData(int64 SavedExperience, int32 SavedLevel, const FPRCharacterStatUpgradeInfo& SavedStats);

	// 성장 저장 데이터를 만든다
	FPRCharacterStatUpgradeInfo MakeGrowthSaveData() const;

	// 리스폰 전 성장 보너스 GameplayEffect 재동기화
	void ResetSystem();

protected:
	UFUNCTION()
	void OnRep_TraitInvestmentInfo();

protected:
	// 레벨별 누적 필요 경험치 테이블
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Growth")
	TObjectPtr<UDataTable> LevelExperienceTable;

	// 특성별 투자 공식 테이블
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Growth")
	TObjectPtr<UDataTable> TraitStatRuleTable;

	// 에디터에서 작성한 특성 보너스 GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Growth")
	TSubclassOf<UGameplayEffect> TraitBonusEffectClass;

	// 현재 능력치별 투자 내역
	UPROPERTY(ReplicatedUsing = OnRep_TraitInvestmentInfo)
	FPRTraitInvestmentInfo TraitInvestmentInfo;

public:
	// 특성 투자 내역이 변경될 때 발행한다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Growth")
	FPRTraitInvestmentChangedSignature OnTraitInvestmentChanged;

private:
	UPRAbilitySystemComponent* GetPRASC() const;
	UDataTable* ResolveLevelExperienceTable() const;
	UDataTable* ResolveTraitStatRuleTable() const;
	TSubclassOf<UGameplayEffect> ResolveTraitBonusEffectClass() const;
	int32 ResolveLevelFromExperience(int64 InExperience) const;
	int32 GetTotalEarnedPoint(int32 InLevel) const;
	int32 GetInvestmentPoint(const FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType) const;
	void SetInvestmentPoint(FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType, int32 Point) const;
	int32 GetTotalSpentPoint(const FPRTraitInvestmentInfo& InvestmentInfo) const;
	bool ValidateInvestment(const FPRTraitInvestmentInfo& DesiredInvestment, EPRTraitInvestmentFailReason& OutFailReason) const;
	void ApplyGrowthAttributes(int64 NewExperience, int32 NewLevel, const FPRTraitInvestmentInfo& NewInvestment);
	void ApplyTraitBonusEffect();
	void RemoveTraitBonusEffect();
	void ClampCurrentVitals(float OldMaxHealth, float OldMaxStamina);
	void BuildTraitBonusMap(TMap<FGameplayTag, float>& OutBonusMap) const;
	bool FindTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const;

private:
	// 현재 적용 중인 특성 보너스 GameplayEffect 핸들
	FActiveGameplayEffectHandle TraitBonusEffectHandle;
};
