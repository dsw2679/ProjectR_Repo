// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (사격 데칼, 투사체 예측 궤적 및 UI 호출 등 코어 정적 헬퍼 함수 구축)
// Author: 이건주 (무기 Mod 버프 상태 체크 정적 헬퍼 구현)
#include "PRGameplayStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/FX/Impact/PRImpactManagerSubsystem.h"
#include "ProjectR/FX/PRFXNetworkComponent.h"
#include "ProjectR/FX/PRFXSubsystem.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"

namespace PRGameplayStaticsPrivate
{
	// GameplayStatics 헬퍼들이 동일한 요청 구조를 만들기 위한 공통 변환 함수
	FPRFXRequest MakeFXRequest(FGameplayTag FXTag, FInstancedStruct Payload, FPRFXPredictionKey PredictionKey = FPRFXPredictionKey())
	{
		FPRFXRequest Request;
		Request.FXTag = FXTag;
		Request.Payload = Payload;
		Request.PredictionKey = PredictionKey;
		return Request;
	}

	// WorldContextObject에서 FX Subsystem을 찾기 위한 공통 조회 함수
	UPRFXSubsystem* GetFXSubsystem(const UObject* WorldContextObject)
	{
		const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
		return IsValid(World) ? World->GetSubsystem<UPRFXSubsystem>() : nullptr;
	}

	// WorldContextObject에서 Impact Manager Subsystem을 찾기 위한 공통 조회 함수
	UPRImpactManagerSubsystem* GetImpactManagerSubsystem(const UObject* WorldContextObject)
	{
		const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
		return IsValid(World) ? World->GetSubsystem<UPRImpactManagerSubsystem>() : nullptr;
	}
}

void UPRGameplayStatics::GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 현재 액터의 모든 MeshComponent 수집
	Actor->GetComponents<UMeshComponent>(OutMeshes, /*bIncludeFromChildActors=*/false);

	// ChildActorComponent를 통한 자식 액터 재귀 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	Actor->GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* ChildActorComp : ChildActorComps)
	{
		if (!IsValid(ChildActorComp))
		{
			continue;
		}

		TArray<UMeshComponent*> ChildMeshes;
		GetAllMeshComponents(ChildActorComp->GetChildActor(), ChildMeshes);
		OutMeshes.Append(ChildMeshes);
	}

	// 부착된 액터 재귀 탐색
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, /*bResetArray=*/true, /*bRecursivelyIncludeAttachedActors=*/false);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		GetAllMeshComponents(AttachedActor, AttachedMeshes);
		OutMeshes.Append(AttachedMeshes);
	}
}

UPRInventoryComponent* UPRGameplayStatics::GetInventoryComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetInventoryComponent();
		}
	}
	
	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetInventoryComponent();
	}
	
	return nullptr;
}

UPRWeaponManagerComponent* UPRGameplayStatics::GetWeaponManagerComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetWeaponManagerComponent();
		}
	}
	
	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetWeaponManagerComponent();
	}
	
	return nullptr;
}

UPRQuickSlotComponent* UPRGameplayStatics::GetQuickSlotComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	APawn* AsPawn = nullptr;

	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}

	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetQuickSlotComponent();
		}
	}

	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetQuickSlotComponent();
	}

	return nullptr;
}

UPREquipmentManagerComponent* UPRGameplayStatics::GetEquipmentManagerComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetEquipmentManagerComponent();
		}
	}
	
	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetEquipmentManagerComponent();
	}
	
	return nullptr;
}

UAbilitySystemComponent* UPRGameplayStatics::GetAbilitySystemComponent(AActor* Actor)
{
	if (AController* AsController = Cast<AController>(Actor))
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AsController->GetPawn());
	}

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
}

