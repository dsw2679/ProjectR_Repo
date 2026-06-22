// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterChoiceWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Player/PRPlayerController.h"

UPRFaerinEncounterChoiceWidget::UPRFaerinEncounterChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::Modal;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRFaerinEncounterChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindNativeDialogueButtons();
}

void UPRFaerinEncounterChoiceWidget::NativeDestruct()
{
	if (IsValid(EncounterDirector))
	{
		EncounterDirector->ClearDialogueEmotePlaybackLocal(!bSuppressDialogueCameraRestore);
		EncounterDirector->StopDialogueVoiceLocal();
	}
	RestoreDialogueCamera();
	UnbindNativeDialogueButtons();
	Super::NativeDestruct();
}

void UPRFaerinEncounterChoiceWidget::InitializeChoice(APRFaerinEncounterDirector* InDirector)
{
	EncounterDirector = InDirector;
	ActiveDialogueData = IsValid(InDirector) ? InDirector->GetDialogueData() : nullptr;
	CurrentDialogueNodeId = NAME_None;
	bSuppressDialogueCameraRestore = false;
	RebuildDialogueLookup();
	CaptureDialogueCameraRestoreTarget();

	BP_OnChoiceInitialized(InDirector);

	if (IsValid(ActiveDialogueData) && ActiveDialogueData->HasDialogueGraph())
	{
		SetCurrentDialogueNode(ActiveDialogueData->RootDialogueNodeId);
	}
}

void UPRFaerinEncounterChoiceWidget::AdvanceDialogue()
{
	const FPRFaerinDialogueNode* CurrentNode = GetCurrentDialogueNodePtr();
	if (CurrentNode == nullptr)
	{
		if (IsValid(ActiveDialogueData) && ActiveDialogueData->HasDialogueGraph())
		{
			SetCurrentDialogueNode(ActiveDialogueData->RootDialogueNodeId);
		}
		return;
	}

	if (CurrentNode->Options.Num() > 0)
	{
		return;
	}

	if (!CurrentNode->NextNodeId.IsNone())
	{
		SetCurrentDialogueNode(CurrentNode->NextNodeId);
		return;
	}

	ExecuteDialogueAction(CurrentNode->NodeAction);
}

void UPRFaerinEncounterChoiceWidget::SelectDialogueOption(int32 OptionIndex)
{
	const FPRFaerinDialogueNode* CurrentNode = GetCurrentDialogueNodePtr();
	if (CurrentNode == nullptr || !CurrentNode->Options.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FPRFaerinDialogueOption& SelectedOption = CurrentNode->Options[OptionIndex];
	if (!SelectedOption.TargetNodeId.IsNone())
	{
		if (SetCurrentDialogueNode(SelectedOption.TargetNodeId))
		{
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Faerin dialogue option target not found. Option=%s Target=%s Action=%d"),
			*SelectedOption.OptionId.ToString(),
			*SelectedOption.TargetNodeId.ToString(),
			static_cast<int32>(SelectedOption.Action));
	}

	ExecuteDialogueAction(SelectedOption.Action);
}

FReply UPRFaerinEncounterChoiceWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const EPRUIInputAction Action = GetUIInputAction(InKeyEvent.GetKey());
	if (Action == EPRUIInputAction::Confirm)
	{
		const FPRFaerinDialogueNode* CurrentNode = GetCurrentDialogueNodePtr();
		if (CurrentNode == nullptr || CurrentNode->Options.Num() == 0)
		{
			AdvanceDialogue();
			return FReply::Handled();
		}
	}

	if (Action == EPRUIInputAction::Cancel)
	{
		RequestDecline();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPRFaerinEncounterChoiceWidget::RequestFight()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(EncounterDirector.Get()))
	{
		return;
	}

	bSuppressDialogueCameraRestore = true;
	EncounterDirector->ClearDialogueEmotePlaybackLocal(false);
	EncounterDirector->StopDialogueVoiceLocal();
	PlayerController->ServerChooseFaerinEncounterFight(EncounterDirector);
}

