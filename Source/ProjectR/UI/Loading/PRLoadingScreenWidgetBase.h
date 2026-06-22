// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Loading Screen 기본 구조 UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "PRLoadingScreenWidgetBase.generated.h"

class UImage;
class UWidgetAnimation;

USTRUCT(BlueprintType)
struct FPRLoadingScreenImageEntry
{
	GENERATED_BODY()

public:
	// 목적지 맵 패키지 이름 또는 짧은 맵 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FName MapName = NAME_None;

	// 목적지 맵에 사용할 첫 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush PrimaryImageBrush;

	// 로딩 단계에서 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush ViewportSecondaryImageBrush;
};

UCLASS(Blueprintable)
class PROJECTR_API UPRLoadingScreenWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// 목적지 맵 이름을 설정하고 대응 이미지를 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void SetLoadingDestination(const FString& MapPackageName);

	// 목적지 맵 패키지 이름 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	FString GetDestinationMapPackageName() const { return DestinationMapPackageName; }

	// 목적지 맵 짧은 이름 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	FName GetDestinationMapShortName() const { return DestinationMapShortName; }

	// 로딩 화면 등장 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void PlayFadeInAnimation();

	// 로딩 화면 퇴장 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void PlayFadeOutAnimation();

protected:
	// 목적지 맵 변경 시 블루프린트 후처리
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|Loading")
	void OnLoadingDestinationChanged(const FString& MapPackageName, FName MapShortName);

private:
	const FPRLoadingScreenImageEntry* FindImageEntry(FName MapPackageName, FName MapShortName) const;
	void ApplyImageBrushes(const FPRLoadingScreenImageEntry* ImageEntry);
	FName ResolveShortMapName(const FString& MapPackageName) const;

protected:
	// 첫 번째 이미지 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Loading")
	TObjectPtr<UImage> PrimaryImage;

	// 두 번째 이미지 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Loading")
	TObjectPtr<UImage> SecondaryImage;

	// 로딩 화면 등장 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FadeIn;

	// 로딩 화면 퇴장 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FadeOut;

	// 매핑이 없을 때 사용할 첫 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush DefaultPrimaryImageBrush;

	// 매핑이 없을 때 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush DefaultViewportSecondaryImageBrush;

	// 목적지 맵별 이미지 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	TArray<FPRLoadingScreenImageEntry> MapImageEntries;

private:
	UPROPERTY(Transient)
	FString DestinationMapPackageName;

	UPROPERTY(Transient)
	FName DestinationMapShortName = NAME_None;

};