void UPRGameplayStatics::CancelAbilityWithTags(AActor* TargetActor, const FGameplayTagContainer& AbilityTags)
{
	if (AbilityTags.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	// GAS 활성 어빌리티 태그 기반 취소
	TargetASC->CancelAbilities(&AbilityTags);
}

void UPRGameplayStatics::GrantAmmo(AActor* TargetActor, TSubclassOf<UGameplayEffect> AmmoEffect,
	float AmmoAmount)
{
	if (!IsValid(TargetActor) || !IsValid(AmmoEffect) || !TargetActor->HasAuthority() || AmmoAmount <= 0.f)
	{
		return;
	}
	
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
	
	const UPRAttributeSet_Weapon* WeaponSet = TargetASC->GetSet<UPRAttributeSet_Weapon>();
	if (!IsValid(WeaponSet))
	{
		return;
	}

	// SetByCaller로 탄약량을 GE Spec에 전달
	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(AmmoEffect, 1.f, Context);
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoMagnitude, AmmoAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

UPRGameInstance* UPRGameplayStatics::GetPRGameInstance(const UObject* WorldContext)
{
	if (IsValid(WorldContext))
	{
		if (auto World = WorldContext->GetWorld())
		{
			return World->GetGameInstance<UPRGameInstance>();
		}
	}
	return nullptr;
}


bool UPRGameplayStatics::GetPawnViewpoint(const APawn* Pawn, FVector& OutLocation, FRotator& OutRotation)
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	if (!IsValid(Pawn))
	{
		return false;
	}

	// TPS 숄더뷰: SpringArm/카메라 오프셋이 반영된 실제 카메라 위치/회전 사용
	const APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (IsValid(PC) && IsValid(PC->PlayerCameraManager))
	{
		OutLocation = PC->PlayerCameraManager->GetCameraLocation();
		OutRotation = PC->PlayerCameraManager->GetCameraRotation();
		return true;
	}

	// 폴백: 컨트롤러 또는 카메라 매니저 부재 시 액터 눈높이 기준
	Pawn->GetActorEyesViewPoint(OutLocation, OutRotation);
	return true;
}

FVector UPRGameplayStatics::ResolveCameraAimPoint(const APawn* Pawn, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors, FHitResult* OutHitResult)
{
	if (OutHitResult != nullptr)
	{
		// 호출자가 이전 프레임 HitResult를 재사용해도 실패 경로에서 충돌 정보가 남지 않도록 초기화
		*OutHitResult = FHitResult();
	}

	FVector CamLocation;
	FRotator CamRotation;
	if (!GetPawnViewpoint(Pawn, CamLocation, CamRotation))
	{
		return FVector::ZeroVector;
	}

	const FVector CamEnd = CamLocation + CamRotation.Vector() * TraceDistance;

	UWorld* World = IsValid(Pawn) ? Pawn->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return CamEnd;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRCameraAimTrace), false);
	Params.AddIgnoredActors(IgnoredActors);
	Params.AddIgnoredActor(Pawn);

	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, CamLocation, CamEnd, TraceChannel, Params);
	if (OutHitResult != nullptr)
	{
		// 카메라 트레이스가 실제 표면을 맞춘 경우 Impact FX의 위치와 법선 fallback으로 사용
		*OutHitResult = AimHit;
	}

	return AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
}

FTransform UPRGameplayStatics::ResolveProjectileLaunchTransform(const APawn* Pawn, const FVector& MuzzleLocation, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors)
{
	FVector CamLocation;
	FRotator CamRotation;
	if (!GetPawnViewpoint(Pawn, CamLocation, CamRotation))
	{
		return FTransform(FQuat::Identity, MuzzleLocation);
	}

	const FVector AimPoint = ResolveCameraAimPoint(Pawn, TraceDistance, TraceChannel, IgnoredActors);

	// 머즐에서 조준점으로 향하는 방향. 거리가 너무 짧으면(조준점이 머즐 바로 앞/뒤 등) 카메라 정면으로 폴백
	FVector LaunchDir = AimPoint - MuzzleLocation;
	if (!LaunchDir.Normalize(KINDA_SMALL_NUMBER))
	{
		LaunchDir = CamRotation.Vector();
	}

	return FTransform(LaunchDir.Rotation(), MuzzleLocation);
}

void UPRGameplayStatics::PlayLocalFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload)
{
	UPRFXSubsystem* FXSubsystem = PRGameplayStaticsPrivate::GetFXSubsystem(WorldContextObject);
	if (!IsValid(FXSubsystem))
	{
		return;
	}

	// 호출자가 가진 FXTag와 구체 Payload를 FX 시스템의 표준 요청 구조로 변환
	const FPRFXRequest Request = PRGameplayStaticsPrivate::MakeFXRequest(FXTag, Payload);
	FXSubsystem->PlayLocalFX(Request);
}

