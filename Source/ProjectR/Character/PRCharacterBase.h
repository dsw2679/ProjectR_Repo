// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 오프셋 연동용 캐릭터 기본 처리)
// Author: 배유찬 (어빌리티 시스템 컴포넌트(ASC) 연동 및 캐릭터 기본 물리/이펙트 구조 구축)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/FX/Impact/PRImpactSurface.h"
#include "PRCharacterBase.generated.h"

struct FGameplayTag;
class UAnimInstance;
class UPRAbilitySet;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;

// 프로젝트 전체 캐릭터 베이스. IAbilitySystemInterface와 IPRCombatInterface를 구현한다.
// 플레이어는 PlayerState의 ASC를 위임, 적은 자기 컴포넌트를 반환 (파생 클래스에서 분기)
UCLASS(Abstract)
class PROJECTR_API APRCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPRCombatInterface, public IPRImpactSurface
{
	GENERATED_BODY()

public:
	APRCharacterBase();

	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaTime) override;

	/*~ ACharacter Interface ~*/
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*~ IPRCombatInterface ~*/
	virtual bool IsDead() const override;
	
	/*~ IPRImpactSurface ~*/
	virtual FPRImpactResult ResolveImpactSurface_Implementation(const FPRImpactContext& Context) const override;
	
public:
	// 프로젝트 ASC 타입으로 반환
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const;

	// 캐릭터 데이터 식별자 반환
	FName GetCharacterID() const { return CharacterID; }

	// 무기 미장착 상태에서 사용할 기본 애니메이션 레이어 클래스 반환
	TSubclassOf<UAnimInstance> GetDefaultAnimLayerClass() const { return DefaultAnimLayerClass; }

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/*~ APRCharacterBase Interface ~*/
	void BindTagChangeEvent();
	// ASC의 OwnedTags 스택 변경 이벤트 핸들러, TagExists가 true 면 태그 추가, false면 태그 제거
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists);
	
protected:
	UPROPERTY(EditAnywhere, Category = "PR Config|Character")
	FName CharacterID;
	
	UPROPERTY(EditAnywhere, Category = "PR Config|Character")
	FText CharacterDisplayName;

	UPROPERTY(EditAnywhere, Category = "PR Config|Ability")
	TObjectPtr<UPRAbilitySet> AbilitySet;

	// 무기 미장착 상태에서 적용할 기본 애니메이션 레이어 (맨손 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR Config|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR Config|VFX")
	FGameplayTag DefaultSurfaceTag;
};
