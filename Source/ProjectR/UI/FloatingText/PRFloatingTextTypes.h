// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI Floating Text 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRFloatingTextTypes.generated.h"

// 플로팅 텍스트 종류. PRDeveloperSettings에서 타입별 위젯 클래스를 매핑한다
UENUM(BlueprintType)
enum class EPRFloatingTextType : uint8
{
	// 일반 피해
	NormalDamage,
	// 방어된 피해
	BlockedDamage,
	// 치명타 피해
	CriticalDamage,
	// 약점 명중 일반 피해
	WeakNormal,
	// 약점 크리티컬
	WeakCritical,
	// 회복
	Heal,
	MAX UMETA(Hidden),
};

// 플로팅 텍스트 요청 정보 (외부 호출자가 매니저에 전달)
USTRUCT(BlueprintType)
struct FPRFloatingTextRequest
{
	GENERATED_BODY()

	// 표시할 텍스트 (데미지 수치 등)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	FText Text;

	// 텍스트 타입 (위젯 외형 결정)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;

	// 월드 좌표 (텍스트 출현 위치)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	FVector WorldLocation = FVector::ZeroVector;
};

// 플로팅 텍스트 위젯 초기화 파라미터 (매니저가 스타일을 반영하여 위젯에 전달)
USTRUCT(BlueprintType)
struct FPRFloatingTextParams
{
	GENERATED_BODY()

	// 표시할 텍스트
	UPROPERTY(BlueprintReadOnly, Category = "FloatingText")
	FText Text;

	// 텍스트 타입
	UPROPERTY(BlueprintReadOnly, Category = "FloatingText")
	EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;

	// 텍스트 색상 (DeveloperSettings 스타일에서 결정)
	UPROPERTY(BlueprintReadOnly, Category = "FloatingText")
	FLinearColor Color = FLinearColor::White;
};
