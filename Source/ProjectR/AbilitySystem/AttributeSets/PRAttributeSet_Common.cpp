// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (체력 기반 사망/다운 상태 전환 및 UI 연동 구현)
// Author: 배유찬 (데미지 계산식 적용 및 공용 속성 초기화)
// Author: 손승우 (적 공격 피해량 산정 공식 연동)
#include "PRAttributeSet_Common.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"

void UPRAttributeSet_Common::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, Health,                   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MaxHealth,                COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MovementSpeedMultiplier,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, Armor,                    COND_None, REPNOTIFY_Always);
}

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Common::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Health는 [0, MaxHealth] 범위로 클램프
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	// MovementSpeedMultiplier 및 Armor는 음수 금지
	else if (Attribute == GetMovementSpeedMultiplierAttribute()
		|| Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Common::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	bool bHealthChanged = false;

	// 메타 어트리뷰트 IncomingDamage 처리
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamageDone = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (LocalDamageDone > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamageDone, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);
			bHealthChanged = true;
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		bHealthChanged = true;
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		bHealthChanged = true;
	}

	// Health => 0 전이 시 State.Dead 부여 + OnDeath 발행
	if (bHealthChanged)
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const UPRAttributeSet_Player* PlayerSet = ASC->GetSet<UPRAttributeSet_Player>();
			if (IsValid(PlayerSet))
			{
				// TODO: 아래 회복 가능량 산출 로직이 명확하지 않고, AttribteSet보다 ExecCalc에서 처리함이 좋을 듯함
				UPRAttributeSet_Player* MutablePlayerSet = const_cast<UPRAttributeSet_Player*>(PlayerSet);
				const float RecoverableHealthMax = FMath::Max(GetMaxHealth() - GetHealth(), 0.0f);
				MutablePlayerSet->SetRecoverableHealth(FMath::Clamp(PlayerSet->GetRecoverableHealth(), 0.0f, RecoverableHealthMax));
			}
			
			const float NewHealth = GetHealth();
			if (NewHealth <= 0.0f)
			{
				if (!ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
					&& !ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down))
				{
					AActor* Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
					AActor* AvatarActor = ASC->GetAvatarActor();
					if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
					{
						// 아래 로직에서 Player가 아닌 경우 (Enemy인 경우) EventTag는 Event_Ability_Death가 됨
						const APRPlayerState* OwnerPlayerState = Cast<APRPlayerState>(ASC->GetOwnerActor());
						const FGameplayTag EventTag = IsValid(OwnerPlayerState) && OwnerPlayerState->HasFightCapableAllyExceptSelf()
							? PRGameplayTags::Event_Ability_Down
							: PRGameplayTags::Event_Ability_Death;

						FGameplayEventData Payload;
						Payload.EventTag = EventTag;
						Payload.Instigator = Instigator;
						Payload.Target = AvatarActor;
						Payload.ContextHandle = Data.EffectSpec.GetContext();

						// 사망 이벤트 전송
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
							AvatarActor,
							EventTag,
							Payload);
						// 캐릭터들은 Event_Ability_Death를 트리거로 하는 Dead 어빌리티가 부여되어 있어야함
					}
				}
			}
		}
	}
}

// =====  OnRep =====

void UPRAttributeSet_Common::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, Health, OldValue);
}

void UPRAttributeSet_Common::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, MaxHealth, OldValue);
}

void UPRAttributeSet_Common::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, MovementSpeedMultiplier, OldValue);
}

void UPRAttributeSet_Common::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, Armor, OldValue);
}
