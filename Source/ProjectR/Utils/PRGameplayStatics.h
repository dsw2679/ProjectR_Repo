// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (사격 데칼, 투사체 예측 궤적 및 UI 호출 등 코어 정적 헬퍼 함수 구축)
// Author: 이건주 (무기 Mod 버프 상태 체크 정적 헬퍼 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "ProjectR/FX/PRFXTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRGameplayStatics.generated.h"

class UPREquipmentManagerComponent;
class UPRWeaponManagerComponent;
class UPRGameInstance;
class UGameplayEffect;
class APawn;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;
class UPRInventoryComponent;
class UPRImpactManagerSubsystem;
class UPRQuickSlotComponent;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 액터 및 모든 ChildActor를 재귀 탐색하여 UMeshComponent를 OutMeshes에 수집 */
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static void GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes);

	/** 액터의 InventoryComponent를 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static UPRInventoryComponent* GetInventoryComponent(AActor* Actor);

	/** 액터의 WeaponManagerComponent 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static UPRWeaponManagerComponent* GetWeaponManagerComponent(AActor* Actor);

	/** 액터의 QuickSlotComponent 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static UPRQuickSlotComponent* GetQuickSlotComponent(AActor* Actor);
	
	/** 액터의 EquipmentManagerComponent 찾아 반환 */
	static UPREquipmentManagerComponent* GetEquipmentManagerComponent(AActor* Actor);
	
	/** Actor로 부터 ASC 획득 (Controller or Owner or Avatar 자동) */
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor);

	/** TargetActor의 ASC에서 지정 태그를 가진 활성 어빌리티 취소 */
	UFUNCTION(BlueprintCallable, Category = "PR|Ability")
	static void CancelAbilityWithTags(AActor* TargetActor, const FGameplayTagContainer& AbilityTags);

	// Ammo 부여
	UFUNCTION(BlueprintCallable, Category = "PR|Statics")
	static void GrantAmmo(AActor* TargetActor,TSubclassOf<UGameplayEffect> AmmoEffect,  float AmmoAmount);
	
	// GameInstance 획득
	UFUNCTION(BlueprintPure, Category = "PR|Statics")
	static UPRGameInstance* GetPRGameInstance(const UObject* WorldContext);
	
	/** 폰의 PlayerController 카메라 위치/회전 추출. 컨트롤러 부재 시 Pawn::GetActorEyesViewPoint 폴백.
	 * 로컬 컨트롤된 폰에서만 의미가 있음. 추출 성공 여부 반환  
	 */
	static bool GetPawnViewpoint(const APawn* Pawn, FVector& OutLocation, FRotator& OutRotation);

	/** 폰 카메라 뷰포인트에서 1차 라인 트레이스로 조준점 산출.
	 * 미충돌 또는 월드 부재 시 카메라 끝점 반환. OutHitResult가 있으면 카메라 트레이스 결과 기록
	 */
	static FVector ResolveCameraAimPoint(const APawn* Pawn, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors, FHitResult* OutHitResult = nullptr);

	/* 머즐 위치 + 카메라 조준점 기반 발사 트랜스폼 산출.
	 * 위치 = MuzzleLocation, 회전 = (AimPoint - MuzzleLocation) Rotation. 거리 0이면 카메라 정면으로 폴백
	 */
	static FTransform ResolveProjectileLaunchTransform(const APawn* Pawn, const FVector& MuzzleLocation, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors);

	// 로컬 월드에서만 FX Cue 실행
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|FX")
	static void PlayLocalFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload);

	// 로컬 예측 FX Cue 실행 후 서버 확정 FX 중복 제거에 사용할 PredictionKey 반환
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|FX")
	static FPRFXPredictionKey PlayPredictiveFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload);

	// 서버에서 확정된 FX Cue를 현재 클라이언트 월드에서 실행
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|FX")
	static void PlayConfirmedFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload, FPRFXPredictionKey PredictionKey);

	// 로컬 예측 FX Cue 실행과 서버 전파 요청을 한 번에 처리
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|FX")
	static FPRFXPredictionKey PlayPredictiveNetworkFX(const UObject* WorldContextObject, AActor* NetworkSourceActor, FGameplayTag FXTag, FInstancedStruct Payload);

	// HitResult를 Impact Manager Subsystem에 전달하여 표면별 Impact VFX와 선택적 Decal 재생 요청
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|Impact")
	static void PlayImpactFromHit(const UObject* WorldContextObject, const FHitResult& HitResult, bool bUseDecal = true);

	// 이미 결정된 Impact 태그와 표면 Transform 정보를 Impact Manager Subsystem에 전달하여 Impact VFX와 선택적 Decal 재생 요청
	UFUNCTION(BlueprintCallable, Category = "PR|Statics|Impact")
	static void PlayImpactAtLocation(const UObject* WorldContextObject, FGameplayTag ImpactTag, FVector ImpactLocation, FVector ImpactNormal, bool bUseDecal = true);
};
