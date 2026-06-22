// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI Floating Text 매니저 구현)
#include "PRFloatingTextManager.h"
#include "PRFloatingTextWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "TimerManager.h"
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFloatingText, Log, All);

// ===== 생성자 =====

UPRFloatingTextManager::UPRFloatingTextManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// ===== UActorComponent Interface =====

void UPRFloatingTextManager::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 클라이언트에서만 풀을 생성한다 (위젯은 로컬 전용)
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || !PC->IsLocalController())
	{
		return;
	}

	// WidgetComponent를 소유할 로컬 전용 액터 스폰. PC의 렌더링 속성 영향을 피한다
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	PoolActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (IsValid(PoolActor))
	{
		PoolActor->SetReplicates(false);
	}

	// 초기 풀 사이즈만큼 WidgetComponent를 미리 생성
	for (int32 i = 0; i < InitialPoolSize; ++i)
	{
		if (UWidgetComponent* NewComp = AcquireWidgetComponent())
		{
			ReleaseWidgetComponent(NewComp);
		}
	}
}

void UPRFloatingTextManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AvailablePool.Empty();
	ActiveWidgets.Empty();
	WidgetToComponentMap.Empty();

	if (IsValid(PoolActor))
	{
		PoolActor->Destroy();
		PoolActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ===== 플로팅 텍스트 표시 =====

void UPRFloatingTextManager::ClientShowFloatingText_Reliable_Implementation(const FPRFloatingTextRequest& Request)
{
	ShowFloatingText(Request);
}

void UPRFloatingTextManager::ClientShowFloatingText_Unreliable_Implementation(const FPRFloatingTextRequest& Request)
{
	ShowFloatingText(Request);
}

void UPRFloatingTextManager::ShowFloatingText(const FPRFloatingTextRequest& Request)
{
	if (MaxActiveCount <= 0)
	{
		return;
	}

	// 활성 텍스트가 최대 수에 도달했으면 가장 오래된 것을 제거
	ForceRetireOldest();

	UWidgetComponent* WidgetComp = AcquireWidgetComponent();
	if (!IsValid(WidgetComp))
	{
		return;
	}

	// 타입에 맞는 스타일 조회
	FPRFloatingTextStyle Style = ResolveStyle(Request.TextType);
	if (!IsValid(Style.WidgetClass))
	{
		ReleaseWidgetComponent(WidgetComp);
		return;
	}

	// 기존 위젯의 클래스가 동일하면 재사용, 다르면 재생성
	UPRFloatingTextWidget* FloatingWidget = Cast<UPRFloatingTextWidget>(WidgetComp->GetWidget());
	if (IsValid(FloatingWidget) && FloatingWidget->GetClass() != Style.WidgetClass)
	{
		// 위젯 클래스 교체 전 기존 위젯의 BP Delay와 애니메이션 상태 제거
		FloatingWidget->ResetFloatingTextForReuse();
		UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Replace WidgetClass OldWidget=%s OldClass=%s NewClass=%s"),
			*GetNameSafe(FloatingWidget),
			*GetNameSafe(FloatingWidget->GetClass()),
			*GetNameSafe(Style.WidgetClass));
	}

	if (!IsValid(FloatingWidget) || FloatingWidget->GetClass() != Style.WidgetClass)
	{
		WidgetComp->SetWidgetClass(Style.WidgetClass);
	}
	
	WidgetComp->InitWidget();
	FloatingWidget = Cast<UPRFloatingTextWidget>(WidgetComp->GetWidget());

	if (!IsValid(FloatingWidget))
	{
		ReleaseWidgetComponent(WidgetComp);
		return;
	}

	// WidgetComponent의 화면 레이어 등록보다 먼저 Slate 위젯 생성
	FloatingWidget->TakeWidget();

	// 풀에서 재사용된 위젯의 이전 BP Delay와 애니메이션 상태 제거
	FloatingWidget->ResetFloatingTextForReuse();

	// WidgetComponent 위치/가시성 설정 (랜덤 오프셋 적용)
	const FVector RandomOffset(
		FMath::FRandRange(-RandomOffsetRange.X, RandomOffsetRange.X),
		FMath::FRandRange(-RandomOffsetRange.Y, RandomOffsetRange.Y),
		FMath::FRandRange(-RandomOffsetRange.Z, RandomOffsetRange.Z)
	);
	WidgetComp->SetWorldLocation(Request.WorldLocation + RandomOffset);
	WidgetComp->SetVisibility(true);
	const FName FloatingTextLayerName(*FString::Printf(TEXT("FloatingTextLayer_%d"), Style.LayerZOrder));
	WidgetComp->SetInitialSharedLayerName(FloatingTextLayerName);
	WidgetComp->SetInitialLayerZOrder(Style.LayerZOrder);

	// 위젯-컴포넌트 매핑 등록
	WidgetToComponentMap.Add(FloatingWidget, WidgetComp);

	// 연출 완료 콜백 바인딩
	FloatingWidget->OnFloatingTextFinished.BindUObject(this, &UPRFloatingTextManager::OnFloatingTextFinished);

	// 요청 + 스타일로 위젯 초기화 파라미터 조립
	FPRFloatingTextParams Params;
	Params.Text = Request.Text;
	Params.TextType = Request.TextType;
	Params.Color = Style.Color;

	// 위젯 초기화 및 연출 시작
	FloatingWidget->InitFloatingText(Params);

	// 화면 레이어 등록 후 다음 Tick 재생 예약
	TWeakObjectPtr<UPRFloatingTextWidget> WeakFloatingWidget = FloatingWidget;
	TWeakObjectPtr<UWidgetComponent> WeakWidgetComp = WidgetComp;
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakFloatingWidget, WeakWidgetComp]()
	{
		UPRFloatingTextWidget* DeferredWidget = WeakFloatingWidget.Get();
		UWidgetComponent* DeferredComp = WeakWidgetComp.Get();
		if (!IsValid(DeferredWidget) || !IsValid(DeferredComp))
		{
			UE_LOG(LogPRFloatingText, Warning, TEXT("[FloatingText] DeferredPlaySkipped Invalid Widget=%s Comp=%s"),
				*GetNameSafe(DeferredWidget),
				*GetNameSafe(DeferredComp));
			return;
		}

		TObjectPtr<UWidgetComponent>* FoundComp = WidgetToComponentMap.Find(DeferredWidget);
		if (!FoundComp || *FoundComp != DeferredComp)
		{
			UE_LOG(LogPRFloatingText, Warning, TEXT("[FloatingText] DeferredPlaySkipped Released Widget=%s Comp=%s"),
				*GetNameSafe(DeferredWidget),
				*GetNameSafe(DeferredComp));
			return;
		}

		DeferredWidget->PlayFloatingText();

		UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] DeferredPlay Widget=%s Comp=%s"),
			*GetNameSafe(DeferredWidget),
			*GetNameSafe(DeferredComp));
	}));

	UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Show Text=%s Type=%d Comp=%s Widget=%s Active=%d Pool=%d"),
		*Request.Text.ToString(),
		static_cast<int32>(Request.TextType),
		*GetNameSafe(WidgetComp),
		*GetNameSafe(FloatingWidget),
		ActiveWidgets.Num(),
		AvailablePool.Num());
}

