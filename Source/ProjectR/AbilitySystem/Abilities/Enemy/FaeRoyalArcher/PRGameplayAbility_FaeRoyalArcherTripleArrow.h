// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 Triple Arrow 공격 패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/FaeRoyalArcher/PRGameplayAbility_FaeRoyalArcherShoot.h"
#include "PRGameplayAbility_FaeRoyalArcherTripleArrow.generated.h"

// Fae Royal Archer의 3연속 화살 사격 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaeRoyalArcherTripleArrow : public UPRGameplayAbility_FaeRoyalArcherShoot
{
	GENERATED_BODY()

public:
	// RoyalArcher TripleArrow 태그와 3발 발사 기본값을 초기화한다.
	UPRGameplayAbility_FaeRoyalArcherTripleArrow();

protected:
	/*~ UPRGameplayAbility_EnemyProjectileAttack Interface ~*/

	// 한 번의 발사 이벤트에서 생성할 투사체 수를 반환한다.
	virtual int32 GetProjectileFireCount() const override;

	// ProjectileSocket_01/02/03 또는 fallback 오프셋으로 각 화살 스폰 위치를 계산한다.
	virtual FTransform GetProjectileSpawnTransformForIndex(int32 ProjectileIndex) const override;

	// 각 화살에 인덱스별 조준 보정 각도를 적용한다.
	virtual FVector CalculateProjectileAimDirectionForIndex(const FVector& SpawnLocation, int32 ProjectileIndex) const override;

	// ===== TripleArrow Socket =====

	// Avatar에 달린 SceneComponent 또는 Mesh Socket에서 발사 기준 Transform을 찾는다.
	bool TryResolveTripleArrowSocketTransform(FName SocketName, FTransform& OutTransform) const;

protected:
	// 3연사에 사용할 소켓 또는 SceneComponent 이름 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|TripleArrow")
	TArray<FName> TripleArrowSocketNames;

	// 소켓이 없을 때 기본 발사 Transform 기준으로 더할 로컬 오프셋 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|TripleArrow")
	TArray<FVector> TripleArrowFallbackLocalOffsets;

	// 각 화살의 조준 방향에 더할 보정 회전 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Projectile|TripleArrow")
	TArray<FRotator> TripleArrowAimRotationOffsets;
};
