// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (기본 투사체 동기화 및 예측 바운스 연산 타입 정의)
// Author: 손승우 (보스 전용 투사체 타입 정의)
// Author: 이건주 (드론/힐링샷 및 Penitent 투사체 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "ProjectR/ProjectR.h"
#include "PRProjectileTypes.generated.h"

class AActor;
class APRProjectileBase;

DECLARE_DYNAMIC_DELEGATE_OneParam(FProjectileSpawnedDelegate, APRProjectileBase*, SpawnedProjectile);

// 투사체 Spawn 시점에 함께 전달되는 homing 프레젠테이션 스케줄
USTRUCT()
struct FPRProjectileRepHomingSchedule
{
	GENERATED_BODY()

	// 호밍 대상 액터. nullptr이면 스케줄을 사용하지 않는다.
	UPROPERTY()
	TObjectPtr<AActor> HomingTargetActor = nullptr;

	// 호밍 가속도. 0 이하이면 스케줄을 사용하지 않는다.
	UPROPERTY()
	float HomingAcceleration = 0.0f;

	// 투사체 Spawn 이후 호밍을 시작하기까지의 지연 시간
	UPROPERTY()
	float StartDelay = 0.0f;

	// 호밍 유지 시간. 0 이하이면 투사체가 끝날 때까지 유지한다.
	UPROPERTY()
	float Duration = 0.0f;

	// 스케줄 변경 번호. Spawn payload에서 같은 값 조합도 명시적으로 구분할 때 사용한다.
	UPROPERTY()
	uint8 Revision = 0;

	bool IsEnabled() const;
	void Reset();
};

// 투사체 이동 동기화 이벤트 종류
UENUM()
enum class EPRRepMovementEvent : uint8
{
	Spawn,       // 최초 스폰 (SimulatedProxy 언히든 + 시뮬 시작)
	Bounce,      // 바운스 발생 시 위치/속도 스냅
	Detonation,  // 폭발/착탄 확정
	Launch,		 // 프로젝타일 런치 상태
	Stop,		 // 최종 충돌 정착
	Fall		 // 최종 충돌 낙하 시작
};

// 투사체 소멸 원인
UENUM(BlueprintType)
enum class EPRProjectileDestroyReason : uint8
{
	// 충돌 파괴
	Impact,
	// 체력 고갈 파괴
	DamageDepleted,
	// 수명 만료 정리
	LifeTimeExpired,
	// 복제 폭발 정리
	ReplicatedDetonation,
	// 명시 요청 정리
	Manual,
};

// 파괴 연출 재생 대상
PROJECTR_API bool PRShouldPlayProjectileDestroyEffect(EPRProjectileDestroyReason DestroyReason);

// 이벤트 드리븐 투사체 이동 동기화 페이로드. 서버가 이벤트 발생 시에만 Push
USTRUCT()
struct FPRProjectileRepMovement
{
	GENERATED_BODY()

	// 이벤트 발생 시점의 위치
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	// 이벤트 발생 시점의 속도
	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	// 이벤트 발생 시점의 회전
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	// 이벤트 종류
	UPROPERTY()
	EPRRepMovementEvent Event = EPRRepMovementEvent::Spawn;

	// Bounce 이벤트 식별 번호. 서버/예측 투사체의 같은 순서 바운스를 비교하기 위한 값
	UPROPERTY()
	uint8 BounceIndex = 0;

	// Spawn 이벤트와 함께 전달할 homing 프레젠테이션 스케줄
	UPROPERTY()
	FPRProjectileRepHomingSchedule HomingSchedule;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRProjectileRepMovement> : public TStructOpsTypeTraitsBase2<FPRProjectileRepMovement>
{
	enum { WithNetSerializer = true };
};

// 투사체 스폰 파라미터. Client/Server 양측에서 동일 ID로 매칭하기 위한 정보 묶음
USTRUCT(BlueprintType)
struct FPRProjectileSpawnInfo
{
	GENERATED_BODY()