// ===== 풀 관리 =====

UWidgetComponent* UPRFloatingTextManager::AcquireWidgetComponent()
{
	// 풀에 사용 가능한 컴포넌트가 있으면 꺼낸다
	while (AvailablePool.Num() > 0)
	{
		UWidgetComponent* Comp = AvailablePool.Pop();
		if (!IsValid(Comp))
		{
			continue;
		}
		ActiveWidgets.Add(Comp);
		UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Acquire Reuse Comp=%s Active=%d Pool=%d"),
			*GetNameSafe(Comp),
			ActiveWidgets.Num(),
			AvailablePool.Num());
		return Comp;
	}

	// 새 WidgetComponent 생성 (PoolActor에 부착)
	if (!IsValid(PoolActor))
	{
		return nullptr;
	}

	UWidgetComponent* NewComp = NewObject<UWidgetComponent>(PoolActor);
	if (!IsValid(NewComp))
	{
		return nullptr;
	}

	NewComp->RegisterComponent();
	NewComp->SetVisibility(false);
	NewComp->SetWidgetSpace(EWidgetSpace::Screen);
	NewComp->SetDrawAtDesiredSize(true);

	ActiveWidgets.Add(NewComp);

	UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Acquire New Comp=%s Active=%d Pool=%d"),
		*GetNameSafe(NewComp),
		ActiveWidgets.Num(),
		AvailablePool.Num());

	return NewComp;
}

