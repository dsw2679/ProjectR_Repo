// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AI/Boss/Faerin/PREncounterDialogueData.h"
#include "TimerManager.h"
#include "PRFaerinEncounterDirector.generated.h"

class ALevelSequenceActor;
class APRFaerinEncounterBoundaryActor;
class APRPlayerCharacter;
class APRPlayerController;
class UAnimationAsset;
class UAnimSequenceBase;
class UAudioComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class USceneComponent;

UENUM(BlueprintType)
enum class EFaerinEncounterState : uint8
{
	PreIntro,
	IntroCutsceneDialogue,
	NegotiationIdle,
	ChoiceDialogue,
	Declined,
	PreFightCutscene,
	CombatStarting,
	Combat,
	WipeReset,
	Defeated
};

UENUM(BlueprintType)
enum class EFaerinEncounterSequence : uint8
{
	Intro,
	FightStart
};

// Faerin 컷신 진입부터 실제 전투 보스 스폰까지의 인카운터 상태를 서버 권위로 관리한다.
// 협상 대기 상태에서 계속 보여야 하는 연출 Actor와 idle 애니메이션을 묶는다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPresentationIdleBinding
{
	GENERATED_BODY()

	// idle loop를 적용할 연출용 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<AActor> Actor = nullptr;

	// 해당 Actor의 첫 SkeletalMeshComponent에 재생할 idle 애니메이션
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<UAnimSequenceBase> IdleAnimation = nullptr;

	// idle 애니메이션 반복 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bLoop = true;
};

// 원작 StageShot ID와 실제 카메라 Actor를 연결하기 위한 편집용 매핑이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueStageShotCameraBinding
{
	GENERATED_BODY()

	// 원작 CameraShotDetails.SequenceShotNameID
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	FName CameraShotId = NAME_None;

	// StageShot 전환에 사용할 카메라 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	TObjectPtr<AActor> CameraActor = nullptr;

	// StageShot 카메라 전환 블렌드 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera", meta = (ClampMin = "0.0"))
	float BlendTime = 0.15f;

	// StageShot 카메라 전환 블렌드 함수
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunc = VTBlend_Cubic;

	// StageShot 카메라 전환 블렌드 지수
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera", meta = (ClampMin = "0.0"))
	float BlendExp = 2.0f;

	// StageShot 카메라 전환 시 기존 카메라를 잠글지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	bool bLockOutgoing = false;
};

// Intro 종료 뒤 마지막 연출 애니메이션을 그대로 loop하기 위한 Actor별 바인딩이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPresentationTailLoopBinding
{
	GENERATED_BODY()

	// tail loop를 적용할 연출용 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<AActor> Actor = nullptr;

	// Intro 마지막에 이어서 반복 재생할 애니메이션
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<UAnimSequenceBase> LoopAnimation = nullptr;

	// loop 시작 위치. Intro 마지막 pose와 맞는 시간을 에디터에서 지정한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation", meta = (ClampMin = "0.0"))
	float StartPositionSeconds = 0.0f;

	// tail 애니메이션 반복 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bLoop = true;

	// 애니메이션 적용 후 Actor/Mesh transform을 적용 전 값으로 되돌릴지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bPreserveActorTransform = true;
};

// Intro/FightStart LevelSequence와 별도로 런타임에서 재생할 음성 cue 설정이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinSequenceVoiceCue
{
	GENERATED_BODY()

	// 음성을 재생할 인카운터 시퀀스
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Sequence|Voice")
	EFaerinEncounterSequence SequenceType = EFaerinEncounterSequence::Intro;

	// 현재 프로젝트 SoundWave/SoundCue 경로
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Sequence|Voice")
	FSoftObjectPath SoundPath;

	// 시퀀스 시작 후 재생할 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Sequence|Voice", meta = (ClampMin = "0.0"))
	float StartTimeSeconds = 0.0f;

	// 로그와 식별용 cue 이름
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Sequence|Voice")
	FName CueId = NAME_None;

	// true면 DialogueVoiceActor/Cine_Faerin에 부착하고, false면 해당 위치에서 재생한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Sequence|Voice")
	bool bAttachToVoiceActor = true;
};

UCLASS(Blueprintable)
class PROJECTR_API APRFaerinEncounterDirector : public AActor
{
	GENERATED_BODY()

public:
	APRFaerinEncounterDirector();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Gather 영역에 들어온 플레이어를 등록하고, 필요한 인원이 모두 모이면 Intro를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void NotifyPlayerEnteredGather(APRPlayerCharacter* Player);

