// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (어빌리티 태스크 Face 타겟 구현)
#include "ProjectR/AbilitySystem/Tasks/PRAbilityTask_FaceTarget.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

namespace
{
	constexpr float PRFaceTargetGroundTraceHalfHeight = 300.0f;
	constexpr float PRFaceTargetObstacleTraceHeight = 50.0f;
	constexpr float PRFaceTargetOccupationRadius = 45.0f;
}

// Tick 기반 회전 처리를 위한 기본 설정
UPRAbilityTask_FaceTarget::UPRAbilityTask_FaceTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

// 대상 방향 회전 및 접근 AbilityTask 생성
UPRAbilityTask_FaceTarget* UPRAbilityTask_FaceTarget::FaceTarget(UGameplayAbility* OwningAbility,
	AActor* TargetActor,
	const float BlendTime,
	const float TaskDuration,
	const float AcceptanceDistance,
	const float MovementScale,
	const float ObstacleBackoffDistance,
	const float ApproachFanAngle,
	const int32 TraceCount)
{
	UPRAbilityTask_FaceTarget* Task = NewAbilityTask<UPRAbilityTask_FaceTarget>(OwningAbility);
	Task->TargetActor = TargetActor;
	Task->BlendTime = FMath::Max(BlendTime, 0.0f);
	Task->TaskDuration = FMath::Max(TaskDuration, 0.0f);
	Task->AcceptanceDistance = FMath::Max(AcceptanceDistance, 0.0f);
	Task->MovementScale = FMath::Max(MovementScale, 0.0f);
	Task->ObstacleBackoffDistance = FMath::Max(ObstacleBackoffDistance, 0.0f);
	Task->ApproachFanAngle = FMath::Clamp(ApproachFanAngle, 0.0f, 360.0f);
	Task->TraceCount = FMath::Max(TraceCount, 0);
	return Task;
}

// AvatarActor와 대상 유효성 확인 후 초기 회전 적용
void UPRAbilityTask_FaceTarget::Activate()
{
	Super::Activate();

	CachedAvatarActor = GetAvatarActor();
	AActor* AvatarActor = CachedAvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		EndTask();
		return;
	}

	StartRotation = AvatarActor->GetActorRotation();
	ElapsedTime = 0.0f;
	bHasApproachLocation = ResolveApproachLocation(ApproachLocation);
	ApplyFaceRotation(0.0f);
	ApplyApproachMovement();
}

// 대상 방향으로 지속 회전 보간
void UPRAbilityTask_FaceTarget::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!IsValid(CachedAvatarActor.Get()) || !IsValid(TargetActor))
	{
		EndTask();
		return;
	}

	ApplyFaceRotation(DeltaTime);
	ApplyApproachMovement();

	if (TaskDuration > UE_SMALL_NUMBER && ElapsedTime >= TaskDuration)
	{
		EndTask();
	}
}

// Task 종료 시 참조 정리
void UPRAbilityTask_FaceTarget::OnDestroy(const bool bInOwnerFinished)
{
	TargetActor = nullptr;
	CachedAvatarActor.Reset();
	bHasApproachLocation = false;

	Super::OnDestroy(bInOwnerFinished);
}

// AvatarActor 기준 대상 수평 방향 회전값 계산
bool UPRAbilityTask_FaceTarget::ResolveDesiredRotation(FRotator& OutRotation) const
{
	const AActor* AvatarActor = CachedAvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector Direction = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutRotation = Direction.Rotation();
	OutRotation.Pitch = 0.0f;
	OutRotation.Roll = 0.0f;
	return true;
}

