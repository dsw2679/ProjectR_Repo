// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Repeat Fire 구현)
#include "PRAT_RepeatFire.h"

#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Fire.h"

UPRAT_RepeatFire* UPRAT_RepeatFire::RepeatFire(UGameplayAbility* OwningAbility, float Interval,
                                                                 bool bFireImmediately)
{
	UPRAT_RepeatFire* Task = NewAbilityTask<UPRAT_RepeatFire>(OwningAbility);
	Task->Interval = Interval;
	Task->bFireImmediately = bFireImmediately;
	return Task; 
}

void UPRAT_RepeatFire::Activate()
{
	Super::Activate();
	
	if (bFireImmediately && ShouldBroadcastAbilityTaskDelegates())
	{
		OnPerform.Broadcast();
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UPRAT_RepeatFire::PerformFire, Interval, true);
	}
}

void UPRAT_RepeatFire::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

void UPRAT_RepeatFire::PerformFire()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPerform.Broadcast();
	}
}