	// Combat 중 arena에 늦게 들어온 플레이어를 전투 위협 후보와 경계 잠금에 추가한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void NotifyPlayerEnteredCombatArena(APRPlayerCharacter* Player);

	// 현재 플레이어가 전투 선택 대화를 열 수 있는지 확인한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool CanOpenChoiceDialogue(APRPlayerCharacter* Player) const;

	// 클라이언트 상호작용 힌트 표시용 예측 조건을 확인한다. 서버 최종 검증은 CanOpenChoiceDialogue가 담당한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool CanShowChoiceDialoguePrompt(APRPlayerCharacter* Player) const;

	// 선택 대화 상태로 진입하고 선택권자를 기록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void StartChoiceDialogue(APRPlayerCharacter* Player);

	// 선택권자가 전투 시작을 선택했을 때 pre-fight 컷신과 보스 스폰 흐름을 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ChooseFight(APRPlayerCharacter* Player);

	// 선택권자가 전투를 거절했을 때 협상 대기 상태로 되돌릴 수 있는 상태를 기록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ChooseDecline(APRPlayerCharacter* Player);

	// 파티 전멸 확정 시 실제 전투 보스를 정리하고 리셋 상태로 진입한다.
	UFUNCTION()
	void HandlePartyWipeConfirmed();

	// 파티 리스폰 완료 시 Intro 없이 협상 상태로 복구한다.
	UFUNCTION()
	void HandlePartyRespawned();

	// Faerin 처치 완료 상태로 전환하고 경계 잠금을 해제한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void MarkDefeated();

	// 현재 인카운터 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	EFaerinEncounterState GetCurrentState() const { return CurrentState; }

	// 현재 인카운터 선택 UI가 사용할 대화 DataAsset을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	UPRFaerinEncounterDialogueData* GetDialogueData() const { return DialogueData; }

	// 로컬 선택 UI가 새 대화 노드를 표시했을 때 연출용 메타데이터를 BP로 전달한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void HandleDialogueNodePresentedLocal(const FPRFaerinDialogueNode& Node);

	// 대화 StageShot ID에 대응하는 카메라 설정을 찾는다.
	const FPRFaerinDialogueStageShotCameraBinding* FindDialogueStageShotCameraBinding(FName CameraShotId) const;

	// 로컬 대화 이모트 재생 타이머를 정리하고 필요하면 기본 idle/tail loop로 복귀한다.
	void ClearDialogueEmotePlaybackLocal(bool bRestoreIdle);

	// 로컬 대화 음성 재생을 중지한다.
	void StopDialogueVoiceLocal();

	// === Gather 대상 단일화 (자막/시퀀스/재도전 이동이 공통으로 사용) ===
	// 현재 Gather(arena) 안에 있는 유효 플레이어 목록을 반환한다.
	void GetGatherPlayers(TArray<APRPlayerCharacter*>& OutPlayers) const;

	// Gather 플레이어의 PlayerController 목록을 반환한다(중복 제거).
	void GetGatherPlayerControllers(TArray<APRPlayerController*>& OutControllers) const;

	// === Intro/FightStart 시퀀스 ===
	// PlayerController Client RPC에서 호출되는 로컬 시퀀스 재생 wrapper.
	void PlayEncounterSequenceForLocalAudience(EFaerinEncounterSequence SequenceType);

	// PlayerController Client RPC에서 호출되는 로컬 시퀀스 중단 wrapper.
	void StopEncounterSequenceForLocalAudience(FName Reason);

	// 서버에서 Gather 대상 PlayerController에만 시퀀스 재생 Client RPC를 보낸다.
	void PlayEncounterSequenceForGatherPlayers(EFaerinEncounterSequence SequenceType);

	// 명시한 플레이어 목록(입력 잠금 대상과 동일)에게만 시퀀스 재생 Client RPC를 보낸다.
	// Intro/FightStart는 잠금 대상과 재생 대상이 반드시 같아야 하므로 Gather를 재계산하지 않고 이 함수를 쓴다.
	void PlayEncounterSequenceForPlayers(const TArray<APRPlayerCharacter*>& Players, EFaerinEncounterSequence SequenceType);

	// === Faerin 하단 자막 ===
	// 해당 노드가 Faerin(NPC) 발화 노드인지 판정한다.
	bool IsFaerinSpokenDialogueNode(const FPRFaerinDialogueNode& Node) const;