void UPRFloatingTextManager::ReleaseWidgetComponent(UWidgetComponent* InComponent)
{
	if (!IsValid(InComponent))
	{
		return;
	}

	UPRFloatingTextWidget* CurrentWidget = Cast<UPRFloatingTextWidget>(InComponent->GetWidget());
	if (IsValid(CurrentWidget))
	{
		// 풀 반납 전 남은 BP Delay와 애니메이션 상태 제거
		CurrentWidget->ResetFloatingTextForReuse();
		CurrentWidget->SetVisibility(ESlateVisibility::Collapsed);
		CurrentWidget->OnFloatingTextFinished.Unbind();
		WidgetToComponentMap.Remove(CurrentWidget);
	}

	// WidgetComponent 소유 위젯의 Slate 경로 유지
	// 위젯 인스턴스는 유지한 채 숨기기만 한다 (풀링 재사용)
	InComponent->SetVisibility(false);
	
	ActiveWidgets.Remove(InComponent);
	AvailablePool.Add(InComponent);

	UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Release Comp=%s Widget=%s Active=%d Pool=%d"),
		*GetNameSafe(InComponent),
		*GetNameSafe(CurrentWidget),
		ActiveWidgets.Num(),
		AvailablePool.Num());
}

void UPRFloatingTextManager::OnFloatingTextFinished(UPRFloatingTextWidget* InWidget)
{
	if (!IsValid(InWidget))
	{
		return;
	}

	TObjectPtr<UWidgetComponent>* FoundComp = WidgetToComponentMap.Find(InWidget);
	if (FoundComp && IsValid(*FoundComp))
	{
		UE_LOG(LogPRFloatingText, Log, TEXT("[FloatingText] Finish Callback Widget=%s Comp=%s"),
			*GetNameSafe(InWidget),
			*GetNameSafe(*FoundComp));
		ReleaseWidgetComponent(*FoundComp);
	}
	else
	{
		UE_LOG(LogPRFloatingText, Warning, TEXT("[FloatingText] Finish Callback Missing Mapping Widget=%s"),
			*GetNameSafe(InWidget));
	}
}

void UPRFloatingTextManager::ForceRetireOldest()
{
	if (MaxActiveCount <= 0)
	{
		ensureMsgf(false, TEXT("MaxActiveCount must be greater than 0."));

		// 잘못된 최대값이면 활성 항목을 모두 정리한다
		while (ActiveWidgets.Num() > 0)
		{
			UWidgetComponent* OldestComp = ActiveWidgets[0];
			ActiveWidgets.RemoveAt(0);
			if (IsValid(OldestComp))
			{
				ReleaseWidgetComponent(OldestComp);
			}
		}
		return;
	}

	while (ActiveWidgets.Num() >= MaxActiveCount)
	{
		// ActiveWidgets 배열의 첫 번째가 가장 오래된 항목
		UWidgetComponent* OldestComp = ActiveWidgets[0];
		if (!IsValid(OldestComp))
		{
			ActiveWidgets.RemoveAt(0);
			continue;
		}

		// 위젯 콜백 해제 및 매핑 정리
		UPRFloatingTextWidget* OldestWidget = Cast<UPRFloatingTextWidget>(OldestComp->GetWidget());
		if (IsValid(OldestWidget))
		{
			UE_LOG(LogPRFloatingText, Warning, TEXT("[FloatingText] ForceRetire Comp=%s Widget=%s Active=%d Max=%d"),
				*GetNameSafe(OldestComp),
				*GetNameSafe(OldestWidget),
				ActiveWidgets.Num(),
				MaxActiveCount);
			ReleaseWidgetComponent(OldestComp);
			continue;
		}

		ReleaseWidgetComponent(OldestComp);
	}
}

// ===== 위젯 클래스 조회 =====

FPRFloatingTextStyle UPRFloatingTextManager::ResolveStyle(EPRFloatingTextType TextType) const
{
	// PRDeveloperSettings에서 타입별 스타일 조회
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (IsValid(Settings))
	{
		FPRFloatingTextStyle Style = Settings->GetFloatingTextStyleSync(TextType);
		if (IsValid(Style.WidgetClass))
		{
			return Style;
		}
	}

	// 폴백: DefaultWidgetClass + 흰색
	FPRFloatingTextStyle Fallback;
	Fallback.WidgetClass = DefaultWidgetClass;
	Fallback.Color = FLinearColor::White;
	return Fallback;
}