FPRFXPredictionKey UPRGameplayStatics::PlayPredictiveFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload)
{
	UPRFXSubsystem* FXSubsystem = PRGameplayStaticsPrivate::GetFXSubsystem(WorldContextObject);
	if (!IsValid(FXSubsystem))
	{
		return FPRFXPredictionKey();
	}

	// 예측 재생 요청은 Subsystem이 PredictionKey를 발급하고 내부 중복 제거 목록에 등록
	const FPRFXRequest Request = PRGameplayStaticsPrivate::MakeFXRequest(FXTag, Payload);
	return FXSubsystem->PlayPredictiveFX(Request);
}

void UPRGameplayStatics::PlayConfirmedFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload, FPRFXPredictionKey PredictionKey)
{
	UPRFXSubsystem* FXSubsystem = PRGameplayStaticsPrivate::GetFXSubsystem(WorldContextObject);
	if (!IsValid(FXSubsystem))
	{
		return;
	}

	// 서버 확정 재생은 호출자가 전달한 PredictionKey로 로컬 예측 재생과의 중복 여부를 검사
	const FPRFXRequest Request = PRGameplayStaticsPrivate::MakeFXRequest(FXTag, Payload, PredictionKey);
	FXSubsystem->PlayConfirmedFX(Request);
}

FPRFXPredictionKey UPRGameplayStatics::PlayPredictiveNetworkFX(const UObject* WorldContextObject, AActor* NetworkSourceActor, FGameplayTag FXTag, FInstancedStruct Payload)
{
	FPRFXRequest Request = PRGameplayStaticsPrivate::MakeFXRequest(FXTag, Payload);
	FPRFXPredictionKey PredictionKey;

	if (UPRFXSubsystem* FXSubsystem = PRGameplayStaticsPrivate::GetFXSubsystem(WorldContextObject))
	{
		// 로컬 예측 재생으로 만든 PredictionKey를 같은 서버 전파 요청에 넣어 소유 클라이언트 중복 재생 방지
		PredictionKey = FXSubsystem->PlayPredictiveFX(Request);
		Request.PredictionKey = PredictionKey;
	}

	const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
	if (!IsValid(World) || World->GetNetMode() == NM_Standalone)
	{
		return PredictionKey;
	}

	UPRFXNetworkComponent* FXNetworkComponent = UPRFXNetworkComponent::FindForActor(NetworkSourceActor);
	if (!IsValid(FXNetworkComponent))
	{
		return PredictionKey;
	}

	// PlayerController 소유 RPC 컴포넌트를 통해 서버가 Registry 정책에 맞는 클라이언트 전파 수행
	FXNetworkComponent->ServerPlayFX_Unreliable(Request);
	return PredictionKey;
}

void UPRGameplayStatics::PlayImpactFromHit(const UObject* WorldContextObject, const FHitResult& HitResult, bool bUseDecal)
{
	UPRImpactManagerSubsystem* ImpactManagerSubsystem = PRGameplayStaticsPrivate::GetImpactManagerSubsystem(WorldContextObject);
	if (!IsValid(ImpactManagerSubsystem))
	{
		return;
	}

	// 호출자가 Subsystem을 직접 조회하지 않고 HitResult 기반 Impact 재생 흐름으로 진입
	ImpactManagerSubsystem->PlayImpactFromHit(HitResult, bUseDecal);
}

void UPRGameplayStatics::PlayImpactAtLocation(const UObject* WorldContextObject, FGameplayTag ImpactTag, FVector ImpactLocation, FVector ImpactNormal, bool bUseDecal)
{
	UPRImpactManagerSubsystem* ImpactManagerSubsystem = PRGameplayStaticsPrivate::GetImpactManagerSubsystem(WorldContextObject);
	if (!IsValid(ImpactManagerSubsystem))
	{
		return;
	}

	// 호출자가 Subsystem을 직접 조회하지 않고 이미 계산된 표면 정보 기반 Impact 재생 흐름으로 진입
	ImpactManagerSubsystem->PlayImpactAtLocation(ImpactTag, ImpactLocation, ImpactNormal, bUseDecal);
}