void UPRFaerinEncounterChoiceWidget::RequestDecline()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(EncounterDirector.Get()))
	{
		return;
	}

	EncounterDirector->ClearDialogueEmotePlaybackLocal(true);
	EncounterDirector->StopDialogueVoiceLocal();
	RestoreDialogueCamera();
	PlayerController->ServerChooseFaerinEncounterDecline(EncounterDirector);
}

bool UPRFaerinEncounterChoiceWidget::GetCurrentDialogueNode(FPRFaerinDialogueNode& OutNode) const
{
	const FPRFaerinDialogueNode* CurrentNode = GetCurrentDialogueNodePtr();
	if (CurrentNode == nullptr)
	{
		return false;
	}

	OutNode = *CurrentNode;
	return true;
}

TArray<FPRFaerinDialogueOption> UPRFaerinEncounterChoiceWidget::GetCurrentDialogueOptions() const
{
	const FPRFaerinDialogueNode* CurrentNode = GetCurrentDialogueNodePtr();
	if (CurrentNode == nullptr)
	{
		return TArray<FPRFaerinDialogueOption>();
	}

	return CurrentNode->Options;
}

// ===== 대화 그래프 탐색 =====

void UPRFaerinEncounterChoiceWidget::CaptureDialogueCameraRestoreTarget()
{
	PreviousDialogueViewTarget.Reset();
	bHasDialogueCameraRestoreTarget = false;

	const APlayerController* PlayerController = GetOwningPlayer();
	if (!IsValid(PlayerController))
	{
		return;
	}

	AActor* ViewTarget = PlayerController->GetViewTarget();
	if (!IsValid(ViewTarget))
	{
		return;
	}

	PreviousDialogueViewTarget = ViewTarget;
	bHasDialogueCameraRestoreTarget = true;
}

void UPRFaerinEncounterChoiceWidget::ApplyDialogueStageShotCamera(FName CameraShotId)
{
	if (CameraShotId.IsNone())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueCamera] Apply StageShotId=%s"), *CameraShotId.ToString());

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(EncounterDirector.Get()))
	{
		return;
	}

	const FPRFaerinDialogueStageShotCameraBinding* Binding =
		EncounterDirector->FindDialogueStageShotCameraBinding(CameraShotId);
	if (Binding == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueCamera] Missing binding. StageShotId=%s"), *CameraShotId.ToString());
		return;
	}

	if (!IsValid(Binding->CameraActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueCamera] Missing camera actor. StageShotId=%s"), *CameraShotId.ToString());
		return;
	}

	PlayerController->SetViewTargetWithBlend(
		Binding->CameraActor,
		Binding->BlendTime,
		Binding->BlendFunc.GetValue(),
		Binding->BlendExp,
		Binding->bLockOutgoing);
}

void UPRFaerinEncounterChoiceWidget::RestoreDialogueCamera()
{
	if (bSuppressDialogueCameraRestore)
	{
		return;
	}

	if (!bHasDialogueCameraRestoreTarget)
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	AActor* ViewTarget = PreviousDialogueViewTarget.Get();
	if (IsValid(PlayerController) && IsValid(ViewTarget))
	{
		PlayerController->SetViewTargetWithBlend(ViewTarget, 0.15f, VTBlend_Cubic, 2.0f, false);
	}

	PreviousDialogueViewTarget.Reset();
	bHasDialogueCameraRestoreTarget = false;
}

