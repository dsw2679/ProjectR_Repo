// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Soldier Armored Character 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRSoldierArmoredCharacter.generated.h"

class UPRSoldierArmoredDebugDrawComponent;
class UMotionWarpingComponent;

// Fae Soldier Armored 전용 캐릭터 클래스다.
// 공통 EnemyBase 위에 CharacterID, 이동 속도, MotionWarping, 디버그 표현을 지정한다.
UCLASS()
class PROJECTR_API APRSoldierArmoredCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRSoldierArmoredCharacter();

protected:
	// Soldier_Armored PIE debug range draw
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRSoldierArmoredDebugDrawComponent> DebugDrawComponent;

	// 공격 루트모션 방향 보정용 Motion Warping
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};
