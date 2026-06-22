// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Fae 로열 아처 Character 구현)
#include "PRFaeRoyalArcherCharacter.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AI/Data/PRFaeRoyalArcherCombatDataAsset.h"

APRFaeRoyalArcherCharacter::APRFaeRoyalArcherCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 일치해야 한다.
	CharacterID = TEXT("Fae_RoyalArcher");

	ApplyRoyalArcherMovementDefaults();
}

void APRFaeRoyalArcherCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyRoyalArcherMovementDefaults();

	if (bStartInFlyingMode && IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}

	InitializePerchIdlePose();
}

void APRFaeRoyalArcherCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaeRoyalArcherCharacter, bUsePerchIdlePose);
}

void APRFaeRoyalArcherCharacter::InitializeEnemyBlackboard(UBlackboardComponent* BlackboardComponent) const
{
	Super::InitializeEnemyBlackboard(BlackboardComponent);

	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	if (ShouldWakeFromPerchKey != NAME_None
		&& BlackboardComponent->GetKeyID(ShouldWakeFromPerchKey) != FBlackboard::InvalidKey)
	{
		BlackboardComponent->SetValueAsBool(ShouldWakeFromPerchKey, bWakeFromPerchOnCombatStart);
	}

	if (HasPlayedWakeFromPerchKey != NAME_None
		&& BlackboardComponent->GetKeyID(HasPlayedWakeFromPerchKey) != FBlackboard::InvalidKey)
	{
		BlackboardComponent->SetValueAsBool(HasPlayedWakeFromPerchKey, false);
	}
}

UPRFaeRoyalArcherCombatDataAsset* APRFaeRoyalArcherCharacter::GetRoyalArcherCombatData() const
{
	return Cast<UPRFaeRoyalArcherCombatDataAsset>(GetCombatDataAsset());
}

void APRFaeRoyalArcherCharacter::PrepareWakeFromPerch()
{
	ClearPerchIdlePose();

	if (bStartInFlyingMode && IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
}

void APRFaeRoyalArcherCharacter::ClearPerchIdlePose()
{
	bUsePerchIdlePose = false;
}

void APRFaeRoyalArcherCharacter::ApplyRoyalArcherMovementDefaults()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	MovementComponent->DefaultLandMovementMode = MOVE_Flying;
	MovementComponent->MaxFlySpeed = DefaultMaxFlySpeed;
	MovementComponent->BrakingDecelerationFlying = DefaultBrakingDecelerationFlying;
	MovementComponent->RotationRate = FRotator(0.0f, DefaultFlightRotationYawRate, 0.0f);
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = true;
	bUseControllerRotationYaw = false;
}

void APRFaeRoyalArcherCharacter::InitializePerchIdlePose()
{
	bUsePerchIdlePose = bWakeFromPerchOnCombatStart && bStartInPerchIdlePose;
}
