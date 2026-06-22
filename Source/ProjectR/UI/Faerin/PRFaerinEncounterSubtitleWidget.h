// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRFaerinEncounterSubtitleWidget.generated.h"

class UTextBlock;

// Faerin 인카운터 하단 자막 위젯.
// 선택지/버튼이 없고 입력을 가로채지 않는다(SelfHitTestInvisible).
// Gather 범위 안 모든 플레이어의 로컬 화면 하단에 Faerin 대사를 표시한다.
UCLASS(Blueprintable)
class PROJECTR_API UPRFaerinEncounterSubtitleWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 화자/본문을 채우고 자막을 표시한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Subtitle")
	void SetSubtitle(const FText& InSpeakerText, const FText& InBodyText);

	// 자막을 비우고 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Subtitle")
	void ClearSubtitle();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;

	// 화자명을 표시하는 TextBlock
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerText = nullptr;

	// 본문을 표시하는 TextBlock
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText = nullptr;
};
