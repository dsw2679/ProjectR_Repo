// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Floating Text UI 위젯 구현)
#include "PRFloatingTextWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "LatentActions.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFloatingTextWidget, Log, All);

void UPRFloatingTextWidget::InitFloatingText(const FPRFloatingTextParams& Params)
{
	SetText(Params.Text);
	SetTextColor(Params.Color);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRFloatingTextWidget::SetText(const FText& InText)
{
	if (IsValid(ValueText))
	{
		ValueText->SetText(InText);
	}
}

void UPRFloatingTextWidget::SetTextColor(FLinearColor InColor)
{
	if (IsValid(ValueText))
	{
		ValueText->SetColorAndOpacity(FSlateColor(InColor));
	}
}

void UPRFloatingTextWidget::ResetFloatingTextForReuse()
{
	bWaitingPlayAnimationFinished = false;
	bFinishNotified = true;

	ClearFinishFallbackTimer();

	// 이전 풀 사용에서 남은 UMG 애니메이션 제거
	StopAllAnimations();

	// 이전 BP OnPlay에서 시작된 Delay latent action 제거
	if (UWorld* World = GetWorld(); IsValid(World))
	{
		World->GetLatentActionManager().RemoveActionsForObject(this);
	}

	// 애니메이션이 변경한 위젯 렌더 상태 기본값 복원
	SetRenderOpacity(1.0f);
	SetRenderTransform(FWidgetTransform());
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] ResetForReuse Widget=%s Class=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()));
}

void UPRFloatingTextWidget::PlayFloatingText()
{
	ClearFinishFallbackTimer();

	bFinishNotified = false;
	bWaitingPlayAnimationFinished = false;

	if (!IsValid(PlayAnim))
	{
		// PlayAnim 미설정 위젯의 최소 생명주기 보장
		OnPlay();

		if (UWorld* World = GetWorld(); IsValid(World))
		{
			World->GetTimerManager().SetTimer(
				FinishFallbackTimerHandle,
				this,
				&ThisClass::HandleFinishFallbackElapsed,
				NoAnimationFallbackSeconds,
				false);
		}

		UE_LOG(LogPRFloatingTextWidget, Warning, TEXT("[FloatingText] PlayWithoutAnimation Widget=%s Class=%s Fallback=%.3f"),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()),
			NoAnimationFallbackSeconds);
		return;
	}

	// 이전 재생 위치 제거
	StopAnimation(PlayAnim);

	const float AnimationEndTime = FMath::Max(PlayAnim->GetEndTime(), 0.0f);
	const float FallbackSeconds = AnimationEndTime + FinishFallbackPaddingSeconds;

	bWaitingPlayAnimationFinished = true;
	PlayAnimation(PlayAnim, 0.0f, 1);

	if (UWorld* World = GetWorld(); IsValid(World) && FallbackSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			FinishFallbackTimerHandle,
			this,
			&ThisClass::HandleFinishFallbackElapsed,
			FallbackSeconds,
			false);
	}

	UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] PlayAnimation Widget=%s Class=%s Anim=%s End=%.3f Fallback=%.3f"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()),
		*GetNameSafe(PlayAnim),
		AnimationEndTime,
		FallbackSeconds);
}

void UPRFloatingTextWidget::OnPlay_Implementation()
{
}

void UPRFloatingTextWidget::FinishFloatingText()
{
	if (bFinishNotified)
	{
		UE_LOG(LogPRFloatingTextWidget, Warning, TEXT("[FloatingText] FinishIgnored Widget=%s Class=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()));
		return;
	}

	bFinishNotified = true;
	bWaitingPlayAnimationFinished = false;
	ClearFinishFallbackTimer();

	UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] Finish Widget=%s Class=%s Bound=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()),
		OnFloatingTextFinished.IsBound() ? 1 : 0);

	OnFloatingTextFinished.ExecuteIfBound(this);
}

void UPRFloatingTextWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(PlayAnim))
	{
		// 풀 위젯의 반복 Construct 대비 기존 완료 콜백 제거
		UnbindAllFromAnimationFinished(PlayAnim);

		FWidgetAnimationDynamicEvent PlayAnimFinishedEvent;
		PlayAnimFinishedEvent.BindDynamic(this, &ThisClass::HandlePlayAnimationFinished);
		BindToAnimationFinished(PlayAnim, PlayAnimFinishedEvent);

		UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] BindAnimationFinished Widget=%s Anim=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayAnim));
	}
	else
	{
		UE_LOG(LogPRFloatingTextWidget, Warning, TEXT("[FloatingText] MissingPlayAnim Widget=%s Class=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()));
	}
}

void UPRFloatingTextWidget::NativeDestruct()
{
	ClearFinishFallbackTimer();

	if (IsValid(PlayAnim))
	{
		UnbindAllFromAnimationFinished(PlayAnim);
	}

	Super::NativeDestruct();
}

void UPRFloatingTextWidget::HandlePlayAnimationFinished()
{
	if (!bWaitingPlayAnimationFinished)
	{
		UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] AnimationFinishedIgnored Widget=%s Class=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()));
		return;
	}

	UE_LOG(LogPRFloatingTextWidget, Log, TEXT("[FloatingText] AnimationFinished Widget=%s Class=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()));

	FinishFloatingText();
}

void UPRFloatingTextWidget::HandleFinishFallbackElapsed()
{
	UE_LOG(LogPRFloatingTextWidget, Warning, TEXT("[FloatingText] FinishFallbackElapsed Widget=%s Class=%s Waiting=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetClass()),
		bWaitingPlayAnimationFinished ? 1 : 0);

	FinishFloatingText();
}

void UPRFloatingTextWidget::ClearFinishFallbackTimer()
{
	if (UWorld* World = GetWorld(); IsValid(World))
	{
		World->GetTimerManager().ClearTimer(FinishFallbackTimerHandle);
	}
}
