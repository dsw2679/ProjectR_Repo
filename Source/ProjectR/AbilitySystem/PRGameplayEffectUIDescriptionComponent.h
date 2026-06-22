// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "PRGameplayEffectUIDescriptionComponent.generated.h"

// GameplayEffect 툴팁에 표시할 설명 한 줄
USTRUCT(BlueprintType)
struct PROJECTR_API FPRGameplayEffectUIDescriptionLine
{
	GENERATED_BODY()

public:
	// 툴팁 상세 항목 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GameplayEffect|UI")
	FText Label;

	// 툴팁 상세 항목 값 또는 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GameplayEffect|UI")
	FText Value;
};

// GameplayEffect가 UI에 노출할 수동 설명을 보관하는 컴포넌트
UCLASS(Blueprintable, EditInlineNew, CollapseCategories, DisplayName = "PR UI Description")
class PROJECTR_API UPRGameplayEffectUIDescriptionComponent : public UGameplayEffectUIData
{
	GENERATED_BODY()

public:
	// 툴팁 표시용 설명 라인 목록 반환
	const TArray<FPRGameplayEffectUIDescriptionLine>& GetDescriptionLines() const { return DescriptionLines; }

protected:
	// GameplayEffect의 조건부 효과나 복합 효과를 설명하는 UI 전용 라인 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GameplayEffect|UI")
	TArray<FPRGameplayEffectUIDescriptionLine> DescriptionLines;
};
