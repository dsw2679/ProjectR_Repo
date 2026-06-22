// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Character Event Router 컴포넌트 구현)
#include "PRFaerinCharacterEventRouterComponent.h"

#include "GameFramework/Actor.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinWeaponVisualComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinEventRouter, Log, All);

namespace
{
	const FName ShowSwordEventName = TEXT("ShowSword");
	const FName ShowShardsEventName = TEXT("ShowShards");

	UPRFaerinWeaponVisualComponent* FindWeaponVisualComponentByName(AActor* OwnerActor, const FName ComponentName)
	{
		if (!IsValid(OwnerActor) || ComponentName == NAME_None)
		{
			return nullptr;
		}

		TArray<UPRFaerinWeaponVisualComponent*> Components;
		OwnerActor->GetComponents<UPRFaerinWeaponVisualComponent>(Components);
		for (UPRFaerinWeaponVisualComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}

		return nullptr;
	}
}

UPRFaerinCharacterEventRouterComponent::UPRFaerinCharacterEventRouterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRFaerinCharacterEventRouterComponent::OnRegister()
{
	Super::OnRegister();
	ResolveWeaponVisualComponent();
}

void UPRFaerinCharacterEventRouterComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveWeaponVisualComponent();
	LogMissingWeaponVisualComponent();
}

UPRFaerinWeaponVisualComponent* UPRFaerinCharacterEventRouterComponent::ResolveWeaponVisualComponent()
{
	if (IsValid(WeaponVisualComponent))
	{
		return WeaponVisualComponent;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return nullptr;
	}

	WeaponVisualComponent = FindWeaponVisualComponentByName(OwnerActor, WeaponVisualComponentName);
	if (IsValid(WeaponVisualComponent))
	{
		return WeaponVisualComponent;
	}

	WeaponVisualComponent = OwnerActor->FindComponentByClass<UPRFaerinWeaponVisualComponent>();
	return WeaponVisualComponent;
}

void UPRFaerinCharacterEventRouterComponent::LogMissingWeaponVisualComponent() const
{
	if (!bWarnIfWeaponVisualMissing || IsValid(WeaponVisualComponent))
	{
		return;
	}

	UE_LOG(LogPRFaerinEventRouter, Warning,
		TEXT("Faerin event router cannot resolve weapon visual component. Owner=%s, ExpectedName=%s"),
		*GetNameSafe(GetOwner()),
		*WeaponVisualComponentName.ToString());
}

void UPRFaerinCharacterEventRouterComponent::RegisterFaerinEventListener(
	UObject* Listener,
	FFaerinCharacterEventSignature::FDelegate Delegate)
{
	if (!IsValid(Listener) || !Delegate.IsBound())
	{
		return;
	}

	OnFaerinCharacterEvent.RemoveAll(Listener);
	OnFaerinCharacterEvent.Add(Delegate);
}

void UPRFaerinCharacterEventRouterComponent::UnregisterFaerinEventListener(UObject* Listener)
{
	if (!IsValid(Listener))
	{
		return;
	}

	OnFaerinCharacterEvent.RemoveAll(Listener);
}

void UPRFaerinCharacterEventRouterComponent::RouteFaerinCharacterEvent(FName EventName)
{
	if (EventName == NAME_None)
	{
		return;
	}

	UPRFaerinWeaponVisualComponent* ResolvedWeaponVisualComponent = ResolveWeaponVisualComponent();

	if (bLogRoutedEvents)
	{
		UE_LOG(LogPRFaerinEventRouter, Verbose,
			TEXT("Faerin character event routed. Owner=%s, Event=%s, WeaponVisual=%s"),
			*GetNameSafe(GetOwner()),
			*EventName.ToString(),
			*GetNameSafe(ResolvedWeaponVisualComponent));
	}

	if ((EventName == ShowSwordEventName || EventName == ShowShardsEventName)
		&& !IsValid(ResolvedWeaponVisualComponent))
	{
		UE_LOG(LogPRFaerinEventRouter, Warning,
			TEXT("Faerin weapon visual event ignored because weapon visual component is missing. Owner=%s, Event=%s"),
			*GetNameSafe(GetOwner()),
			*EventName.ToString());
	}

	if (EventName == ShowSwordEventName && IsValid(ResolvedWeaponVisualComponent))
	{
		ResolvedWeaponVisualComponent->ShowSword();
	}
	else if (EventName == ShowShardsEventName && IsValid(ResolvedWeaponVisualComponent))
	{
		ResolvedWeaponVisualComponent->ShowShards();
	}

	OnFaerinCharacterEvent.Broadcast(EventName);
}