void UPRFaerinEncounterChoiceWidget::BindNativeDialogueButtons()
{
	if (IsValid(ContinueButton))
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleContinueClicked);
		ContinueButton->OnClicked.AddDynamic(this, &ThisClass::HandleContinueClicked);
	}

	if (IsValid(OptionButton0))
	{
		OptionButton0->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton0Clicked);
		OptionButton0->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton0Clicked);
	}

	if (IsValid(OptionButton1))
	{
		OptionButton1->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton1Clicked);
		OptionButton1->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton1Clicked);
	}

	if (IsValid(OptionButton2))
	{
		OptionButton2->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton2Clicked);
		OptionButton2->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton2Clicked);
	}

	if (IsValid(OptionButton3))
	{
		OptionButton3->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton3Clicked);
		OptionButton3->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton3Clicked);
	}

	if (IsValid(OptionButton4))
	{
		OptionButton4->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton4Clicked);
		OptionButton4->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton4Clicked);
	}

	if (IsValid(OptionButton5))
	{
		OptionButton5->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton5Clicked);
		OptionButton5->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton5Clicked);
	}

	if (IsValid(OptionButton6))
	{
		OptionButton6->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton6Clicked);
		OptionButton6->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton6Clicked);
	}

	if (IsValid(OptionButton7))
	{
		OptionButton7->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton7Clicked);
		OptionButton7->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton7Clicked);
	}

	if (IsValid(OptionButton8))
	{
		OptionButton8->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton8Clicked);
		OptionButton8->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton8Clicked);
	}

	if (IsValid(OptionButton9))
	{
		OptionButton9->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton9Clicked);
		OptionButton9->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton9Clicked);
	}

	if (IsValid(OptionButton10))
	{
		OptionButton10->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton10Clicked);
		OptionButton10->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton10Clicked);
	}

	if (IsValid(OptionButton11))
	{
		OptionButton11->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton11Clicked);
		OptionButton11->OnClicked.AddDynamic(this, &ThisClass::HandleOptionButton11Clicked);
	}
}

void UPRFaerinEncounterChoiceWidget::UnbindNativeDialogueButtons()
{
	if (IsValid(ContinueButton))
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleContinueClicked);
	}

	if (IsValid(OptionButton0))
	{
		OptionButton0->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton0Clicked);
	}

	if (IsValid(OptionButton1))
	{
		OptionButton1->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton1Clicked);
	}

	if (IsValid(OptionButton2))
	{
		OptionButton2->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton2Clicked);
	}

	if (IsValid(OptionButton3))
	{
		OptionButton3->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton3Clicked);
	}

	if (IsValid(OptionButton4))
	{
		OptionButton4->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton4Clicked);
	}

	if (IsValid(OptionButton5))
	{
		OptionButton5->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton5Clicked);
	}

	if (IsValid(OptionButton6))
	{
		OptionButton6->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton6Clicked);
	}

	if (IsValid(OptionButton7))
	{
		OptionButton7->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton7Clicked);
	}

	if (IsValid(OptionButton8))
	{
		OptionButton8->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton8Clicked);
	}

	if (IsValid(OptionButton9))
	{
		OptionButton9->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton9Clicked);
	}

	if (IsValid(OptionButton10))
	{
		OptionButton10->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton10Clicked);
	}

	if (IsValid(OptionButton11))
	{
		OptionButton11->OnClicked.RemoveDynamic(this, &ThisClass::HandleOptionButton11Clicked);
	}
}

void UPRFaerinEncounterChoiceWidget::RefreshNativeDialogueView(const FPRFaerinDialogueNode& Node)
{
	if (IsValid(SpeakerText))
	{
		SpeakerText->SetText(Node.SpeakerName);
		SetWidgetVisible(SpeakerText, !Node.SpeakerName.IsEmpty());
	}

	if (IsValid(BodyText))
	{
		BodyText->SetText(Node.Text);
		SetWidgetVisible(BodyText, !Node.Text.IsEmpty());
	}

	if (IsValid(ContinueText) && ContinueText->GetText().IsEmpty())
	{
		ContinueText->SetText(NSLOCTEXT("FaerinEncounterDialogue", "Continue", "Continue"));
	}

	const bool bHasOptions = Node.Options.Num() > 0;
	SetWidgetVisible(ChoicePanel, bHasOptions);
	SetWidgetVisible(ContinueButton, !bHasOptions);

	const TArray<UButton*> Buttons = GetOptionButtons();
	const TArray<UTextBlock*> Texts = GetOptionTexts();
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		const bool bUseOption = Node.Options.IsValidIndex(Index);
		SetWidgetVisible(Buttons[Index], bUseOption);

		if (bUseOption && Texts.IsValidIndex(Index) && IsValid(Texts[Index]))
		{
			Texts[Index]->SetText(Node.Options[Index].DisplayText);
		}
	}

	if (Node.Options.Num() > Buttons.Num())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Faerin dialogue option count exceeds native slots. Node=%s Options=%d Slots=%d"),
			*Node.NodeId.ToString(),
			Node.Options.Num(),
			Buttons.Num());
	}
}