	// NodeId로 자막 화자/본문을 해석한다. Faerin 발화가 아니면 false.
	bool ResolveDialogueSubtitleText(FName DialogueNodeId, FText& OutSpeakerText, FText& OutBodyText) const;

	// 상호작용자 클라가 보고한 현재 노드를 검증하고 Gather 자막을 송출한다.
	void NotifyDialogueNodePresentedFromClient(APRPlayerController* SourceController, FName DialogueNodeId);

	// Gather 플레이어에게 현재 노드 자막을 표시하거나(Faerin) 숨긴다(그 외).
	void BroadcastFaerinSubtitleForDialogueNode(const FPRFaerinDialogueNode& Node);

	// Gather 플레이어 전체의 하단 자막을 숨긴다.
	void HideFaerinSubtitleForGatherPlayers();

	// 지정한 플레이어들의 하단 자막을 숨긴다.
	void HideFaerinSubtitleForPlayers(const TArray<APRPlayerCharacter*>& Players);

	// === Gather 이탈 정리 ===
	// BoundaryActor가 Gather 이탈을 알리면 해당 플레이어의 자막/시퀀스/입력/카메라를 정리한다.
	void NotifyPlayerExitedGather(APRPlayerCharacter* Player);

protected:
	UFUNCTION()
	void OnRep_CurrentState(EFaerinEncounterState PreviousState);

