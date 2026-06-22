// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (반사 궤적 예측 시뮬레이션 및 비행 경로 동기화 연산 구현)
// Author: 이건주 (서포트 드론 유도 비행 궤적 연산 구현)
#include "PRProjectileMovementComponent.h"

#include "PRProjectileBase.h"
#include "GameFramework/Character.h"


UPRProjectileMovementComponent::UPRProjectileMovementComponent()
{
	// 초기 속도 (cm/s). 100m/s = 유탄/로켓 수준. 히트스캔 대체 고속탄은 파생 BP에서 상향 조정
	InitialSpeed = 10000.f;

	// 최고 속도. 중력·공기저항 가속 시 속도 상한
	MaxSpeed = 10000.f;

	// 투사체 수명. ProjectileBase의 MaxLifetime과 별개로 PMC 이동을 제한하지 않음 (수명 관리는 Actor에서)
	ProjectileGravityScale = 0.3f;

	// 바운스 미사용 (기본). 유탄 등 파생 BP에서 활성화
	bShouldBounce = false;
	Bounciness = 0.3f;
	Friction = 0.3f;

	// Fast-Forward 시 큰 델타타임을 잘게 분할하여 중간 충돌 누락 방지
	// bForceSubStepping이 false면 TickComponent(ForwardPredictionTime) 단일 스텝으로 처리되어
	// 스폰 위치~전진 위치 사이 충돌을 모두 건너뜀
	bForceSubStepping = true;
	MaxSimulationTimeStep = 0.005f;
	MaxSimulationIterations = 16;

	// 이동 리플리케이션 비활성. 폭발 시점에만 위치 동기화
	SetIsReplicated(false);
}

bool UPRProjectileMovementComponent::ShouldBounce(const FHitResult& Hit)
{
	if (!bShouldBounce)
	{
		return false;
	}

	if (CurrentBounceCount >= MaxBounceCount)
	{
		return false;
	}
	
	if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		ECollisionChannel HitObjectType = HitComponent->GetCollisionObjectType();
		if (BounceChannels.Contains(HitObjectType))
		{
			return true;
		}
	}

	return false;
}

void UPRProjectileMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	AActor* HitActor = Hit.GetActor();
	bool bStopSimulating = false;

	if (ShouldBounce(Hit))
	{
		const FVector OldVelocity = Velocity;
		Velocity = ComputeBounceResult(Hit, TimeSlice, MoveDelta);

		// 바운스 이벤트 브로드캐스트
		OnProjectileBounce.Broadcast(Hit, OldVelocity);

		// 이벤트에서 속도를 변경했을 수 있으므로 임계값 재검사
		Velocity = LimitVelocity(Velocity);
		if (IsVelocityUnderSimulationThreshold())
		{
			bStopSimulating = true;
		}
		
		
		const bool bIsCharacter = Cast<ACharacter>(HitActor) != nullptr;
		if (bIsCharacter)
		{
			// 캐릭터에 바운스 한 경우 속력을 작게 보정 (앞에 떨어지도록)
			double VelX = Velocity.X * 0.3f;
			double VelY = Velocity.Y * 0.3f;
			double VelZ = Velocity.Z > 0 ? Velocity.Z * 0.1f : Velocity.Z;
			Velocity = LimitVelocity(FVector(VelX, VelY, VelZ));

			// [임시방편] 복잡한 메시와 연속 충돌하여 공중에서 멈추는 현상 방지. 1회 바운스 후 해당 액터 콜리전 무시
			if (UPrimitiveComponent* UpdatedPrim = Cast<UPrimitiveComponent>(UpdatedComponent))
			{
				UpdatedPrim->IgnoreActorWhenMoving(HitActor, true);
			}
		}
		else
		{
			// 캐릭터에 바운스 하지 않은 경우만 남은 바운스 횟수 차감
			++CurrentBounceCount;

			// 최대 바운스 도달 처리
			if (CurrentBounceCount >= MaxBounceCount)
			{
				bShouldBounce = false;
			}
		}
		
		// 서버 권위 투사체인 경우 바운스 시점 위치/속도를 클라이언트에 동기화
		APRProjectileBase* OwnerProjectile = Cast<APRProjectileBase>(GetOwner());
		if (IsValid(OwnerProjectile))
		{
			// 최종 바운스 이동 상태 기록
			OwnerProjectile->NotifyProjectileBounce();

			if (OwnerProjectile->HasProjectileAuthority())
			{
				// 서버 권위 바운스 이벤트 전파
				OwnerProjectile->PushRepMovement(EPRRepMovementEvent::Bounce);
			}
		}
	}
	else
	{
		bStopSimulating = true;
	}

	if (bStopSimulating)
	{
		StopSimulating(Hit);
	}
}