void UPRFaerinEncounterChoiceWidget::SetWidgetVisible(UWidget* Widget, bool bVisible) const
{
	if (!IsValid(Widget))
	{
		return;
	}

	Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

TArray<UButton*> UPRFaerinEncounterChoiceWidget::GetOptionButtons() const
{
	return {
		OptionButton0.Get(),
		OptionButton1.Get(),
		OptionButton2.Get(),
		OptionButton3.Get(),
		OptionButton4.Get(),
		OptionButton5.Get(),
		OptionButton6.Get(),
		OptionButton7.Get(),
		OptionButton8.Get(),
		OptionButton9.Get(),
		OptionButton10.Get(),
		OptionButton11.Get(),
	};
}

TArray<UTextBlock*> UPRFaerinEncounterChoiceWidget::GetOptionTexts() const
{
	return {
		OptionText0.Get(),
		OptionText1.Get(),
		OptionText2.Get(),
		OptionText3.Get(),
		OptionText4.Get(),
		OptionText5.Get(),
		OptionText6.Get(),
		OptionText7.Get(),
		OptionText8.Get(),
		OptionText9.Get(),
		OptionText10.Get(),
		OptionText11.Get(),
	};
}

void UPRFaerinEncounterChoiceWidget::RebuildDialogueLookup()
{
	DialogueNodeIndexById.Reset();

	if (!IsValid(ActiveDialogueData))
	{
		return;
	}

	for (int32 Index = 0; Index < ActiveDialogueData->DialogueNodes.Num(); ++Index)
	{
		const FPRFaerinDialogueNode& Node = ActiveDialogueData->DialogueNodes[Index];
		if (!Node.NodeId.IsNone())
		{
			DialogueNodeIndexById.Add(Node.NodeId, Index);
		}
	}
}

bool UPRFaerinEncounterChoiceWidget::SetCurrentDialogueNode(FName NodeId)
{
	if (NodeId.IsNone())
	{
		return false;
	}

	constexpr int32 MaxAutoAdvanceSteps = 32;
	FName CandidateNodeId = NodeId;
	for (int32 Step = 0; Step < MaxAutoAdvanceSteps; ++Step)
	{
		const FPRFaerinDialogueNode* Node = FindDialogueNode(CandidateNodeId);
		if (Node == nullptr)
		{
			return false;
		}

		CurrentDialogueNodeId = CandidateNodeId;

		if (IsTerminalActionNode(*Node))
		{
			ExecuteDialogueAction(Node->NodeAction);
			return true;
		}

		if (IsValid(EncounterDirector))
		{
			EncounterDirector->HandleDialogueNodePresentedLocal(*Node);
		}

		if (Node->NodeType == EPRFaerinDialogueNodeType::StageShot && !Node->CameraShotId.IsNone())
		{
			ApplyDialogueStageShotCamera(Node->CameraShotId);
			BP_OnDialogueStageShotChanged(Node->CameraShotId);
		}

		if (ShouldAutoAdvanceNode(*Node) && !Node->NextNodeId.IsNone())
		{
			CandidateNodeId = Node->NextNodeId;
			continue;
		}

		RefreshNativeDialogueView(*Node);
		BP_OnDialogueNodeChanged(*Node);

		// 실제로 표시되는 최종 노드만 서버에 보고하여 Gather 전체 하단 자막 송출을 트리거한다.
		// (상호작용자 외 플레이어는 선택 UI 없이 하단 자막만 받는다.)
		if (IsValid(EncounterDirector))
		{
			if (APRPlayerController* PRPC = GetOwningPlayer<APRPlayerController>())
			{
				UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] ChoiceWidget -> server notify node=%s"), *Node->NodeId.ToString());
				PRPC->ServerNotifyFaerinDialogueNodePresented(EncounterDirector, Node->NodeId);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[FaerinSubtitle] ChoiceWidget owning PC null; cannot notify server node=%s"), *Node->NodeId.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[FaerinSubtitle] ChoiceWidget EncounterDirector null; cannot notify server."));
		}

		return true;
	}

	return false;
}

