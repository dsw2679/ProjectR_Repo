// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI Floating Text 매니저 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRFloatingTextTypes.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "PRFloatingTextManager.generated.h"

class UWidgetComponent;
class UPRFloatingTextWidget;

// 풀링된 WidgetComponent를 이용해 플로팅 텍스트를 표시하는 매니저 컴포넌트
// PlayerController에 부착하여 사용한다
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRFloatingTextManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFloatingTextManager();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 플로팅 텍스트를 표시한다 (Reliable RPC). 중요한 텍스트(치명타, 약점 등)에 사용
	UFUNCTION(Client, Reliable)
	void ClientShowFloatingText_Reliable(const FPRFloatingTextRequest& Request);

	// 플로팅 텍스트를 표시한다 (Unreliable RPC). 일반 데미지 등 누락 허용 텍스트에 사용
	UFUNCTION(Client, Unreliable)
	void ClientShowFloatingText_Unreliable(const FPRFloatingTextRequest& Request);

	// 로컬에서 직접 플로팅 텍스트를 표시한다
	void ShowFloatingText(const FPRFloatingTextRequest& Request);

protected:
	// 풀에서 WidgetComponent를 꺼낸다. 부족하면 새로 생성한다
	UWidgetComponent* AcquireWidgetComponent();

	// WidgetComponent를 풀에 반환한다
	void ReleaseWidgetComponent(UWidgetComponent* InComponent);

	// 위젯 연출 완료 콜백. 풀 반환을 처리한다
	void OnFloatingTextFinished(UPRFloatingTextWidget* InWidget);

	// 활성 텍스트가 MaxActiveCount를 초과하면 가장 오래된 것을 즉시 제거한다
	void ForceRetireOldest();

	// 타입에 대응하는 스타일(위젯 클래스 + 색상)을 조회한다
	FPRFloatingTextStyle ResolveStyle(EPRFloatingTextType TextType) const;

protected:
	// 풀 초기 사이즈
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Pool")
	int32 InitialPoolSize = 5;

	// 화면에 동시에 표시할 수 있는 최대 플로팅 텍스트 수. 초과 시 가장 오래된 것부터 제거
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Pool", meta = (ClampMin = "1"))
	int32 MaxActiveCount = 20;

	// 기본 위젯 클래스. PRDeveloperSettings에 매핑이 없을 때 폴백으로 사용
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Widget")
	TSubclassOf<UPRFloatingTextWidget> DefaultWidgetClass;

	// 표시 위치에 적용할 랜덤 월드 오프셋 범위 (각 축 ±Range)
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Widget")
	FVector RandomOffsetRange = FVector(4.0f, 4.0f, 4.0f);

private:
	// WidgetComponent를 소유할 로컬 전용 액터. PC의 렌더링 속성 영향을 받지 않기 위해 분리
	UPROPERTY(Transient)
	TObjectPtr<AActor> PoolActor;

	// 사용 가능한 WidgetComponent 풀
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> AvailablePool;

	// 현재 활성 상태인 WidgetComponent 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> ActiveWidgets;

	// 활성 위젯과 소유 WidgetComponent 매핑 (반환 시 역추적용)
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPRFloatingTextWidget>, TObjectPtr<UWidgetComponent>> WidgetToComponentMap;

};
