// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 오프셋 연동용 캐릭터 기본 처리)
// Author: 배유찬 (어빌리티 시스템 컴포넌트(ASC) 연동 및 캐릭터 기본 물리/이펙트 구조 구축)
#include "PRCharacterBase.h"

#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"

APRCharacterBase::APRCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultSurfaceTag = FGameplayTag::RequestGameplayTag(TEXT("FX.Impact.Flesh"));
}

// =====  AActor Interface =====

void APRCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void APRCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	// 바인딩한 태그 변경 이벤트 제거
	if (UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		ASC->OnGameplayTagUpdated.RemoveAll(this);
	}
}

void APRCharacterBase::BindTagChangeEvent()
{
	UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent();
	if (!ensureMsgf(ASC, TEXT("BindTagChangeEvent 타이밍 에러: ASC가 아직 존재하지 않음")))
	{
		return;
	}
	
	ASC->OnGameplayTagUpdated.AddUObject(this, &ThisClass::HandleGameplayTagUpdated);
	
	// 이미 가지고 있던 태그가 있을 수 있으므로 명시적 호출
	FGameplayTagContainer OwnedGameplayTags;
	ASC->GetOwnedGameplayTags(OwnedGameplayTags);
	
	for (const FGameplayTag& Tag : OwnedGameplayTags)
	{
		HandleGameplayTagUpdated(Tag, true);
	}
}

void APRCharacterBase::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	
}

void APRCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// =====  ACharacter Interface =====

void APRCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// =====  IAbilitySystemInterface =====

UAbilitySystemComponent* APRCharacterBase::GetAbilitySystemComponent() const
{
	return GetPRAbilitySystemComponent();
}

bool APRCharacterBase::IsDead() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
	}
	return false;
}

FPRImpactResult APRCharacterBase::ResolveImpactSurface_Implementation(const FPRImpactContext& Context) const
{
	FPRImpactResult Result;
	Result.bHasImpactTag = true;
	Result.ImpactTag = DefaultSurfaceTag;
	return Result;
}

UPRAbilitySystemComponent* APRCharacterBase::GetPRAbilitySystemComponent() const
{
	// 기본 구현은 nullptr. 플레이어는 PlayerState, 적은 자기 컴포넌트에서 override
	return nullptr;
}
