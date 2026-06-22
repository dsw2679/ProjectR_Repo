// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재화/아이템 드롭 처리 및 픽업 HUD 알림 연동 구현)
// Author: 배유찬 (드롭 확률 테이블 기반 아이템 스포너 배치 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "PRItemDropManagerSubsystem.generated.h"

class APRPlayerState;
class APRRewardPickupActor;
class UPRItemDataAsset;

// 탄약 지급 결과. 백분율 해석 후 실제 계산된 목표량과 지급량을 담는다
struct FPRAmmoGrantResult
{
	// MaxMagazineAmmo 기반으로 환산된 실제 획득 목표량
	int32 DesiredQuantity = 0;

	// 예비탄 한도 클램프 후 실제 지급된 수량
	int32 GrantedQuantity = 0;
};

// 아이템과 재화 드롭의 확정, 지급, 픽업 Claim을 서버 권위로 처리한다
UCLASS()
class PROJECTR_API UPRItemDropManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 몬스터 사망 컨텍스트를 기반으로 드롭 보상을 처리한다
	void HandleMonsterDied(const FPRMonsterDeathDropRequest& Request);

	// 월드 보상 픽업 Claim을 처리한다
	bool ClaimPickup(APRRewardPickupActor* PickupActor, AActor* Interactor);

	// 외부 시스템에서 확정 보상 픽업을 월드에 생성
	APRRewardPickupActor* SpawnResolvedRewardPickup(const FPRResolvedDropReward& Reward, const FVector& DropLocation, const AActor* IgnoredActor) const;

protected:
	// 드롭 Entry를 확정 보상으로 변환한다
	bool ResolveReward(const FPRDropRewardEntry& Entry, FPRResolvedDropReward& OutReward) const;

	// 확정 보상을 즉시 지급하거나 월드 픽업으로 생성한다
	void CommitResolvedReward(const FPRResolvedDropReward& Reward, const FPRMonsterDeathDropRequest& Request);

	// 몬스터 처치 경험치를 지급한다
	void GrantExperienceReward(const FPRMonsterDropTableRow& DropRow, const FPRMonsterDeathDropRequest& Request) const;

	// 확정 보상 픽업을 월드에 생성한다
	APRRewardPickupActor* SpawnRewardPickup(const FPRResolvedDropReward& Reward, const FVector& DropLocation, const AActor* IgnoredActor) const;

	// 드롭 위치 아래의 바닥을 찾아 픽업 스폰 위치를 만든다
	FVector ResolveRewardPickupSpawnLocation(const FVector& DropLocation, const AActor* IgnoredActor) const;

	// 분배 규칙에 맞는 지급 대상 목록을 채운다
	void ResolveRecipients(EPRRewardDistributionRule DistributionRule, AController* PersonalController, TArray<APRPlayerState*>& OutRecipients) const;

	// Interactor에서 플레이어 컨트롤러를 찾는다
	AController* ResolveInteractorController(AActor* Interactor) const;

	// 플레이어에게 확정 보상을 지급한다
	bool GrantRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const;

	// 플레이어 ASC의 예비탄에 탄약 보상을 지급한다
	bool GrantAmmoRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const;

	// 플레이어 ASC의 예비탄에 실제로 적립된 탄약 수량을 반환한다
	FPRAmmoGrantResult GrantAmmoRewardAmountToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const;

	// 지급에 성공한 드롭 보상 알림을 대상 클라이언트로 전달
	void NotifyPickupRewardGranted(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const;

private:
	// 이미 드롭 처리한 사망 몬스터 목록
	TSet<TWeakObjectPtr<AActor>> ProcessedDeadMonsters;

	// 이미 Claim 처리한 픽업 목록
	TSet<TWeakObjectPtr<APRRewardPickupActor>> ClaimedPickups;
};
