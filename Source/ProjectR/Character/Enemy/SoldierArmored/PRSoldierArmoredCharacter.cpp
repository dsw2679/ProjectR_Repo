// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Soldier Armored Character 구현)
#include "PRSoldierArmoredCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "ProjectR/AI/Components/PRSoldierArmoredDebugDrawComponent.h"

APRSoldierArmoredCharacter::APRSoldierArmoredCharacter()
{
	DebugDrawComponent = CreateDefaultSubobject<UPRSoldierArmoredDebugDrawComponent>(TEXT("DebugDrawComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// 스탯 Registry / DT_EnemyStats의 RowName과 맞아야 한다.
	CharacterID = TEXT("Soldier_Armored");

	GetCharacterMovement()->MaxWalkSpeed = 360.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
}
