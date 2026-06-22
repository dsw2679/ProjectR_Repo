// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Fire 프리뷰 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/Optional.h"
#include "PRFirePreviewTypes.h"
#include "PRProjectileTypes.h"
#include "PRFirePreviewComponent.generated.h"

class UPRWeaponManagerComponent;
class APRWeaponActor;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * 투사체 발사 예측 경로를 매 틱 산출하는 액터 컴포넌트.
 * 시각화는 DrawTrajectory 가상 함수로 위임하며, 실제 출력 방식은 파생 클래스에서 구현.
 * 시각화는 로컬 클라이언트 전용으로 복제되지 않음.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFirePreviewComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPRFirePreviewComponent();
    
    /*~ UActorComponent Interface ~*/
    virtual void BeginPlay() override;

    /*~ UPRFirePreviewComponent Interface ~*/
    UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
    void SetTrajectoryPreviewEnabled(bool bInEnabled);

    // 투사체 경로 프리뷰 파라미터 등록
    void RegisterProjectilePreviewParams(const FPRFirePreviewKey& Key, const FPRFirePreviewEntry& Entry);

    // 투사체 경로 프리뷰 파라미터 등록 해제
    void UnregisterProjectilePreviewParams(const FPRFirePreviewKey& Key, FGameplayAbilitySpecHandle AbilitySpecHandle);

    // 현재 슬롯과 발사 모드 기준 프리뷰 파라미터 갱신
    void RefreshActivePreviewParams();

    // 궤적 기점 무기 액터 주입. nullptr 주입 시 표시 강제 OFF
    UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
    void SetWeaponActor(APRWeaponActor* InWeaponActor);

    // 표시 활성화. PrimaryComponentTick 활성. WeaponActor가 무효이면 활성화 무시
    UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
    void Show();

    // 표시 비활성화. PrimaryComponentTick 비활성 + ClearTrajectory 호출. 이미 비활성이면 무시
    UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
    void Hide();

    // 표시 상태 조회
    UFUNCTION(BlueprintPure, Category = "ProjectR|Projectile|Preview")
    bool IsShowing() const { return bIsShowing; }

    // 직전 틱 산출 결과 조회. UI/에임 어시스트 등 외부 시스템이 착탄 위치 참조 시 사용
    const FPRProjectilePreviewResult& GetLastResult() const { return LastResult; }

protected:
    /*~ UActorComponent Interface ~*/
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void OnUnregister() override;

    /*~ UPRFirePreviewComponent Interface ~*/

    // 매 틱 산출된 샘플 포인트 배열로 궤적을 출력. 베이스 구현은 DrawDebugSphere로 각 포인트 표시 후 끝에서 DrawTrajectoryISMC 호출.
    // 정식 백엔드(ISMC)는 PreviewMesh 지정 시 활성, 미지정 시 자동 스킵
    virtual void DrawTrajectory(const TArray<FVector>& SamplePoints);

    // 표시 종료 시 출력 상태를 초기화. 디버그 드로우는 단발 프레임이므로 정리 불필요, ISMC 인스턴스만 비움
    virtual void ClearTrajectory();

    // ISMC 인스턴스를 SamplePoints에 맞춰 갱신. 일반 포인트는 CustomData=0, 착탄 포인트는 CustomData=1로 설정.
    // PreviewMesh 미지정 또는 owner 액터 부재 시 no-op
    virtual void DrawTrajectoryISMC(const TArray<FVector>& SamplePoints);

    // ISMC 인스턴스 일괄 제거
    virtual void ClearTrajectoryISMC();

private:
    // 현재 슬롯과 발사 모드 기준 프리뷰 키 산출
    bool TryResolveActivePreviewKey(FPRFirePreviewKey& OutKey);

    // 지정 키의 투사체 프리뷰 등록 여부
    bool HasProjectilePreviewForKey(const FPRFirePreviewKey& Key) const;

    // 발사 파라미터 일괄 반영
    void ApplyFireParams(const FPRProjectilePreviewParams& InParams);

    // PredictProjectilePath 1회 호출 + 결과를 SampleSpacing 기준으로 다운샘플링하여 LastResult 갱신
    void RebuildPath();

    // 카메라 조준점 기반 히트스캔 미리보기 갱신
    void UpdateHitScanPreview();

    // 현재 활성 무기 데이터 기준 히트스캔 미리보기 거리 반환
    float GetHitScanPreviewDistance();

    // 크로스헤어 히트스캔 미리보기 상태 초기화
    void ClearHitScanPreview();

    // EventManager 기반 크로스헤어 미리보기 적중 상태 전파
    void BroadcastPreviewHit(bool bHit, bool bForceBroadcast = false);

    // PlayerState 복제 지연 이후 무기 매니저 재조회 캐시
    UPRWeaponManagerComponent* GetWeaponManager();

    // 사격 모드 태그 조회용 ASC 캐시
    class UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

protected:
    // 발사 파라미터 묶음. 무기/탄종 변경 시 일괄 교체
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
    FPRProjectilePreviewParams FireParams;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
    bool bDrawDebug = false;
    
    // 임시 DrawDebugSphere 시각화의 구체 반지름(cm)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug", meta = (ClampMin = "0.1"))
    float DebugSphereRadius = 3.f;
    
    // 일반 포인트 색상
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
    FColor DebugColor = FColor::Green;

    // 착탄 지점 강조 색상. EndReason이 HitBlocking일 때 마지막 포인트에 적용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
    FColor DebugHitColor = FColor::Red;

    // ISMC 인스턴싱에 사용할 작은 구체 메시. nullptr이면 ISMC 출력 자동 스킵
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|ISMC")
    TObjectPtr<UStaticMesh> PreviewMesh;

    // ISMC 인스턴스의 균일 스케일 배율. 메시 원본 스케일 기준
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|ISMC", meta = (ClampMin = "0.01"))
    float PreviewMeshScale = 0.6f;

private:
    // 궤적 기점 무기 액터 약참조. 매 틱 GetMuzzleTransform()으로 시작 위치/방향 조회
    TWeakObjectPtr<APRWeaponActor> WeaponActor;

    // ISMC 컴포넌트. owner 액터에 동적 생성/등록되며 OnUnregister에서 정리.
    // 첫 DrawTrajectoryISMC 호출 시 lazy 생성
    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> TrajectoryISMC;

    // 활성화 여부
    bool bIsTrajectoryEnabled = false;
    
    // 표시 ON/OFF 상태
    bool bIsShowing = false;

    // 직전 틱 산출 결과 캐시
    FPRProjectilePreviewResult LastResult;

    // 키별 투사체 경로 프리뷰 등록 목록
    TMap<FPRFirePreviewKey, FPRFirePreviewEntry> PreviewEntries;

    // 현재 적용 중인 프리뷰 키
    TOptional<FPRFirePreviewKey> ActivePreviewKey;

    // 크로스헤어에 마지막으로 전파한 히트스캔 미리보기 상태 존재 여부
    bool bHasPreviewHitState = false;

    // 크로스헤어에 마지막으로 전파한 히트스캔 미리보기 적중 여부
    bool bLastPreviewHit = false;
    
    TWeakObjectPtr<UPRWeaponManagerComponent> CachedWeaponManager;
};
