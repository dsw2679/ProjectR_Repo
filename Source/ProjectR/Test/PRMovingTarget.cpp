// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Moving 타겟 구현)
#include "PRMovingTarget.h"
#include "Components/BoxComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "Net/UnrealNetwork.h"

APRMovingTarget::APRMovingTarget()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false); // 수동 위치 보간을 위해 기본 무브먼트 리플리케이션은 끈다.

	RootCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("RootCollision"));
	RootComponent = RootCollision;
	RootCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
}

void APRMovingTarget::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();
	ServerLocation = StartLocation;

	if (HasAuthority() && IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 피격시 죽지 않게 체력을 매우 높게 설정
		CommonSet->InitMaxHealth(999999.0f);
		CommonSet->InitHealth(999999.0f);
	}
}

void APRMovingTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		CurrentDistance += MoveSpeed * DeltaTime * MoveDirection;

		if (FMath::Abs(CurrentDistance) >= MoveDistance)
		{
			MoveDirection *= -1;
			CurrentDistance = FMath::Clamp(CurrentDistance, -MoveDistance, MoveDistance);
		}

		// ActorRightVector를 기준으로 좌우 이동
		FVector NewLocation = StartLocation + GetActorRightVector() * CurrentDistance;
		SetActorLocation(NewLocation);
		ServerLocation = NewLocation;
	}
	else
	{
		// 클라이언트측 보간
		FVector CurrentLoc = GetActorLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLoc, ServerLocation, DeltaTime, InterpSpeed);
		SetActorLocation(NewLocation);
	}
}

void APRMovingTarget::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRMovingTarget, ServerLocation);
}

UAbilitySystemComponent* APRMovingTarget::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FPRDamageRegionInfo APRMovingTarget::GetDamageRegionInfo(FName BoneName) const
{
	FPRDamageRegionInfo Info;
	Info.RegionType = EPRDamageRegionType::None;
	Info.RegionTag = NAME_None;
	return Info;
}

void APRMovingTarget::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	// 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}

	APRPlayerController* PC = Cast<APRPlayerController>(Context.InstigatorController.Get());
	if (!IsValid(PC))
	{
		return;
	}

	UPRFloatingTextManager* FloatingTextManager = PC->GetFloatingTextManager();
	if (!IsValid(FloatingTextManager))
	{
		return;
	}

	EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;
	if (Context.Region.IsWeakpoint())
	{
		TextType = EPRFloatingTextType::WeakNormal;
	}
	else if (Context.bIsCritical)
	{
		TextType = EPRFloatingTextType::CriticalDamage;
	}

	FPRFloatingTextRequest Request;
	Request.Text = FText::AsNumber(FMath::CeilToInt(Context.FinalDamage));
	Request.TextType = TextType;
	Request.WorldLocation = Context.HitResult.ImpactPoint;

	FloatingTextManager->ClientShowFloatingText_Unreliable(Request);
}

FVector APRMovingTarget::GetPingMarkerWorldLocation_Implementation() const
{
	return GetActorLocation();
}

FPRWorldMarkerVisualData APRMovingTarget::GetPingMarkerVisualData_Implementation() const
{
	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetWorldMarkerPreset(EPRWorldMarkerPreset::Enemy);
	}
	return FPRWorldMarkerVisualData();
}

void APRMovingTarget::OnRep_ServerLocation()
{
	// OnRep에서는 보간을 수행하지 않고 Tick에서 수행한다.
}
