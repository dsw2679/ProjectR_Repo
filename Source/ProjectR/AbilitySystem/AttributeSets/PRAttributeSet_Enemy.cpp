// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 관련 피해 배율 속성 구현)
// Author: 배유찬 (데미지 파이프라인 기반 적 속성 연동)
// Author: 손승우 (몬스터 그로기 임계치 및 상태 속성 구현)
#include "PRAttributeSet_Enemy.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Enemy::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetGroggyGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGroggyGauge());
	}
	else if (Attribute == GetMaxGroggyGaugeAttribute() || Attribute == GetAttackPowerAttribute())
	{
		// 음수 금지
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Enemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// GroggyGauge Max 도달 시 State.Groggy 토글 + OnGroggyStateChanged 발행
	if (Data.EvaluatedData.Attribute == GetGroggyGaugeAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (IsValid(ASC))
		{
			const FGameplayTag GroggyTag = PRGameplayTags::State_Groggy;
			const bool bHasTag = ASC->HasMatchingGameplayTag(GroggyTag);
			const float Current = GetGroggyGauge();
			AActor* AvatarActor = ASC->GetAvatarActor();
			const APREnemyBaseCharacter* EnemyAvatar = Cast<APREnemyBaseCharacter>(AvatarActor);
			const bool bCanEnterGroggyState = !IsValid(EnemyAvatar) || EnemyAvatar->CanEnterGroggyState();

			if (!bCanEnterGroggyState)
			{
				if (bHasTag)
				{
					ASC->RemoveLooseGameplayTag(GroggyTag);
					ASC->RemoveReplicatedLooseGameplayTag(GroggyTag);
					OnGroggyStateChanged.Broadcast(false);
				}
				return;
			}

			if (Current <= 0.0f && !bHasTag)
			{
				ASC->AddLooseGameplayTag(GroggyTag);
				ASC->AddReplicatedLooseGameplayTag(GroggyTag);

				AActor* Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
				if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
				{
					FGameplayEventData Payload;
					Payload.EventTag = PRGameplayTags::Event_Ability_GroggyEntered;
					Payload.Instigator = Instigator;
					Payload.Target = AvatarActor;

					// 그로기 진입 판정이 확정된 서버 지점에서 Ability 트리거 이벤트를 직접 발행한다.
					UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
						AvatarActor,
						PRGameplayTags::Event_Ability_GroggyEntered,
						Payload);
				}

				OnGroggyStateChanged.Broadcast(true);
			}
			else if (Current >= GetMaxGroggyGauge() && bHasTag)
			{
				ASC->RemoveLooseGameplayTag(GroggyTag);
				ASC->RemoveReplicatedLooseGameplayTag(GroggyTag);
				OnGroggyStateChanged.Broadcast(false);
			}
		}
	}
}

void UPRAttributeSet_Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, GroggyGauge,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, MaxGroggyGauge,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, AttackPower,         COND_None, REPNOTIFY_Always);

}

// =====  OnRep =====

void UPRAttributeSet_Enemy::OnRep_GroggyGauge(const FGameplayAttributeData& OldValue)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, GroggyGauge, OldValue); }
void UPRAttributeSet_Enemy::OnRep_MaxGroggyGauge(const FGameplayAttributeData& OldValue)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, MaxGroggyGauge, OldValue); }
void UPRAttributeSet_Enemy::OnRep_AttackPower(const FGameplayAttributeData& OldValue)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, AttackPower, OldValue); }
