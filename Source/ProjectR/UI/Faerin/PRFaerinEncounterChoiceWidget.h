// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Boss/Faerin/PREncounterDialogueData.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRFaerinEncounterChoiceWidget.generated.h"

class AActor;
class APRFaerinEncounterDirector;
class UButton;
class UPRFaerinEncounterDialogueData;
class UTextBlock;
class UWidget;

// Faerin 협상 선택지를 owning client에서 표시하고 서버 선택 요청을 전달한다.
UCLASS(Blueprintable)
class PROJECTR_API UPRFaerinEncounterChoiceWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRFaerinEncounterChoiceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 선택 UI가 사용할 Director 컨텍스트를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void InitializeChoice(APRFaerinEncounterDirector* InDirector);

	// 현재 대화 노드를 다음 노드로 진행한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	void AdvanceDialogue();

	// 현재 대화 노드의 선택지를 index 기준으로 선택한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	void SelectDialogueOption(int32 OptionIndex);

	// 전투 시작 선택을 서버에 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RequestFight();

	// 전투 거절 선택을 서버에 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RequestDecline();

	// 현재 표시 중인 대화 노드를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	bool GetCurrentDialogueNode(FPRFaerinDialogueNode& OutNode) const;

	// 현재 대화 노드에 표시할 선택지 목록을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueOption> GetCurrentDialogueOptions() const;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	// Blueprint 위젯이 선택지 텍스트와 버튼 상태를 초기화할 수 있도록 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void BP_OnChoiceInitialized(APRFaerinEncounterDirector* InDirector);

	// Blueprint 위젯이 현재 대화 노드 표시를 갱신할 수 있도록 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	void BP_OnDialogueNodeChanged(const FPRFaerinDialogueNode& Node);

	// Blueprint 위젯이 StageShot 카메라 힌트를 별도로 받을 수 있도록 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	void BP_OnDialogueStageShotChanged(FName CameraShotId);

private:
	void BindNativeDialogueButtons();
	void UnbindNativeDialogueButtons();
	void RefreshNativeDialogueView(const FPRFaerinDialogueNode& Node);
	void SetWidgetVisible(UWidget* Widget, bool bVisible) const;
	TArray<UButton*> GetOptionButtons() const;
	TArray<UTextBlock*> GetOptionTexts() const;
	void RebuildDialogueLookup();
	bool SetCurrentDialogueNode(FName NodeId);
	const FPRFaerinDialogueNode* FindDialogueNode(FName NodeId) const;
	const FPRFaerinDialogueNode* GetCurrentDialogueNodePtr() const;
	void ExecuteDialogueAction(EPRFaerinDialogueOptionAction Action);
	bool ShouldAutoAdvanceNode(const FPRFaerinDialogueNode& Node) const;
	bool IsTerminalActionNode(const FPRFaerinDialogueNode& Node) const;
	void CaptureDialogueCameraRestoreTarget();
	void ApplyDialogueStageShotCamera(FName CameraShotId);
	void RestoreDialogueCamera();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleOptionButton0Clicked();

	UFUNCTION()
	void HandleOptionButton1Clicked();

	UFUNCTION()
	void HandleOptionButton2Clicked();

	UFUNCTION()
	void HandleOptionButton3Clicked();

	UFUNCTION()
	void HandleOptionButton4Clicked();

	UFUNCTION()
	void HandleOptionButton5Clicked();

	UFUNCTION()
	void HandleOptionButton6Clicked();

	UFUNCTION()
	void HandleOptionButton7Clicked();

	UFUNCTION()
	void HandleOptionButton8Clicked();

	UFUNCTION()
	void HandleOptionButton9Clicked();

	UFUNCTION()
	void HandleOptionButton10Clicked();

	UFUNCTION()
	void HandleOptionButton11Clicked();

private:
	// 현재 대화 노드의 화자명을 표시하는 TextBlock
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerText = nullptr;

	// 현재 대화 노드의 본문을 표시하는 TextBlock
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText = nullptr;

	// 선택지가 없는 대화 노드를 진행하는 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton = nullptr;

	// ContinueButton 내부 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueText = nullptr;

	// 선택지 버튼 묶음 루트 위젯
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ChoicePanel = nullptr;

	// 대화 선택지 버튼 0
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton0 = nullptr;

	// 대화 선택지 버튼 1
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton1 = nullptr;

	// 대화 선택지 버튼 2
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton2 = nullptr;

	// 대화 선택지 버튼 3
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton3 = nullptr;

	// 대화 선택지 버튼 4
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton4 = nullptr;

	// 대화 선택지 버튼 5
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton5 = nullptr;

	// 대화 선택지 버튼 6
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton6 = nullptr;

	// 대화 선택지 버튼 7
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton7 = nullptr;

	// 대화 선택지 버튼 8
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton8 = nullptr;

	// 대화 선택지 버튼 9
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton9 = nullptr;

	// 대화 선택지 버튼 10
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton10 = nullptr;

	// 대화 선택지 버튼 11
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton11 = nullptr;

	// 대화 선택지 텍스트 0
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText0 = nullptr;

	// 대화 선택지 텍스트 1
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText1 = nullptr;

	// 대화 선택지 텍스트 2
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText2 = nullptr;

	// 대화 선택지 텍스트 3
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText3 = nullptr;

	// 대화 선택지 텍스트 4
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText4 = nullptr;

	// 대화 선택지 텍스트 5
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText5 = nullptr;

	// 대화 선택지 텍스트 6
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText6 = nullptr;

	// 대화 선택지 텍스트 7
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText7 = nullptr;

	// 대화 선택지 텍스트 8
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText8 = nullptr;

	// 대화 선택지 텍스트 9
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText9 = nullptr;

	// 대화 선택지 텍스트 10
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText10 = nullptr;

	// 대화 선택지 텍스트 11
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText11 = nullptr;

	// 선택 결과를 전달할 서버 권위 Director
	UPROPERTY(Transient)
	TObjectPtr<APRFaerinEncounterDirector> EncounterDirector = nullptr;

	// 현재 선택 UI가 탐색 중인 대화 DataAsset
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinEncounterDialogueData> ActiveDialogueData = nullptr;

	// 현재 표시 중이거나 처리 중인 대화 노드 식별자
	UPROPERTY(Transient)
	FName CurrentDialogueNodeId = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousDialogueViewTarget;

	TMap<FName, int32> DialogueNodeIndexById;

	bool bHasDialogueCameraRestoreTarget = false;
	bool bSuppressDialogueCameraRestore = false;
};