	// 투사체 식별자. 0은 무효
	uint32 ProjectileId = 0;

	// 스폰할 투사체 클래스
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 스폰 위치
	FVector SpawnLocation = FVector::ZeroVector;

	// 스폰 회전
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// 추가 스폰 옵션. 기본값은 호출자가 채워서 전달
	FActorSpawnParameters SpawnParameters;
};

// 투사체 예측 경로 시뮬레이션 종료 사유
UENUM(BlueprintType)
enum class EPRPreviewEndReason : uint8
{
	// 정상 진행 중 또는 미산출 상태
	None,
	// 첫 착탄으로 막힘. 이 지점에서 표시 종료
	HitBlocking,
	// MaxSimTime 도달
	TimeExceeded,
	// MaxDistance 도달
	DistanceExceeded,
};

// 투사체 예측 경로 산출에 필요한 발사 파라미터 묶음. 무기/어빌리티가 채워서 컴포넌트에 주입.
// 기점 무기 액터는 본 구조체에 포함되지 않으며 컴포넌트의 별도 Setter로 주입됨
USTRUCT(BlueprintType)
struct FPRProjectilePreviewParams
{
	GENERATED_BODY()

	// 초기 속력(cm/s). 무기 머즐 트랜스폼의 ForwardVector에 곱해져 초기 속도 벡터 구성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float InitialSpeed = 3000.f;

	// 중력 배율. 1.0이면 월드 기본 중력, 0.0이면 직선 비행. UProjectileMovementComponent의 ProjectileGravityScale와 1:1 대응
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	float GravityScale = 1.f;

	// 최대 시뮬레이션 시간(초). 이 시간을 넘으면 산출 종료. 첫 착탄이 먼저 발생하면 그 시점에 종료
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float MaxSimTime = 3.f;

	// 최대 비행 거리(cm). 누적 거리가 이 값을 초과하면 산출 종료. 0이면 거리 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float MaxDistance = 5000.f;

	// 시뮬레이션 스텝 빈도(Hz). 클수록 정밀도 상승, 비용 증가. 권장 30 ~ 60.
	// PredictProjectilePath 내부에서 SubstepDeltaTime = 1.f / SimFrequency 로 사용되므로 단위는 Hz임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "1.0"))
	float SimFrequency = 30.f;

	// 충돌 검사 반지름(cm). 0이면 라인 트레이스, 양수면 스피어 트레이스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float CollisionRadius = 5.f;

	// 샘플 포인트 사이의 최소 거리(cm). 다운샘플링 기준. 너무 작으면 포인트 과다, 너무 크면 굴곡 표현 부족
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "1.0"))
	float SampleSpacing = 40.f;

	// 표시할 최대 포인트 수. 출력 배열 상한 보호. 0이면 무제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0"))
	int32 MaxSamplePoints = 64;

	// 충돌 채널. 기본값은 프로젝트 Projectile 채널로, Projectile 콜리전 프로필을 가진 실제 투사체와 동일한 블록 대응을 따름.
	// Visibility 채널은 Projectile 프로필이 무시하는 객체까지 막아 시각/실제 궤적이 어긋나므로 사용 금지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	TEnumAsByte<ECollisionChannel> TraceChannel = PRCollisionChannels::ECC_Projectile;
};

// 매 틱 산출되는 궤적 결과 캐시. 외부에서 조회 가능
USTRUCT(BlueprintType)
struct FPRProjectilePreviewResult
{
	GENERATED_BODY()

	// 다운샘플링 후 최종 표시에 사용된 월드 좌표 배열. 시작점에서 종료점까지
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	TArray<FVector> SamplePoints;

	// 시뮬레이션 종료 사유
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	EPRPreviewEndReason EndReason = EPRPreviewEndReason::None;

	// 종료 지점의 충돌 정보. EndReason이 HitBlocking일 때만 유효
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	FHitResult EndHit;

	// 산출에 걸린 총 시뮬레이션 시간(초)
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	float ElapsedSimTime = 0.f;
};