// 플레이어 기준 대상 방향 부채꼴 후보 중 가장 가까운 유효 접근 위치 계산
bool UPRAbilityTask_FaceTarget::ResolveApproachLocation(FVector& OutLocation) const
{
	const AActor* AvatarActor = CachedAvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor) || TraceCount <= 0)
	{
		return false;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams ApproachQueryParams(SCENE_QUERY_STAT(PRFaceTargetApproachTrace), false);
	ApproachQueryParams.AddIgnoredActor(AvatarActor);

	FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(PRFaceTargetApproachGroundTrace), false);
	GroundQueryParams.AddIgnoredActor(AvatarActor);
	GroundQueryParams.AddIgnoredActor(TargetActor);

	FCollisionQueryParams TargetViewQueryParams(SCENE_QUERY_STAT(PRFaceTargetApproachTargetViewTrace), false);
	TargetViewQueryParams.AddIgnoredActor(AvatarActor);
	TargetViewQueryParams.AddIgnoredActor(TargetActor);

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector CenterDirection = TargetLocation - AvatarLocation;
	CenterDirection.Z = 0.0f;
	const float DistanceToTarget = CenterDirection.Size();
	if (DistanceToTarget <= UE_SMALL_NUMBER)
	{
		return false;
	}

	CenterDirection /= DistanceToTarget;
	const float HalfFanAngle = ApproachFanAngle * 0.5f;
	const int32 SideStepCount = FMath::Max(FMath::CeilToInt(static_cast<float>(TraceCount - 1) * 0.5f), 1);
	float BestDistanceSq = TNumericLimits<float>::Max();
	bool bFoundLocation = false;

	for (int32 TraceIndex = 0; TraceIndex < TraceCount; ++TraceIndex)
	{
		const int32 StepIndex = (TraceIndex + 1) / 2;
		const float SideSign = TraceIndex % 2 == 0 ? 1.0f : -1.0f;
		const float AngleOffset = TraceIndex == 0 ? 0.0f : SideSign * HalfFanAngle * static_cast<float>(StepIndex) / static_cast<float>(SideStepCount);
		FVector TraceDirection = CenterDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
		TraceDirection.Z = 0.0f;
		TraceDirection.Normalize();

		const FVector HorizontalTraceStart = AvatarLocation + FVector::UpVector * PRFaceTargetObstacleTraceHeight;
		const FVector HorizontalTraceEnd = HorizontalTraceStart + TraceDirection * DistanceToTarget;
		FHitResult ApproachHit;
		const bool bHitApproach = World->LineTraceSingleByChannel(
			ApproachHit,
			HorizontalTraceStart,
			HorizontalTraceEnd,
			ECC_Visibility,
			ApproachQueryParams);

		// 플레이어 중심과 대상 중심 사이 거리만큼 진행한 ray의 첫 유효 지점
		const float CandidateTraceDistance = bHitApproach
			? FMath::Max(FVector::Dist2D(HorizontalTraceStart, ApproachHit.ImpactPoint) - ObstacleBackoffDistance, 0.0f)
			: DistanceToTarget;
		const FVector CandidateLocation = HorizontalTraceStart + TraceDirection * CandidateTraceDistance;
		const FVector GroundTraceStart = CandidateLocation + FVector::UpVector * PRFaceTargetGroundTraceHalfHeight;
		const FVector GroundTraceEnd = CandidateLocation - FVector::UpVector * PRFaceTargetGroundTraceHalfHeight;

		FHitResult GroundHit;
		if (!World->LineTraceSingleByChannel(GroundHit, GroundTraceStart, GroundTraceEnd, ECC_Visibility, GroundQueryParams))
		{
			continue;
		}

		const FVector GroundLocation = GroundHit.ImpactPoint;
		const FVector TargetViewTraceStart = GroundLocation + FVector::UpVector * PRFaceTargetObstacleTraceHeight;
		const FVector TargetViewTraceEnd = TargetLocation + FVector::UpVector * PRFaceTargetObstacleTraceHeight;

		FHitResult TargetViewHit;
		if (World->LineTraceSingleByChannel(TargetViewHit, TargetViewTraceStart, TargetViewTraceEnd, ECC_Visibility, TargetViewQueryParams))
		{
			continue;
		}

		if (IsApproachLocationOccupied(GroundLocation))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(AvatarLocation, GroundLocation);
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			OutLocation = GroundLocation;
			bFoundLocation = true;
		}
	}

	return bFoundLocation;
}

// 접근 후보 지점 주변의 다른 Pawn 점유 여부 확인
bool UPRAbilityTask_FaceTarget::IsApproachLocationOccupied(const FVector& TestLocation) const
{
	const AActor* AvatarActor = CachedAvatarActor.Get();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaceTargetApproachOccupation), false);
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(TargetActor);

	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(PRFaceTargetOccupationRadius);
	const bool bOverlapped = World->OverlapMultiByChannel(
		OverlapResults,
		TestLocation,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape,
		QueryParams);

	if (!bOverlapped)
	{
		return false;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		if (IsValid(Cast<APawn>(OverlapResult.GetActor())))
		{
			return true;
		}
	}

	return false;
}

// 보간 시간에 따른 yaw 회전 적용
void UPRAbilityTask_FaceTarget::ApplyFaceRotation(const float DeltaTime)
{
	AActor* AvatarActor = CachedAvatarActor.Get();
	FRotator DesiredRotation = FRotator::ZeroRotator;
	if (!IsValid(AvatarActor) || !ResolveDesiredRotation(DesiredRotation))
	{
		return;
	}

	ElapsedTime += FMath::Max(DeltaTime, 0.0f);
	const FRotator CurrentRotation = AvatarActor->GetActorRotation();
	const float Alpha = BlendTime > UE_SMALL_NUMBER ? FMath::Clamp(ElapsedTime / BlendTime, 0.0f, 1.0f) : 1.0f;
	const float TargetYaw = StartRotation.Yaw + FMath::FindDeltaAngleDegrees(StartRotation.Yaw, DesiredRotation.Yaw);
	const float NewYaw = FMath::Lerp(StartRotation.Yaw, TargetYaw, Alpha);

	AvatarActor->SetActorRotation(FRotator(CurrentRotation.Pitch, NewYaw, CurrentRotation.Roll));
}

// 선택된 접근 위치까지 수평 이동 입력 적용
void UPRAbilityTask_FaceTarget::ApplyApproachMovement()
{
	APawn* AvatarPawn = Cast<APawn>(CachedAvatarActor.Get());
	if (!IsValid(AvatarPawn) || !bHasApproachLocation || MovementScale <= UE_SMALL_NUMBER)
	{
		return;
	}

	FVector Direction = ApproachLocation - AvatarPawn->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.SizeSquared() <= FMath::Square(AcceptanceDistance))
	{
		return;
	}

	AvatarPawn->AddMovementInput(Direction.GetSafeNormal(), MovementScale);
}