const FPRFaerinDialogueNode* UPRFaerinEncounterChoiceWidget::FindDialogueNode(FName NodeId) const
{
	if (!IsValid(ActiveDialogueData) || NodeId.IsNone())
	{
		return nullptr;
	}

	if (const int32* FoundIndex = DialogueNodeIndexById.Find(NodeId))
	{
		if (ActiveDialogueData->DialogueNodes.IsValidIndex(*FoundIndex))
		{
			return &ActiveDialogueData->DialogueNodes[*FoundIndex];
		}
	}

	return ActiveDialogueData->FindDialogueNodePtr(NodeId);
}

const FPRFaerinDialogueNode* UPRFaerinEncounterChoiceWidget::GetCurrentDialogueNodePtr() const
{
	return FindDialogueNode(CurrentDialogueNodeId);
}

void UPRFaerinEncounterChoiceWidget::ExecuteDialogueAction(EPRFaerinDialogueOptionAction Action)
{
	switch (Action)
	{
	case EPRFaerinDialogueOptionAction::Fight:
		RequestFight();
		break;
	case EPRFaerinDialogueOptionAction::Decline:
	case EPRFaerinDialogueOptionAction::Close:
		RequestDecline();
		break;
	case EPRFaerinDialogueOptionAction::None:
	case EPRFaerinDialogueOptionAction::Continue:
	case EPRFaerinDialogueOptionAction::OpenOptions:
	case EPRFaerinDialogueOptionAction::OpenQuestions:
	case EPRFaerinDialogueOptionAction::Unsupported:
	default:
		break;
	}
}

bool UPRFaerinEncounterChoiceWidget::ShouldAutoAdvanceNode(const FPRFaerinDialogueNode& Node) const
{
	switch (Node.NodeType)
	{
	case EPRFaerinDialogueNodeType::StageShot:
	case EPRFaerinDialogueNodeType::Random:
	case EPRFaerinDialogueNodeType::Switch:
	case EPRFaerinDialogueNodeType::Event:
	case EPRFaerinDialogueNodeType::Unsupported:
		return Node.Options.Num() == 0;
	case EPRFaerinDialogueNodeType::ExecTrigger:
		return Node.NodeAction == EPRFaerinDialogueOptionAction::Continue
			|| Node.NodeAction == EPRFaerinDialogueOptionAction::OpenOptions
			|| Node.NodeAction == EPRFaerinDialogueOptionAction::OpenQuestions;
	case EPRFaerinDialogueNodeType::Dialog:
	case EPRFaerinDialogueNodeType::Options:
	case EPRFaerinDialogueNodeType::Exit:
	default:
		return false;
	}
}

bool UPRFaerinEncounterChoiceWidget::IsTerminalActionNode(const FPRFaerinDialogueNode& Node) const
{
	return (Node.NodeType == EPRFaerinDialogueNodeType::ExecTrigger || Node.NodeType == EPRFaerinDialogueNodeType::Exit)
		&& (Node.NodeAction == EPRFaerinDialogueOptionAction::Fight
			|| Node.NodeAction == EPRFaerinDialogueOptionAction::Decline
			|| Node.NodeAction == EPRFaerinDialogueOptionAction::Close);
}

void UPRFaerinEncounterChoiceWidget::HandleContinueClicked()
{
	AdvanceDialogue();
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton0Clicked()
{
	SelectDialogueOption(0);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton1Clicked()
{
	SelectDialogueOption(1);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton2Clicked()
{
	SelectDialogueOption(2);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton3Clicked()
{
	SelectDialogueOption(3);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton4Clicked()
{
	SelectDialogueOption(4);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton5Clicked()
{
	SelectDialogueOption(5);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton6Clicked()
{
	SelectDialogueOption(6);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton7Clicked()
{
	SelectDialogueOption(7);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton8Clicked()
{
	SelectDialogueOption(8);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton9Clicked()
{
	SelectDialogueOption(9);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton10Clicked()
{
	SelectDialogueOption(10);
}

void UPRFaerinEncounterChoiceWidget::HandleOptionButton11Clicked()
{
	SelectDialogueOption(11);
}
