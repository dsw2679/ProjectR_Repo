// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (전투 인터페이스 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRCombatInterface.generated.h"

// 전투 진영 구분. 우호/적대 판정과 데미지 파이프라인 분기에 사용한다.
UENUM(BlueprintType)
enum class EPRTeam : uint8
{
	Neutral	UMETA(DisplayName = "Neutral"),
	Player	UMETA(DisplayName = "Player"),
	Enemy	UMETA(DisplayName = "Enemy")
};

// 데미지 적용 직후 수신자에게 전달되는 컨텍스트. 페이즈 전환·다운/소생 같은 후처리 입력값
USTRUCT(BlueprintType)
struct PROJECTR_API FPRDamageAppliedContext
{
	GENERATED_BODY()

	// 체력에서 차감된 최종 피해량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	float FinalDamage = 0.0f;

	// 그로기 게이지에서 차감된 최종 피해량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	float FinalGroggyDamage = 0.0f;

	// 치명타 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	bool bIsCritical = false;

	// 데미지 적용 직전 체력
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	float HealthBeforeDamage = 0.0f;

	// 데미지 적용 직후 체력 (예측값)
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	float HealthAfterDamage = 0.0f;

	// 최대 체력
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	float MaxHealth = 0.0f;

	// 적중 부위 정보
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	FPRDamageRegionInfo Region;

	// Effect Source
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	TWeakObjectPtr<UObject> SourceObject;

	// 피해를 발생시킨 캐릭터 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	FName SourceCharacterID = NAME_None;
	
	// 공격을 발생시킨 액터 (소스 액터, 플레이어 어빌리티에서 발생한 경우 PlayerState가 되는 경우가 많음)
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	TWeakObjectPtr<AActor> Instigator;
	
	// 공격자 Controller
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	TWeakObjectPtr<APlayerController> InstigatorController;
	
	// 피격 지점 정보
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	FHitResult HitResult;
	
	// Effect의 AllTags
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	FGameplayTagContainer EffectTags;
};

// IAbilitySystemInterface 같은 UInterface 패턴
UINTERFACE(MinimalAPI, Blueprintable)
class UPRCombatInterface : public UInterface
{
	GENERATED_BODY()
};

// 전투 참여자가 구현하는 공통 인터페이스. 진영 식별, 부위 정보 조회, 데미지 후처리 진입점
class PROJECTR_API IPRCombatInterface
{
	GENERATED_BODY()

public:
	// 자신의 진영을 반환한다. 우호/적대 판정에 사용
	virtual EPRTeam GetTeam() const { return EPRTeam::Neutral; }

	// 피격 본 이름으로 자기 부위 정보를 반환한다. 매핑이 없으면 Default Region 반환
	virtual FPRDamageRegionInfo GetDamageRegionInfo(FName BoneName) const { return FPRDamageRegionInfo(); }

	// 데미지 적용 직후 호출. 페이즈 전환·그로기 임계·다운 흐름 진입 등 종류별 후처리 자리
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) {}
	
	// 사망 여부
	virtual bool IsDead() const {return false;}
};
