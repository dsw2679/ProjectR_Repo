// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Character Event Router 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRFaerinCharacterEventRouterComponent.generated.h"

class UPRFaerinWeaponVisualComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FFaerinCharacterEventSignature, FName);

// Faerin 원작 CharacterEvent를 C++ 시스템으로 중계한다.
// Router는 직접 패턴을 실행하지 않고, 시각 전환과 활성 Ability listener 호출만 담당한다.
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinCharacterEventRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFaerinCharacterEventRouterComponent();

	// 활성 Ability가 Faerin CharacterEvent를 받을 수 있도록 listener를 등록한다.
	void RegisterFaerinEventListener(UObject* Listener, FFaerinCharacterEventSignature::FDelegate Delegate);

	// Ability 종료 시 listener를 제거한다.
	void UnregisterFaerinEventListener(UObject* Listener);

	// AnimNotify에서 전달한 원작 CharacterEvent 이름을 처리하고 listener에게 전파한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	void RouteFaerinCharacterEvent(FName EventName);

protected:
	/*~ UActorComponent Interface ~*/
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

private:
	UPRFaerinWeaponVisualComponent* ResolveWeaponVisualComponent();
	void LogMissingWeaponVisualComponent() const;

protected:
	// ShowSword / ShowShards 이벤트를 처리할 무기 비주얼 컴포넌트다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<UPRFaerinWeaponVisualComponent> WeaponVisualComponent;

	// 직접 참조가 비어 있을 때 탐색할 무기 비주얼 컴포넌트 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Fallback")
	FName WeaponVisualComponentName = TEXT("PRFaerinWeaponVisualComponent");

	// BeginPlay에서 무기 비주얼 컴포넌트 복구가 실패했을 때 경고 로그를 출력할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug")
	bool bWarnIfWeaponVisualMissing = true;

	// Faerin CharacterEvent 라우팅 여부를 로그로 확인할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Debug")
	bool bLogRoutedEvents = false;

private:
	FFaerinCharacterEventSignature OnFaerinCharacterEvent;
};
