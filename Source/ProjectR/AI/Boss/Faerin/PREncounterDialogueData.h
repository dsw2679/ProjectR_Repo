// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PREncounterDialogueData.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EPRFaerinDialogueNodeType : uint8
{
	Dialog,
	Options,
	StageShot,
	Random,
	Switch,
	Event,
	ExecTrigger,
	Exit,
	Unsupported
};

UENUM(BlueprintType)
enum class EPRFaerinDialogueOptionAction : uint8
{
	None,
	Continue,
	OpenOptions,
	OpenQuestions,
	Fight,
	Decline,
	Close,
	Unsupported
};

// Faerin 인카운터 컷신/선택지 대사 한 줄의 표시 데이터를 보관한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueLine
{
	GENERATED_BODY()

	// 대사 라인을 외부 컷신/자막 데이터와 매칭하기 위한 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName LineId = NAME_None;

	// 자막에 표시할 화자 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText SpeakerName;

	// 자막에 표시할 본문 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText SubtitleText;

	// 선택적으로 재생할 음성 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TObjectPtr<USoundBase> Voice = nullptr;

	// 음성이 없거나 짧을 때 보장할 최소 표시 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue", meta = (ClampMin = "0.0"))
	float MinDisplayTime = 1.0f;

	// 호스트가 해당 라인을 스킵할 수 있는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	bool bHostCanSkip = true;
};

// Faerin 원작 EventTree 선택지 한 개의 표시/이동 정보를 보관한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueOption
{
	GENERATED_BODY()

	// DataAsset 안에서 선택지를 추적하기 위한 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName OptionId = NAME_None;

	// UI에 표시할 선택지 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText DisplayText;

	// Fight, Question, Leave, Info, GiveTake 등 원작 선택지 아이콘 키
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName IconKey = NAME_None;

	// 선택 후 이동할 대화 노드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName TargetNodeId = NAME_None;

	// TargetNodeId가 없거나 최종 ExecTrigger일 때 수행할 의미 액션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	EPRFaerinDialogueOptionAction Action = EPRFaerinDialogueOptionAction::None;

	// 원작 추출 JSON에서 온 ObjectPath
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FString OriginalObjectPath;
};

// Faerin 원작 EventTree 노드를 ProjectR 선택 UI가 탐색할 수 있는 단위로 보관한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueNode
{
	GENERATED_BODY()

	// DataAsset 안에서 노드를 찾기 위한 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName NodeId = NAME_None;

	// 원작 EventTree 노드를 런타임 표시/제어 역할 기준으로 축약한 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	EPRFaerinDialogueNodeType NodeType = EPRFaerinDialogueNodeType::Unsupported;

	// 대사 화자 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText SpeakerName;

	// UI에 표시할 대사 본문
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText Text;

	// StageShot에서 전달된 원작 카메라 샷 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName CameraShotId = NAME_None;

	// 원작 Dialog.RefToSoundObject.AssetPathName을 저장한다. 실제 재생은 Director/BP 쪽 브릿지가 담당한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	FSoftObjectPath VoiceEventPath;

	// 원작 Emote ObjectName 기반 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	FName EmoteId = NAME_None;

	// 원작 Emote ObjectPath. 프로젝트 Emote 시스템 연결 전까지 원본 경로를 보존한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	FString EmoteObjectPath;

	// Continue 입력 시 이동할 다음 노드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName NextNodeId = NAME_None;

	// Options 노드 또는 선택지가 붙은 Dialog 노드에서 표시할 선택지 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueOption> Options;

	// ExecTrigger 또는 Exit 노드가 직접 수행할 의미 액션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	EPRFaerinDialogueOptionAction NodeAction = EPRFaerinDialogueOptionAction::None;

	// 원작 ExecTrigger 문자열
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName TriggerName = NAME_None;

	// 원작 추출 JSON에서 온 ObjectPath
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FString OriginalObjectPath;
};

// Faerin 인카운터 대사 묶음을 DataAsset으로 보관한다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinEncounterDialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 지정한 식별자에 해당하는 대화 그래프 노드를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	bool FindDialogueNode(FName NodeId, FPRFaerinDialogueNode& OutNode) const;

	// 대화 그래프가 런타임 표시 가능한 상태인지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	bool HasDialogueGraph() const;

	// C++ 런타임 탐색용 노드 조회 함수
	const FPRFaerinDialogueNode* FindDialogueNodePtr(FName NodeId) const;

public:
	// 최초 조우 컷신에서 재생할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> IntroLines;

	// 상호작용 선택지 UI 진입 시 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> ChoiceLines;

	// 전투 거절 후 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> DeclineLines;

	// 전투 시작 컷신 직전에 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> PreFightLines;

	// 선택 UI가 시작할 원작 대화 그래프 노드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Graph")
	FName RootDialogueNodeId = NAME_None;

	// 원작 EventTree에서 변환된 선택 UI용 대화 그래프
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Graph")
	TArray<FPRFaerinDialogueNode> DialogueNodes;
};
