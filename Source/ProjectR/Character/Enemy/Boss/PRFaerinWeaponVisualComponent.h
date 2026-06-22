// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Weapon Visual 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRFaerinWeaponVisualComponent.generated.h"

class UBoxComponent;
class USkeletalMeshComponent;

// Faerin 본체 BP에 배치된 검 파츠와 공격 판정 참조를 제어한다.
// 컴포넌트 생성 책임은 에디터 BP에 두고, C++은 원작 CharacterEvent에 따른 표시 전환만 담당한다.
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinWeaponVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFaerinWeaponVisualComponent();

	// BP에 배치된 컴포넌트 참조를 직접 할당 또는 이름 기반으로 다시 찾는다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	void ResolveConfiguredComponents();

	// 원작 ShowSword 이벤트에 맞춰 완성 검날 표현을 켠다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	void ShowSword();

	// 원작 ShowShards 이벤트에 맞춰 조각난 검 표현을 켠다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	void ShowShards();

	// 런지/검 공격의 판정 구간에서 HitBox 충돌 참조를 켜거나 끈다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	void SetBladeHitBoxEnabled(bool bEnabled);

	// 런지 Ability가 사용할 검 판정 BoxComponent를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin")
	UBoxComponent* GetBladeHitBox() const { return BladeHitBox; }

	// 검 판정 fallback에서 사용할 본/소켓 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin")
	FName GetBladeBoneName() const { return BladeBoneName; }

protected:
	/*~ UActorComponent Interface ~*/
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

private:
	void LogMissingConfiguredComponents() const;

protected:
	// 완성 검날 Mesh다. BP에서 직접 연결하는 방식을 1차 기준으로 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Weapon")
	TObjectPtr<USkeletalMeshComponent> SwordBladeMesh;

	// 손잡이/장식 Mesh다. 기본적으로 계속 표시한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Weapon")
	TObjectPtr<USkeletalMeshComponent> SwordHandleMesh;

	// 조각난 검날 Mesh다. 원작 ShowShards 이벤트에서 다시 표시한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Weapon")
	TObjectPtr<USkeletalMeshComponent> SwordShardsMesh;

	// WP_Blade 대체용 판정 Box다. 실제 피해는 Ability sweep이 서버에서 처리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Weapon")
	TObjectPtr<UBoxComponent> BladeHitBox;

	// 직접 참조가 비어 있을 때 탐색할 완성 검날 컴포넌트 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Fallback")
	FName SwordBladeMeshName = TEXT("SwordBladeMesh");

	// 직접 참조가 비어 있을 때 탐색할 손잡이 컴포넌트 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Fallback")
	FName SwordHandleMeshName = TEXT("SwordHandleMesh");

	// 직접 참조가 비어 있을 때 탐색할 조각 검날 컴포넌트 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Fallback")
	FName SwordShardsMeshName = TEXT("SwordShardsMesh");

	// 직접 참조가 비어 있을 때 탐색할 판정 Box 컴포넌트 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Fallback")
	FName BladeHitBoxName = TEXT("WP_Blade");

	// 검 판정 fallback에서 참조할 원작 검날 본 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Hit")
	FName BladeBoneName = TEXT("Bone_FN_Weapon_Sword_Blade");

	// ShowSword / ShowShards 전환 중 손잡이 Mesh를 계속 켜둘지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Weapon")
	bool bKeepHandleVisible = true;

	// BeginPlay에서 이름 기반 참조 복구가 실패했을 때 경고 로그를 출력할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug")
	bool bWarnIfComponentResolveFails = true;
};