	UFUNCTION()
	void HandleSpawnedCombatBossDestroyed(AActor* DestroyedActor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEncounterSequence(EFaerinEncounterSequence SequenceType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPresentationActorsHidden(bool bPresentationHidden);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyNegotiationPresentationIdle();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPrimeIntroStartPose();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyIntroStartLoopAnimations();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopSequenceVoiceCues(FName Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ReceiveEncounterStateChanged(EFaerinEncounterState PreviousState, EFaerinEncounterState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueNodePresented(const FPRFaerinDialogueNode& Node);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueStageShotChanged(FName CameraShotId);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueVoiceEvent(const FSoftObjectPath& VoiceEventPath);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueEmoteChanged(FName EmoteId, const FString& EmoteObjectPath);

private:
	bool IsEligibleEncounterPlayer(APRPlayerCharacter* Player) const;
	bool ContainsPlayer(const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player) const;
	void AddPlayer(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player);
	void CleanupPlayerSet(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players) const;
	void SetEncounterState(EFaerinEncounterState NewState);
	void BuildIntroRequiredPlayers();
	bool AreAllIntroRequiredPlayersEntered() const;
	void StartIntroSequence();
	void FinishIntroSequence();
	void StartFightSequence();
	void FinishFightSequence();
	void ResetCombatBoss();
	void RestoreNegotiationAfterWipe();
	void SnapshotCombatStartParticipants(APRPlayerCharacter* FallbackPlayer);
	void RecoverFightStartFailure();
	void GetPlayersFromSet(const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, TArray<APRPlayerCharacter*>& OutPlayers) const;
	void SetInputLockedForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bLock) const;
	void SetHUDVisibleForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bVisible) const;
	void RestoreViewTargetForPlayers(const TArray<APRPlayerCharacter*>& Players, float BlendTime, const TCHAR* Reason) const;
	void AlignCombatStartParticipants();
	void AlignRetryGatherPlayersToPlayerPoints(const TArray<APRPlayerCharacter*>& GatherPlayers);
	void ApplySlotTransform(APRPlayerCharacter* Player, const FTransform& SlotTransform) const;
	FTransform ResolveSlotTransform(AActor* SlotActor, const FTransform& FallbackTransform) const;
	AActor* SpawnCombatBossFromSpawner();
	void BindCombatBossDefeat(AActor* Boss);
	void UnbindCombatBossDefeat();
	void SeedCombatBossThreat();
	void AddThreatToCombatBoss(const TArray<APRPlayerCharacter*>& Participants, APRPlayerCharacter* PrimaryTarget, float ThreatAmount);
	float GetSequenceDurationSeconds(ULevelSequence* Sequence, float FallbackSeconds) const;
	void ApplyNegotiationPresentationIdleLocal();
	void ApplyConfiguredNegotiationIdleLocal();
	void ApplyIntroStartLoopAnimationsLocal();
	void ApplyIntroTailAnimationLoopsLocal();
	void PrimeIntroStartPoseLocal();
	void HandleDialogueVoiceEventLocal(const FSoftObjectPath& VoiceEventPath, FName NodeId);
	void HandleDialogueEmoteChangedLocal(FName EmoteId, const FString& EmoteObjectPath);
	void RestoreDialogueEmoteIdleLocal();
	void ClearDialogueEmoteReturnTimerLocal();
	AActor* ResolveDialogueEmoteActor() const;
	void ForceDetachFightStartFaerinPresentationActorLocal(FName Reason);
	AActor* ResolveDialogueVoiceActor() const;
	USceneComponent* ResolveDialogueVoiceAttachComponent(AActor* VoiceActor) const;
	void ScheduleSequenceVoiceCuesLocal(EFaerinEncounterSequence SequenceType);
	void StopSequenceVoiceCuesLocal(FName Reason);
	bool ResolveSequenceSubtitleText(EFaerinEncounterSequence SequenceType, FName CueId, FText& OutSpeakerText, FText& OutBodyText) const;
	void ShowSequenceSubtitleCueLocal(EFaerinEncounterSequence SequenceType, FName CueId);
	void PlaySequenceVoiceCueLocal(
		EFaerinEncounterSequence SequenceType,
		FName CueId,
		FSoftObjectPath SoundPath,
		bool bAttachToVoiceActor);
	void PlayIdleAnimationOnPresentationActor(AActor* Actor, UAnimSequenceBase* Animation, bool bLoop) const;
	void PlaySingleNodeAnimationOnPresentationActor(
		AActor* Actor,
		UAnimationAsset* Animation,
		float StartPositionSeconds,
		bool bLoop,
		bool bPreserveActorTransform) const;
	void PlaySequenceLocal(EFaerinEncounterSequence SequenceType);
	void StopLocalSequences();
	void OpenChoiceUIForPlayer(APRPlayerCharacter* Player);
	void CloseChoiceUIForInstigator(bool bRestoreDefaultBGM = true);

public:
	// 실제 전투 보스를 생성하는 스포너 Actor. IPRBossSpawnProviderInterface 구현체여야 한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<AActor> BossSpawnerActor = nullptr;

	// Faerin 전용 경계 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<APRFaerinEncounterBoundaryActor> BoundaryActor = nullptr;

	// 컷신 staging root. Python/BP 세팅에서 프레젠테이션 Actor 정렬 기준으로 사용한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<AActor> CinematicRootActor = nullptr;

	// 컷신용 Faerin/Throne/RoyalArcher 등 프레젠테이션 Actor 묶음
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TArray<TObjectPtr<AActor>> PresentationActors;

	// 최초 조우 컷신 LevelSequence
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence")
	TObjectPtr<ULevelSequence> IntroSequence = nullptr;

	// 전투 시작 컷신 LevelSequence
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence")
	TObjectPtr<ULevelSequence> FightStartSequence = nullptr;

	// Intro/FightStart 종료 후 플레이어 카메라로 복귀할 때 사용하는 블렌드 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Camera", meta = (ClampMin = "0.0"))
	float IntroCameraRestoreBlendTime = 0.35f;

	// FightStart 종료 후 플레이어 카메라로 복귀할 때 사용하는 블렌드 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Camera", meta = (ClampMin = "0.0"))
	float FightStartCameraRestoreBlendTime = 0.35f;

	// FightStart 첫 평가에서 Faerin 초기 attach/pose를 받은 뒤 분리하기까지의 지연 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence", meta = (ClampMin = "0.0"))
	float FightStartFaerinDetachDelaySeconds = 0.08f;

	// 컷신/선택지 대사 DataAsset
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Dialogue")
	TObjectPtr<UPRFaerinEncounterDialogueData> DialogueData = nullptr;

	// 대화 이모트 AnimSequence를 1회 재생할 Faerin 연출 Actor. 비어 있으면 Cine_Faerin 이름을 가진 PresentationActor를 찾는다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Emote")
	TObjectPtr<AActor> DialogueEmoteActor = nullptr;

	// 대화 음성을 재생할 Faerin 연출 Actor. 비어 있으면 DialogueEmoteActor 또는 Cine_Faerin PresentationActor를 사용한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Voice")
	TObjectPtr<AActor> DialogueVoiceActor = nullptr;

	// Intro/FightStart 시퀀스에 맞춰 런타임에서 재생할 음성 cue 목록
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence|Voice")
	TArray<FPRFaerinSequenceVoiceCue> SequenceVoiceCues;

	// BeginPlay 시점에 Intro 첫 프레임 pose/transform을 미리 평가할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bPrimeIntroStartPoseOnBeginPlay = true;

	// Intro 재생 직전 첫 프레임 pose/transform을 다시 맞출지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bPrimeIntroStartPoseBeforeIntro = true;

	// PreIntro 대기 상태에서 Intro 첫 프레임용 loop 애니메이션을 재생할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bApplyIntroStartLoopBeforeIntro = true;

	// Intro 종료 후 별도 idle 대신 Intro tail 애니메이션을 우선 loop할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bUseIntroTailLoopAfterIntro = true;

	// Intro 첫 프레임 pose와 이어지는 Actor별 PreIntro loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	TArray<FPRFaerinPresentationTailLoopBinding> IntroStartLoopBindings;

	// Intro 마지막 pose와 이어지는 Actor별 tail loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	TArray<FPRFaerinPresentationTailLoopBinding> IntroTailLoopBindings;

	// Intro 종료 후 NegotiationIdle에서 보여줄 연출 Actor별 idle loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Idle")
	TArray<FPRFaerinPresentationIdleBinding> NegotiationIdleBindings;

	// NegotiationIdle 진입/복구 시 idle loop를 자동 적용할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Idle")
	bool bApplyNegotiationIdleAfterIntro = true;

	// 원작 StageShot ID와 실제 카메라 Actor 편집용 매핑
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	TArray<FPRFaerinDialogueStageShotCameraBinding> DialogueStageShotCameraBindings;

	// 선택 플레이어 기준 왼쪽 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotLeftActor = nullptr;

	// 선택 플레이어 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotCenterActor = nullptr;

	// 선택 플레이어 기준 오른쪽 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotRightActor = nullptr;

	// 슬롯 Actor가 비어 있을 때 사용할 왼쪽 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotLeft;

	// 슬롯 Actor가 비어 있을 때 사용할 중앙 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotCenter;

	// 슬롯 Actor가 비어 있을 때 사용할 오른쪽 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotRight;

	// Sequence 길이를 읽지 못할 때 사용할 Intro fallback 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence", meta = (ClampMin = "0.0"))
	float IntroSequenceFallbackSeconds = 0.0f;

	// Sequence 길이를 읽지 못할 때 사용할 전투 시작 fallback 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence", meta = (ClampMin = "0.0"))
	float FightStartSequenceFallbackSeconds = 0.0f;

	// 전투 시작 시 참가자에게 주입할 초기 위협량
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Threat", meta = (ClampMin = "0.0"))
	float InitialEncounterThreat = 1000.0f;

	// 전투 중 늦게 진입한 플레이어에게 사용할 보조 위협량
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Threat", meta = (ClampMin = "0.0"))
	float LateJoinThreat = 100.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, VisibleInstanceOnly, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	EFaerinEncounterState CurrentState = EFaerinEncounterState::PreIntro;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedCombatBoss = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APRPlayerCharacter> ChoiceInstigator = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelSequencePlayer>> ActiveSequencePlayers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALevelSequenceActor>> ActiveSequenceActors;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveDialogueVoiceAudioComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveSequenceVoiceAudioComponents;

	TSet<TWeakObjectPtr<APRPlayerCharacter>> IntroRequiredPlayers;
	TSet<TWeakObjectPtr<APRPlayerCharacter>> IntroEnteredPlayers;
	TSet<TWeakObjectPtr<APRPlayerCharacter>> CombatStartParticipants;

	FTimerHandle IntroSequenceTimerHandle;
	FTimerHandle FightStartSequenceTimerHandle;
	FTimerHandle DialogueEmoteReturnTimerHandle;
	TArray<FTimerHandle> SequenceVoiceTimerHandles;

	bool bResettingCombatBoss = false;
	bool bCombatBossDefeated = false;
	bool bDialogueEmotePlaying = false;

	// Intro 최초 정상 종료 시점에 캡처한 Faerin 연출 액터의 월드 transform(재도전 복원 참조용).
	UPROPERTY(Transient)
	FTransform CapturedPostIntroFaerinTransform = FTransform::Identity;

	// PostIntro transform 캡처 완료 여부.
	UPROPERTY(Transient)
	bool bCapturedPostIntroFaerinTransform = false;

public:
	// 재도전 복원 시 캡처한 PostIntro transform을 강제 적용할지 여부.
	// 사용자가 attach/detach를 직접 처리 중이므로 기본 false. 충돌 확인 후 켠다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss|Faerin|Encounter|Retry")
	bool bRestoreCapturedPostIntroTransformAfterWipe = false;
};
