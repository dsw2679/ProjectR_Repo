// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterDirector.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "MovieSceneSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AI/Boss/PRBossSpawnProviderInterface.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/World/PRFaerinEncounterBoundaryActor.h"
#include "Sound/SoundBase.h"

namespace
{
FText MakeFaerinDisplayText(const FText& SourceText)
{
	FString DisplayString = SourceText.ToString();
	DisplayString.ReplaceInline(TEXT("페어린"), TEXT("파에린"));
	return FText::FromString(DisplayString);
}
}

APRFaerinEncounterDirector::APRFaerinEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

/*~ AActor Interface ~*/

void APRFaerinEncounterDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bPrimeIntroStartPoseOnBeginPlay)
	{
		PrimeIntroStartPoseLocal();
	}

	if (bApplyIntroStartLoopBeforeIntro)
	{
		ApplyIntroStartLoopAnimationsLocal();
	}

	if (!HasAuthority())
	{
		return;
	}

	if (APRPlayGameMode* GameMode = GetWorld()->GetAuthGameMode<APRPlayGameMode>())
	{
		GameMode->OnPartyWipeConfirmed.AddDynamic(this, &ThisClass::HandlePartyWipeConfirmed);
		GameMode->OnPartyRespawned.AddDynamic(this, &ThisClass::HandlePartyRespawned);
	}
}

void APRFaerinEncounterDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		CloseChoiceUIForInstigator();
	}

	if (HasAuthority())
	{
		if (APRPlayGameMode* GameMode = GetWorld()->GetAuthGameMode<APRPlayGameMode>())
		{
			GameMode->OnPartyWipeConfirmed.RemoveDynamic(this, &ThisClass::HandlePartyWipeConfirmed);
			GameMode->OnPartyRespawned.RemoveDynamic(this, &ThisClass::HandlePartyRespawned);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntroSequenceTimerHandle);
		World->GetTimerManager().ClearTimer(FightStartSequenceTimerHandle);
	}
	ClearDialogueEmotePlaybackLocal(false);
	StopDialogueVoiceLocal();
	StopSequenceVoiceCuesLocal(TEXT("EndPlay"));

	StopLocalSequences();
	Super::EndPlay(EndPlayReason);
}

void APRFaerinEncounterDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinEncounterDirector, CurrentState);
}

// ===== 공개 함수 =====

void APRFaerinEncounterDirector::NotifyPlayerEnteredGather(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	if (CurrentState == EFaerinEncounterState::Combat)
	{
		NotifyPlayerEnteredCombatArena(Player);
		return;
	}

	if (CurrentState != EFaerinEncounterState::PreIntro)
	{
		return;
	}

	if (IntroRequiredPlayers.Num() == 0)
	{
		BuildIntroRequiredPlayers();
	}

	AddPlayer(IntroEnteredPlayers, Player);
	if (AreAllIntroRequiredPlayersEntered())
	{
		StartIntroSequence();
	}
}

void APRFaerinEncounterDirector::NotifyPlayerEnteredCombatArena(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::Combat || !IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	const bool bAlreadyTracked = ContainsPlayer(CombatStartParticipants, Player);
	AddPlayer(CombatStartParticipants, Player);

	if (bAlreadyTracked)
	{
		return;
	}

	TArray<APRPlayerCharacter*> LatePlayers;
	LatePlayers.Add(Player);
	AddThreatToCombatBoss(LatePlayers, Player, LateJoinThreat);
}

bool APRFaerinEncounterDirector::CanOpenChoiceDialogue(APRPlayerCharacter* Player) const
{
	if (!IsEligibleEncounterPlayer(Player))
	{
		return false;
	}

	const bool bChoiceState =
		CurrentState == EFaerinEncounterState::NegotiationIdle ||
		CurrentState == EFaerinEncounterState::Declined;
	if (!bChoiceState)
	{
		return false;
	}

	if (IsValid(BoundaryActor)
		&& !BoundaryActor->IsPlayerInsideArena(Player)
		&& !BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
	{
		return false;
	}

	return true;
}

bool APRFaerinEncounterDirector::CanShowChoiceDialoguePrompt(APRPlayerCharacter* Player) const
{
	if (!IsEligibleEncounterPlayer(Player))
	{
		return false;
	}

	const bool bChoiceState =
		CurrentState == EFaerinEncounterState::NegotiationIdle ||
		CurrentState == EFaerinEncounterState::Declined;
	if (!bChoiceState)
	{
		return false;
	}

	if (IsValid(BoundaryActor) && !BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
	{
		return false;
	}

	return true;
}

void APRFaerinEncounterDirector::StartChoiceDialogue(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !CanOpenChoiceDialogue(Player))
	{
		return;
	}

	ChoiceInstigator = Player;
	SetEncounterState(EFaerinEncounterState::ChoiceDialogue);
	OpenChoiceUIForPlayer(Player);
}

void APRFaerinEncounterDirector::ChooseFight(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::ChoiceDialogue)
	{
		return;
	}

	if (IsValid(ChoiceInstigator) && ChoiceInstigator != Player)
	{
		return;
	}

	if (!IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	CloseChoiceUIForInstigator(false);
	SnapshotCombatStartParticipants(Player);

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	if (Participants.Num() == 0)
	{
		return;
	}

	HideFaerinSubtitleForPlayers(Participants);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::PreFight);
	}

	SetInputLockedForPlayers(Participants, true);
	AlignCombatStartParticipants();
	StartFightSequence();
}

void APRFaerinEncounterDirector::ChooseDecline(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::ChoiceDialogue)
	{
		return;
	}

	if (IsValid(ChoiceInstigator) && ChoiceInstigator != Player)
	{
		return;
	}

	ChoiceInstigator = Player;
	CloseChoiceUIForInstigator();

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	SetInputLockedForPlayers(Participants, false);
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();
	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	SetEncounterState(EFaerinEncounterState::Declined);
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::HandlePartyWipeConfirmed()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState != EFaerinEncounterState::Combat && CurrentState != EFaerinEncounterState::CombatStarting)
	{
		return;
	}

	CloseChoiceUIForInstigator();
	SetEncounterState(EFaerinEncounterState::WipeReset);
	ResetCombatBoss();
}

void APRFaerinEncounterDirector::HandlePartyRespawned()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState != EFaerinEncounterState::WipeReset && CurrentState != EFaerinEncounterState::Combat)
	{
		return;
	}

	RestoreNegotiationAfterWipe();
}

void APRFaerinEncounterDirector::MarkDefeated()
{
	if (!HasAuthority())
	{
		return;
	}

	bCombatBossDefeated = true;
	UnbindCombatBossDefeat();
	CloseChoiceUIForInstigator();
	SetEncounterState(EFaerinEncounterState::Defeated);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Defeated);
		BoundaryActor->ClearEncounterState();
	}

	MulticastSetPresentationActorsHidden(true);
}

// ===== 복제/멀티캐스트 =====

void APRFaerinEncounterDirector::HandleDialogueNodePresentedLocal(const FPRFaerinDialogueNode& Node)
{
	if (Node.NodeType == EPRFaerinDialogueNodeType::StageShot && !Node.CameraShotId.IsNone())
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueCamera] StageShotId=%s"), *Node.CameraShotId.ToString());
		ReceiveDialogueStageShotChanged(Node.CameraShotId);
	}

	if (Node.NodeType == EPRFaerinDialogueNodeType::Dialog)
	{
		HandleDialogueVoiceEventLocal(Node.VoiceEventPath, Node.NodeId);
		if (Node.VoiceEventPath.IsValid())
		{
			ReceiveDialogueVoiceEvent(Node.VoiceEventPath);
		}

		if (!Node.EmoteId.IsNone() || !Node.EmoteObjectPath.IsEmpty())
		{
			HandleDialogueEmoteChangedLocal(Node.EmoteId, Node.EmoteObjectPath);
			ReceiveDialogueEmoteChanged(Node.EmoteId, Node.EmoteObjectPath);
		}
	}
	else
	{
		StopDialogueVoiceLocal();
	}

	ReceiveDialogueNodePresented(Node);
}

const FPRFaerinDialogueStageShotCameraBinding* APRFaerinEncounterDirector::FindDialogueStageShotCameraBinding(
	FName CameraShotId) const
{
	if (CameraShotId.IsNone())
	{
		return nullptr;
	}

	for (const FPRFaerinDialogueStageShotCameraBinding& Binding : DialogueStageShotCameraBindings)
	{
		if (Binding.CameraShotId == CameraShotId)
		{
			return &Binding;
		}
	}

	return nullptr;
}

void APRFaerinEncounterDirector::ClearDialogueEmotePlaybackLocal(bool bRestoreIdle)
{
	ClearDialogueEmoteReturnTimerLocal();

	if (bRestoreIdle && bDialogueEmotePlaying)
	{
		RestoreDialogueEmoteIdleLocal();
		return;
	}

	bDialogueEmotePlaying = false;
}

void APRFaerinEncounterDirector::StopDialogueVoiceLocal()
{
	if (!IsValid(ActiveDialogueVoiceAudioComponent))
	{
		ActiveDialogueVoiceAudioComponent = nullptr;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueVoice] Stop active dialogue voice"));
	ActiveDialogueVoiceAudioComponent->Stop();
	ActiveDialogueVoiceAudioComponent = nullptr;
}

void APRFaerinEncounterDirector::OnRep_CurrentState(EFaerinEncounterState PreviousState)
{
	ReceiveEncounterStateChanged(PreviousState, CurrentState);
}

void APRFaerinEncounterDirector::HandleSpawnedCombatBossDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != SpawnedCombatBoss.Get())
	{
		return;
	}

	UnbindCombatBossDefeat();
	SpawnedCombatBoss = nullptr;

	if (bResettingCombatBoss || bCombatBossDefeated || CurrentState != EFaerinEncounterState::Combat)
	{
		return;
	}

	bCombatBossDefeated = true;
	MarkDefeated();
}

void APRFaerinEncounterDirector::MulticastPlayEncounterSequence_Implementation(EFaerinEncounterSequence SequenceType)
{
	PlaySequenceLocal(SequenceType);
}

void APRFaerinEncounterDirector::MulticastSetPresentationActorsHidden_Implementation(bool bPresentationHidden)
{
	if (bPresentationHidden)
	{
		ClearDialogueEmotePlaybackLocal(false);
		StopDialogueVoiceLocal();
		StopSequenceVoiceCuesLocal(TEXT("PresentationHidden"));
	}

	for (AActor* PresentationActor : PresentationActors)
	{
		if (IsValid(PresentationActor))
		{
			PresentationActor->SetActorHiddenInGame(bPresentationHidden);
			PresentationActor->SetActorEnableCollision(false);
			PresentationActor->SetActorTickEnabled(!bPresentationHidden);
		}
	}
}

// ===== 상태/플레이어 관리 =====

void APRFaerinEncounterDirector::MulticastApplyNegotiationPresentationIdle_Implementation()
{
	ApplyNegotiationPresentationIdleLocal();
}

void APRFaerinEncounterDirector::MulticastPrimeIntroStartPose_Implementation()
{
	PrimeIntroStartPoseLocal();
}

void APRFaerinEncounterDirector::MulticastApplyIntroStartLoopAnimations_Implementation()
{
	ApplyIntroStartLoopAnimationsLocal();
}

void APRFaerinEncounterDirector::MulticastStopSequenceVoiceCues_Implementation(FName Reason)
{
	StopSequenceVoiceCuesLocal(Reason);
}

bool APRFaerinEncounterDirector::IsEligibleEncounterPlayer(APRPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return false;
	}

	const APRPlayerState* PlayerState = Player->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		return false;
	}

	return PlayerState->IsCombatParticipant() && !PlayerState->IsOutOfFight();
}

bool APRFaerinEncounterDirector::ContainsPlayer(
	const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return false;
	}

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : Players)
	{
		if (WeakPlayer.Get() == Player)
		{
			return true;
		}
	}

	return false;
}

void APRFaerinEncounterDirector::AddPlayer(
	TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player)
{
	if (IsValid(Player))
	{
		Players.Add(Player);
	}
}

void APRFaerinEncounterDirector::CleanupPlayerSet(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players) const
{
	for (auto It = Players.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void APRFaerinEncounterDirector::SetEncounterState(EFaerinEncounterState NewState)
{
	if (!HasAuthority() || CurrentState == NewState)
	{
		return;
	}

	const EFaerinEncounterState PreviousState = CurrentState;
	CurrentState = NewState;
	ReceiveEncounterStateChanged(PreviousState, CurrentState);
	ForceNetUpdate();
}

void APRFaerinEncounterDirector::BuildIntroRequiredPlayers()
{
	IntroRequiredPlayers.Reset();

	const UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APRPlayerCharacter* Player : GameState->GetPlayerCharacters())
	{
		if (IsEligibleEncounterPlayer(Player))
		{
			AddPlayer(IntroRequiredPlayers, Player);
		}
	}
}

bool APRFaerinEncounterDirector::AreAllIntroRequiredPlayersEntered() const
{
	if (IntroRequiredPlayers.Num() == 0)
	{
		return false;
	}

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : IntroRequiredPlayers)
	{
		APRPlayerCharacter* Player = WeakPlayer.Get();
		if (!IsValid(Player) || !ContainsPlayer(IntroEnteredPlayers, Player))
		{
			return false;
		}
	}

	return true;
}

void APRFaerinEncounterDirector::GetPlayersFromSet(
	const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	TArray<APRPlayerCharacter*>& OutPlayers) const
{
	OutPlayers.Reset();
	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : Players)
	{
		if (APRPlayerCharacter* Player = WeakPlayer.Get())
		{
			OutPlayers.Add(Player);
		}
	}
}

// ===== Gather 대상 단일화 =====

void APRFaerinEncounterDirector::GetGatherPlayers(TArray<APRPlayerCharacter*>& OutPlayers) const
{
	OutPlayers.Reset();

	if (!IsValid(BoundaryActor))
	{
		return;
	}

	TArray<APRPlayerCharacter*> Candidates;
	BoundaryActor->GetArenaInsidePlayers(Candidates);

	for (APRPlayerCharacter* Player : Candidates)
	{
		if (!IsEligibleEncounterPlayer(Player))
		{
			continue;
		}

		// 추적 set과 물리 위치가 어긋난 경우를 방어한다.
		if (!BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
		{
			continue;
		}

		OutPlayers.AddUnique(Player);
	}

	const UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APRPlayerCharacter* Player : GameState->GetPlayerCharacters())
	{
		if (!IsEligibleEncounterPlayer(Player))
		{
			continue;
		}

		if (!BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
		{
			continue;
		}

		OutPlayers.AddUnique(Player);
	}
}

void APRFaerinEncounterDirector::GetGatherPlayerControllers(TArray<APRPlayerController*>& OutControllers) const
{
	OutControllers.Reset();

	TArray<APRPlayerCharacter*> Players;
	GetGatherPlayers(Players);
	for (APRPlayerCharacter* Player : Players)
	{
		if (APRPlayerController* PC = Cast<APRPlayerController>(Player->GetController()))
		{
			OutControllers.AddUnique(PC);
		}
	}
}

// ===== Intro/FightStart 시퀀스 (Gather 대상 로컬 재생) =====

void APRFaerinEncounterDirector::PlayEncounterSequenceForLocalAudience(EFaerinEncounterSequence SequenceType)
{
	PlaySequenceLocal(SequenceType);
}

void APRFaerinEncounterDirector::StopEncounterSequenceForLocalAudience(FName Reason)
{
	StopSequenceVoiceCuesLocal(Reason);
	StopLocalSequences();
}

void APRFaerinEncounterDirector::PlayEncounterSequenceForGatherPlayers(EFaerinEncounterSequence SequenceType)
{
	TArray<APRPlayerCharacter*> GatherPlayers;
	GetGatherPlayers(GatherPlayers);
	PlayEncounterSequenceForPlayers(GatherPlayers, SequenceType);
}

void APRFaerinEncounterDirector::PlayEncounterSequenceForPlayers(
	const TArray<APRPlayerCharacter*>& Players,
	EFaerinEncounterSequence SequenceType)
{
	if (!HasAuthority())
	{
		return;
	}

	int32 AudienceCount = 0;
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		if (APRPlayerController* PC = Cast<APRPlayerController>(Player->GetController()))
		{
			if (PC->IsLocalController())
			{
				PC->PlayFaerinEncounterSequenceLocal(this, SequenceType);
			}
			else
			{
				PC->ClientPlayFaerinEncounterSequence(this, SequenceType);
			}
			++AudienceCount;
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinSequenceAudience] Play sequence=%d audience=%d"),
		static_cast<int32>(SequenceType),
		AudienceCount);
}

// ===== Faerin 하단 자막 =====

bool APRFaerinEncounterDirector::IsFaerinSpokenDialogueNode(const FPRFaerinDialogueNode& Node) const
{
	if (Node.NodeType != EPRFaerinDialogueNodeType::Dialog || Node.Text.IsEmpty())
	{
		return false;
	}

	// SpeakerName만으로는 로컬라이즈 문자열에 흔들릴 수 있고,
	// VoiceEventPath만으로는 음성 없는 Faerin 라인을 놓칠 수 있어 병행 판정한다.
	const FString Speaker = Node.SpeakerName.ToString();
	const FString VoicePath = Node.VoiceEventPath.ToString();

	return Speaker.Contains(TEXT("Faerin"))
		|| Speaker.Contains(TEXT("페어린"))
		|| Speaker.Contains(TEXT("파에린"))
		|| VoicePath.Contains(TEXT("VO_ET_Faerin_"));
}

bool APRFaerinEncounterDirector::ResolveDialogueSubtitleText(
	FName DialogueNodeId,
	FText& OutSpeakerText,
	FText& OutBodyText) const
{
	const FPRFaerinDialogueNode* Node = IsValid(DialogueData) ? DialogueData->FindDialogueNodePtr(DialogueNodeId) : nullptr;
	if (Node == nullptr || !IsFaerinSpokenDialogueNode(*Node))
	{
		return false;
	}

	OutSpeakerText = MakeFaerinDisplayText(Node->SpeakerName);
	OutBodyText = MakeFaerinDisplayText(Node->Text);
	return !OutBodyText.IsEmpty();
}

void APRFaerinEncounterDirector::NotifyDialogueNodePresentedFromClient(
	APRPlayerController* SourceController,
	FName DialogueNodeId)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::ChoiceDialogue)
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] NotifyFromClient rejected auth=%d state=%d node=%s"),
			HasAuthority() ? 1 : 0, static_cast<int32>(CurrentState), *DialogueNodeId.ToString());
		return;
	}

	// 보고자는 반드시 선택권자(상호작용자)여야 한다.
	APRPlayerCharacter* SourcePlayer = IsValid(SourceController) ? Cast<APRPlayerCharacter>(SourceController->GetPawn()) : nullptr;
	if (!IsValid(SourcePlayer) || ChoiceInstigator.Get() != SourcePlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] NotifyFromClient rejected: not instigator (src=%s instigator=%s)"),
			*GetNameSafe(SourcePlayer), *GetNameSafe(ChoiceInstigator.Get()));
		return;
	}

	const FPRFaerinDialogueNode* Node = IsValid(DialogueData) ? DialogueData->FindDialogueNodePtr(DialogueNodeId) : nullptr;
	if (Node == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinSubtitle] NotifyFromClient node not found id=%s (DialogueData=%s)"),
			*DialogueNodeId.ToString(), *GetNameSafe(DialogueData));
		return;
	}

	BroadcastFaerinSubtitleForDialogueNode(*Node);
}

void APRFaerinEncounterDirector::BroadcastFaerinSubtitleForDialogueNode(const FPRFaerinDialogueNode& Node)
{
	TArray<APRPlayerController*> Audience;
	GetGatherPlayerControllers(Audience);

	// 상호작용자는 Gather 물리 판정과 무관하게 반드시 자막을 받아야 한다(요구 6.1).
	if (APRPlayerCharacter* InstigatorPlayer = ChoiceInstigator.Get())
	{
		if (APRPlayerController* InstigatorPC = Cast<APRPlayerController>(InstigatorPlayer->GetController()))
		{
			Audience.AddUnique(InstigatorPC);
		}
	}

	const bool bFaerinNode = IsFaerinSpokenDialogueNode(Node);
	UE_LOG(LogTemp, Log, TEXT("[FaerinSubtitle] Broadcast node=%s faerin=%d audience=%d speaker=%s"),
		*Node.NodeId.ToString(), bFaerinNode ? 1 : 0, Audience.Num(), *Node.SpeakerName.ToString());

	for (APRPlayerController* PC : Audience)
	{
		if (!IsValid(PC))
		{
			continue;
		}

		if (bFaerinNode)
		{
			// 본문/화자는 NodeId로 클라에서 resolve한다(FText RPC 금지).
			if (PC->IsLocalController())
			{
				PC->ShowFaerinSubtitleLocal(this, Node.NodeId);
			}
			else
			{
				PC->ClientShowFaerinSubtitle(this, Node.NodeId);
			}
		}
		else
		{
			if (PC->IsLocalController())
			{
				PC->HideFaerinSubtitleLocal();
			}
			else
			{
				PC->ClientHideFaerinSubtitle();
			}
		}
	}
}

void APRFaerinEncounterDirector::HideFaerinSubtitleForGatherPlayers()
{
	TArray<APRPlayerController*> Audience;
	GetGatherPlayerControllers(Audience);
	for (APRPlayerController* PC : Audience)
	{
		if (IsValid(PC))
		{
			if (PC->IsLocalController())
			{
				PC->HideFaerinSubtitleLocal();
			}
			else
			{
				PC->ClientHideFaerinSubtitle();
			}
		}
	}
}

void APRFaerinEncounterDirector::HideFaerinSubtitleForPlayers(const TArray<APRPlayerCharacter*>& Players)
{
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		if (APRPlayerController* PC = Cast<APRPlayerController>(Player->GetController()))
		{
			if (PC->IsLocalController())
			{
				PC->HideFaerinSubtitleLocal();
			}
			else
			{
				PC->ClientHideFaerinSubtitle();
			}
		}
	}
}

// ===== Gather 이탈 정리 =====

void APRFaerinEncounterDirector::NotifyPlayerExitedGather(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !IsValid(Player))
	{
		return;
	}

	APRPlayerController* PC = Cast<APRPlayerController>(Player->GetController());
	if (!IsValid(PC))
	{
		return;
	}

	if (PC->IsLocalController())
	{
		PC->HideFaerinSubtitleLocal();
	}
	else
	{
		PC->ClientHideFaerinSubtitle();
	}

	// 연출 중 이탈이면 해당 클라의 로컬 시퀀스/입력/카메라까지 복구한다.
	if (CurrentState == EFaerinEncounterState::IntroCutsceneDialogue
		|| CurrentState == EFaerinEncounterState::PreFightCutscene)
	{
		if (PC->IsLocalController())
		{
			PC->StopFaerinEncounterSequenceLocal(this, TEXT("LeftGather"));
		}
		else
		{
			PC->ClientStopFaerinEncounterSequence(this, TEXT("LeftGather"));
		}
	}

	if (PC->IsLocalController())
	{
		PC->SetEncounterInputLockLocal(false);
		PC->RestoreFaerinEncounterViewTargetLocal(0.0f);
		PC->SetFaerinEncounterHUDVisibleLocal(true);
	}
	else
	{
		PC->ClientSetEncounterInputLock(false);
		PC->ClientRestoreFaerinEncounterViewTarget(0.0f);
		PC->ClientSetFaerinEncounterHUDVisible(true);
	}
}

// ===== 인트로/전투 시작 흐름 =====

void APRFaerinEncounterDirector::StartIntroSequence()
{
	TArray<APRPlayerCharacter*> IntroPlayers;
	GetPlayersFromSet(IntroEnteredPlayers, IntroPlayers);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Intro);
	}

	SetInputLockedForPlayers(IntroPlayers, true);

	if (bPrimeIntroStartPoseBeforeIntro)
	{
		MulticastPrimeIntroStartPose();
	}

	if (bApplyIntroStartLoopBeforeIntro)
	{
		MulticastApplyIntroStartLoopAnimations();
	}

	SetEncounterState(EFaerinEncounterState::IntroCutsceneDialogue);
	// 입력을 잠근 대상(IntroPlayers)과 시퀀스 재생 대상을 동일하게 유지한다.
	// 이 시점에는 arena 물리 판정이 아직 동기화되지 않을 수 있어 Gather를 재계산하지 않는다.
	PlayEncounterSequenceForPlayers(IntroPlayers, EFaerinEncounterSequence::Intro);

	const float Duration = GetSequenceDurationSeconds(IntroSequence, IntroSequenceFallbackSeconds);
	if (Duration <= UE_SMALL_NUMBER)
	{
		FinishIntroSequence();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IntroSequenceTimerHandle,
			this,
			&ThisClass::FinishIntroSequence,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::FinishIntroSequence()
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::IntroCutsceneDialogue)
	{
		return;
	}

	MulticastStopSequenceVoiceCues(TEXT("IntroFinished"));

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	TArray<APRPlayerCharacter*> IntroPlayers;
	GetPlayersFromSet(IntroEnteredPlayers, IntroPlayers);
	for (APRPlayerCharacter* Player : IntroPlayers)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		if (APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController()))
		{
			if (PlayerController->IsLocalController())
			{
				PlayerController->StopFaerinEncounterSequenceLocal(this, TEXT("IntroFinished"));
			}
			else
			{
				PlayerController->ClientStopFaerinEncounterSequence(this, TEXT("IntroFinished"));
			}
		}
	}

	SetHUDVisibleForPlayers(IntroPlayers, true);
	RestoreViewTargetForPlayers(IntroPlayers, IntroCameraRestoreBlendTime, TEXT("IntroFinished"));
	SetInputLockedForPlayers(IntroPlayers, false);

	// 재도전 복원 참조용: Intro 최초 정상 종료 시점의 Faerin 연출 액터 월드 transform 캡처.
	if (!bCapturedPostIntroFaerinTransform)
	{
		if (AActor* FaerinActor = ResolveDialogueEmoteActor())
		{
			CapturedPostIntroFaerinTransform = FaerinActor->GetActorTransform();
			bCapturedPostIntroFaerinTransform = true;
		}
	}

	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::StartFightSequence()
{
	ClearDialogueEmotePlaybackLocal(false);
	StopDialogueVoiceLocal();
	SetEncounterState(EFaerinEncounterState::PreFightCutscene);
	// 입력을 잠근 대상(CombatStartParticipants)과 시퀀스 재생 대상을 동일하게 유지한다.
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	PlayEncounterSequenceForPlayers(Participants, EFaerinEncounterSequence::FightStart);

	const float Duration = GetSequenceDurationSeconds(FightStartSequence, FightStartSequenceFallbackSeconds);
	if (Duration <= UE_SMALL_NUMBER)
	{
		FinishFightSequence();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FightStartSequenceTimerHandle,
			this,
			&ThisClass::FinishFightSequence,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::FinishFightSequence()
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::PreFightCutscene)
	{
		return;
	}

	MulticastStopSequenceVoiceCues(TEXT("FightStartFinished"));
	SetEncounterState(EFaerinEncounterState::CombatStarting);
	MulticastSetPresentationActorsHidden(true);

	SpawnedCombatBoss = SpawnCombatBossFromSpawner();
	if (!IsValid(SpawnedCombatBoss))
	{
		UE_LOG(LogTemp, Error, TEXT("FaerinEncounterDirector failed to spawn combat boss."));
		RecoverFightStartFailure();
		return;
	}

	BindCombatBossDefeat(SpawnedCombatBoss);
	SeedCombatBossThreat();

	if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(SpawnedCombatBoss))
	{
		BossCharacter->RequestBossEncounterBegin();
	}

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Combat);
	}

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	SetHUDVisibleForPlayers(Participants, true);
	RestoreViewTargetForPlayers(Participants, FightStartCameraRestoreBlendTime, TEXT("FightStartFinished"));
	SetInputLockedForPlayers(Participants, false);
	SetEncounterState(EFaerinEncounterState::Combat);
}

void APRFaerinEncounterDirector::ResetCombatBoss()
{
	bResettingCombatBoss = true;
	UnbindCombatBossDefeat();

	if (IsValid(BossSpawnerActor) && BossSpawnerActor->GetClass()->ImplementsInterface(UPRBossSpawnProviderInterface::StaticClass()))
	{
		IPRBossSpawnProviderInterface::Execute_ResetBossForEncounter(BossSpawnerActor, SpawnedCombatBoss);
	}
	else if (IsValid(SpawnedCombatBoss))
	{
		SpawnedCombatBoss->Destroy();
	}

	SpawnedCombatBoss = nullptr;
	bResettingCombatBoss = false;
}

void APRFaerinEncounterDirector::RestoreNegotiationAfterWipe()
{
	ResetCombatBoss();
	CloseChoiceUIForInstigator();

	// 1) BoundaryActor->ClearEncounterState()는 arena tracking set을 비우므로,
	//    Gather 대상 산출은 반드시 그 전에 snapshot 한다.
	TArray<APRPlayerCharacter*> RetryGatherPlayers;
	GetGatherPlayers(RetryGatherPlayers);

	// 2) Gather 범위 안 플레이어만 PlayerPoint/슬롯으로 이동(Intro는 재생하지 않음).
	// AlignRetryGatherPlayersToPlayerPoints(RetryGatherPlayers);

	// 3) 입력/카메라/자막 정리는 snapshot 대상 기준으로 수행.
	SetInputLockedForPlayers(RetryGatherPlayers, false);
	RestoreViewTargetForPlayers(RetryGatherPlayers, IntroCameraRestoreBlendTime, TEXT("RetryReady"));
	SetHUDVisibleForPlayers(RetryGatherPlayers, true);
	HideFaerinSubtitleForPlayers(RetryGatherPlayers);

	// 4) 연출 Actor 복구(Intro 종료 후 협상 idle 상태로 복귀).
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	// 4b) (옵션) 캡처한 PostIntro transform 강제 복원. 사용자 attach/detach와 충돌 확인 후 켠다.
	if (bRestoreCapturedPostIntroTransformAfterWipe && bCapturedPostIntroFaerinTransform)
	{
		if (AActor* FaerinActor = ResolveDialogueEmoteActor())
		{
			FaerinActor->SetActorTransform(CapturedPostIntroFaerinTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->ClearEncounterState();
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	CombatStartParticipants.Reset();
	ChoiceInstigator = nullptr;
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::RecoverFightStartFailure()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	MulticastStopSequenceVoiceCues(TEXT("FightStartFailure"));
	CloseChoiceUIForInstigator();
	RestoreViewTargetForPlayers(Participants, FightStartCameraRestoreBlendTime, TEXT("FightStartFailure"));
	SetInputLockedForPlayers(Participants, false);
	SetHUDVisibleForPlayers(Participants, true);
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	SpawnedCombatBoss = nullptr;
	bResettingCombatBoss = false;
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::SnapshotCombatStartParticipants(APRPlayerCharacter* FallbackPlayer)
{
	CombatStartParticipants.Reset();

	TArray<APRPlayerCharacter*> InsidePlayers;
	GetGatherPlayers(InsidePlayers);

	for (APRPlayerCharacter* Player : InsidePlayers)
	{
		if (IsEligibleEncounterPlayer(Player))
		{
			AddPlayer(CombatStartParticipants, Player);
		}
	}

	if (IsEligibleEncounterPlayer(FallbackPlayer))
	{
		AddPlayer(CombatStartParticipants, FallbackPlayer);
	}
}

// ===== 플레이어 배치/입력 잠금 =====

void APRFaerinEncounterDirector::SetInputLockedForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bLock) const
{
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
		if (IsValid(PlayerController))
		{
			if (PlayerController->IsLocalController())
			{
				PlayerController->SetEncounterInputLockLocal(bLock);
			}
			else
			{
				PlayerController->ClientSetEncounterInputLock(bLock);
			}
		}
	}
}

void APRFaerinEncounterDirector::SetHUDVisibleForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bVisible) const
{
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
		if (IsValid(PlayerController))
		{
			if (PlayerController->IsLocalController())
			{
				PlayerController->SetFaerinEncounterHUDVisibleLocal(bVisible);
			}
			else
			{
				PlayerController->ClientSetFaerinEncounterHUDVisible(bVisible);
			}
		}
	}
}

void APRFaerinEncounterDirector::RestoreViewTargetForPlayers(
	const TArray<APRPlayerCharacter*>& Players,
	float BlendTime,
	const TCHAR* Reason) const
{
	const float ClampedBlendTime = FMath::Max(BlendTime, 0.0f);
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
		if (!IsValid(PlayerController))
		{
			continue;
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinEncounterCamera] RestoreViewTarget player=%s blend=%.2f reason=%s"),
			*Player->GetName(),
			ClampedBlendTime,
			Reason != nullptr ? Reason : TEXT("None"));
		if (PlayerController->IsLocalController())
		{
			PlayerController->RestoreFaerinEncounterViewTargetLocal(ClampedBlendTime);
		}
		else
		{
			PlayerController->ClientRestoreFaerinEncounterViewTarget(ClampedBlendTime);
		}
	}
}

void APRFaerinEncounterDirector::AlignCombatStartParticipants()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	if (Participants.Num() == 0)
	{
		return;
	}

	Participants.RemoveAll([](APRPlayerCharacter* Player)
	{
		return !IsValid(Player);
	});

	APRPlayerCharacter* CenterPlayer = IsValid(ChoiceInstigator) && Participants.Contains(ChoiceInstigator.Get())
		? ChoiceInstigator.Get()
		: Participants[0];
	Participants.Remove(CenterPlayer);

	ApplySlotTransform(CenterPlayer, ResolveSlotTransform(SlotCenterActor, PlayerSlotCenter));

	if (Participants.Num() >= 1)
	{
		ApplySlotTransform(Participants[0], ResolveSlotTransform(SlotLeftActor, PlayerSlotLeft));
	}

	if (Participants.Num() >= 2)
	{
		ApplySlotTransform(Participants[1], ResolveSlotTransform(SlotRightActor, PlayerSlotRight));
	}
}

void APRFaerinEncounterDirector::AlignRetryGatherPlayersToPlayerPoints(const TArray<APRPlayerCharacter*>& GatherPlayers)
{
	TArray<APRPlayerCharacter*> Players = GatherPlayers;
	Players.RemoveAll([](APRPlayerCharacter* Player)
	{
		return !IsValid(Player);
	});

	if (Players.Num() == 0)
	{
		return;
	}

	// 재도전 시점에는 ChoiceInstigator가 없을 수 있으므로 PlayerId로 안정 정렬한다.
	// (TArray<T*>::Sort는 포인터 배열일 때 역참조 객체를 비교 콜백에 전달한다.)
	Players.Sort([](const APRPlayerCharacter& A, const APRPlayerCharacter& B)
	{
		const APRPlayerState* StateA = A.GetPlayerState<APRPlayerState>();
		const APRPlayerState* StateB = B.GetPlayerState<APRPlayerState>();
		const int32 IdA = IsValid(StateA) ? StateA->GetPlayerId() : MAX_int32;
		const int32 IdB = IsValid(StateB) ? StateB->GetPlayerId() : MAX_int32;
		return IdA < IdB;
	});

	// 1명: Center / 2명: Center+Left / 3명: Center+Left+Right
	ApplySlotTransform(Players[0], ResolveSlotTransform(SlotCenterActor, PlayerSlotCenter));

	if (Players.Num() >= 2)
	{
		ApplySlotTransform(Players[1], ResolveSlotTransform(SlotLeftActor, PlayerSlotLeft));
	}

	if (Players.Num() >= 3)
	{
		ApplySlotTransform(Players[2], ResolveSlotTransform(SlotRightActor, PlayerSlotRight));
	}

	// 슬롯이 3개뿐이므로 4명 이상은 추가 이동하지 않는다.
	for (const APRPlayerCharacter* Player : Players)
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinRetry] Gather player=%s aligned to player point."), *GetNameSafe(Player));
	}
}

void APRFaerinEncounterDirector::ApplySlotTransform(APRPlayerCharacter* Player, const FTransform& SlotTransform) const
{
	if (!IsValid(Player))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	Player->SetActorTransform(SlotTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}

FTransform APRFaerinEncounterDirector::ResolveSlotTransform(AActor* SlotActor, const FTransform& FallbackTransform) const
{
	return IsValid(SlotActor) ? SlotActor->GetActorTransform() : FallbackTransform;
}

// ===== 보스 스폰/위협 주입 =====

AActor* APRFaerinEncounterDirector::SpawnCombatBossFromSpawner()
{
	if (!IsValid(BossSpawnerActor) || !BossSpawnerActor->GetClass()->ImplementsInterface(UPRBossSpawnProviderInterface::StaticClass()))
	{
		return nullptr;
	}

	return IPRBossSpawnProviderInterface::Execute_SpawnBossForEncounter(BossSpawnerActor);
}

void APRFaerinEncounterDirector::BindCombatBossDefeat(AActor* Boss)
{
	if (!IsValid(Boss))
	{
		return;
	}

	Boss->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
	Boss->OnDestroyed.AddDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
	bCombatBossDefeated = false;
}

void APRFaerinEncounterDirector::UnbindCombatBossDefeat()
{
	AActor* Boss = SpawnedCombatBoss.Get();
	if (Boss == nullptr)
	{
		return;
	}

	Boss->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
}

void APRFaerinEncounterDirector::SeedCombatBossThreat()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	AddThreatToCombatBoss(Participants, ChoiceInstigator, InitialEncounterThreat);
}

void APRFaerinEncounterDirector::AddThreatToCombatBoss(
	const TArray<APRPlayerCharacter*>& Participants,
	APRPlayerCharacter* PrimaryTarget,
	float ThreatAmount)
{
	if (!HasAuthority() || !IsValid(SpawnedCombatBoss))
	{
		return;
	}

	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(SpawnedCombatBoss))
	{
		FaerinCharacter->SeedEncounterTargets(Participants, PrimaryTarget, ThreatAmount);
		return;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(SpawnedCombatBoss);
	UPREnemyThreatComponent* ThreatComponent = EnemyInterface != nullptr
		? EnemyInterface->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	APRPlayerCharacter* ResolvedPrimaryTarget = IsValid(PrimaryTarget) ? PrimaryTarget : nullptr;
	const float ResolvedThreatAmount = FMath::Max(ThreatAmount, 0.0f);
	for (APRPlayerCharacter* Participant : Participants)
	{
		if (!IsValid(Participant))
		{
			continue;
		}

		if (ResolvedThreatAmount > UE_SMALL_NUMBER)
		{
			ThreatComponent->AddThreat(Participant, ResolvedThreatAmount);
		}

		if (!IsValid(ResolvedPrimaryTarget))
		{
			ResolvedPrimaryTarget = Participant;
		}
	}

	if (IsValid(PrimaryTarget) && !Participants.Contains(PrimaryTarget) && ResolvedThreatAmount > UE_SMALL_NUMBER)
	{
		ThreatComponent->AddThreat(PrimaryTarget, ResolvedThreatAmount);
	}

	if (IsValid(ResolvedPrimaryTarget))
	{
		ThreatComponent->ForceCurrentTarget(ResolvedPrimaryTarget);
	}
}

// ===== 시퀀스 재생 =====

float APRFaerinEncounterDirector::GetSequenceDurationSeconds(ULevelSequence* Sequence, float FallbackSeconds) const
{
	if (!IsValid(Sequence))
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!IsValid(MovieScene))
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	if (!PlaybackRange.HasLowerBound() || !PlaybackRange.HasUpperBound())
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const double TickResolutionDecimal = TickResolution.AsDecimal();
	if (TickResolutionDecimal <= UE_SMALL_NUMBER)
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const int32 FrameCount = PlaybackRange.GetUpperBoundValue().Value - PlaybackRange.GetLowerBoundValue().Value;
	return FMath::Max(static_cast<float>(static_cast<double>(FrameCount) / TickResolutionDecimal), 0.0f);
}

void APRFaerinEncounterDirector::ApplyNegotiationPresentationIdleLocal()
{
	if (bUseIntroTailLoopAfterIntro && IntroTailLoopBindings.Num() > 0)
	{
		ApplyIntroTailAnimationLoopsLocal();
		return;
	}

	ApplyConfiguredNegotiationIdleLocal();
}

void APRFaerinEncounterDirector::ApplyConfiguredNegotiationIdleLocal()
{
	if (!bApplyNegotiationIdleAfterIntro)
	{
		return;
	}

	for (const FPRFaerinPresentationIdleBinding& Binding : NegotiationIdleBindings)
	{
		PlayIdleAnimationOnPresentationActor(Binding.Actor, Binding.IdleAnimation, Binding.bLoop);
	}
}

void APRFaerinEncounterDirector::ApplyIntroStartLoopAnimationsLocal()
{
	for (const FPRFaerinPresentationTailLoopBinding& Binding : IntroStartLoopBindings)
	{
		PlaySingleNodeAnimationOnPresentationActor(
			Binding.Actor,
			Binding.LoopAnimation,
			Binding.StartPositionSeconds,
			Binding.bLoop,
			Binding.bPreserveActorTransform);
	}
}

void APRFaerinEncounterDirector::ApplyIntroTailAnimationLoopsLocal()
{
	for (const FPRFaerinPresentationTailLoopBinding& Binding : IntroTailLoopBindings)
	{
		PlaySingleNodeAnimationOnPresentationActor(
			Binding.Actor,
			Binding.LoopAnimation,
			Binding.StartPositionSeconds,
			Binding.bLoop,
			Binding.bPreserveActorTransform);
	}
}

void APRFaerinEncounterDirector::PrimeIntroStartPoseLocal()
{
	if (!IsValid(IntroSequence))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UMovieScene* MovieScene = IntroSequence->GetMovieScene();
	if (!IsValid(MovieScene))
	{
		return;
	}

	const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	if (!PlaybackRange.HasLowerBound())
	{
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bDisableCameraCuts = true;
	PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;

	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World,
		IntroSequence,
		PlaybackSettings,
		SequenceActor);
	if (!IsValid(SequencePlayer))
	{
		return;
	}

	const FFrameTime StartFrameTime(PlaybackRange.GetLowerBoundValue());
	SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(StartFrameTime, EUpdatePositionMethod::Jump));

	if (IsValid(SequenceActor))
	{
		SequenceActor->Destroy();
	}
}

void APRFaerinEncounterDirector::HandleDialogueVoiceEventLocal(const FSoftObjectPath& VoiceEventPath, FName NodeId)
{
	StopDialogueVoiceLocal();

	const FString PathString = VoiceEventPath.ToString().TrimStartAndEnd();
	if (PathString.IsEmpty())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FaerinDialogueVoice] Empty voice path for node=%s"), *NodeId.ToString());
		return;
	}

	UObject* LoadedObject = VoiceEventPath.TryLoad();
	if (!IsValid(LoadedObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueVoice] Failed to load voice path=%s"), *PathString);
		return;
	}

	USoundBase* Sound = Cast<USoundBase>(LoadedObject);
	if (!IsValid(Sound))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinDialogueVoice] Loaded non-sound asset path=%s class=%s"),
			*PathString,
			*LoadedObject->GetClass()->GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AActor* VoiceActor = ResolveDialogueVoiceActor();
	USceneComponent* AttachComponent = ResolveDialogueVoiceAttachComponent(VoiceActor);
	if (IsValid(AttachComponent))
	{
		ActiveDialogueVoiceAudioComponent = UGameplayStatics::SpawnSoundAttached(
			Sound,
			AttachComponent,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			false,
			1.0f,
			1.0f,
			0.0f);
	}
	else if (IsValid(VoiceActor))
	{
		ActiveDialogueVoiceAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
			World,
			Sound,
			VoiceActor->GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueVoice] DialogueVoiceActor is not set."));
		return;
	}

	if (IsValid(ActiveDialogueVoiceAudioComponent))
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueVoice] Play voice path=%s sound=%s"), *PathString, *Sound->GetName());
	}
}

void APRFaerinEncounterDirector::HandleDialogueEmoteChangedLocal(FName EmoteId, const FString& EmoteObjectPath)
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinDialogueEmote] EmoteId=%s Path=%s"),
		*EmoteId.ToString(),
		*EmoteObjectPath);

	ClearDialogueEmoteReturnTimerLocal();

	const FString PathString = EmoteObjectPath.TrimStartAndEnd();
	if (PathString.IsEmpty())
	{
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	if (PathString.StartsWith(TEXT("/Game/_Core/Emotes/")))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FaerinDialogueEmote] Skip generic emote path=%s"), *PathString);
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	const FSoftObjectPath EmotePath(PathString);
	if (EmotePath.IsNull())
	{
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	UObject* LoadedObject = EmotePath.TryLoad();
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(LoadedObject);
	if (!IsValid(AnimSequence))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueEmote] AnimSequence load failed. Path=%s"), *PathString);
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	AActor* TargetActor = ResolveDialogueEmoteActor();
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueEmote] DialogueEmoteActor is not set."));
		bDialogueEmotePlaying = false;
		return;
	}

	const float Duration = FMath::Max(0.05f, AnimSequence->GetPlayLength());
	UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueEmote] Play Anim=%s Duration=%.3f"), *AnimSequence->GetName(), Duration);

	PlaySingleNodeAnimationOnPresentationActor(TargetActor, AnimSequence, 0.0f, false, true);
	bDialogueEmotePlaying = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DialogueEmoteReturnTimerHandle,
			this,
			&ThisClass::RestoreDialogueEmoteIdleLocal,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::RestoreDialogueEmoteIdleLocal()
{
	ClearDialogueEmoteReturnTimerLocal();
	bDialogueEmotePlaying = false;
	ApplyNegotiationPresentationIdleLocal();
}

void APRFaerinEncounterDirector::ClearDialogueEmoteReturnTimerLocal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueEmoteReturnTimerHandle);
	}
}

AActor* APRFaerinEncounterDirector::ResolveDialogueEmoteActor() const
{
	if (IsValid(DialogueEmoteActor))
	{
		return DialogueEmoteActor;
	}

	for (AActor* PresentationActor : PresentationActors)
	{
		if (IsValid(PresentationActor) && PresentationActor->GetName().Contains(TEXT("Cine_Faerin")))
		{
			return PresentationActor;
		}
	}

	return nullptr;
}

void APRFaerinEncounterDirector::ForceDetachFightStartFaerinPresentationActorLocal(FName Reason)
{
	AActor* FaerinActor = ResolveDialogueEmoteActor();
	const FString ReasonString = Reason.ToString();
	if (!IsValid(FaerinActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinFightStartDetach] %s: Faerin presentation actor missing."),
			*ReasonString);
		return;
	}

	const FTransform SavedActorWorldTransform = FaerinActor->GetActorTransform();
	USkeletalMeshComponent* FaerinMeshComponent = FaerinActor->FindComponentByClass<USkeletalMeshComponent>();
	const bool bHasFaerinMeshComponent = IsValid(FaerinMeshComponent);
	const FTransform SavedRootWorldTransform = IsValid(FaerinActor->GetRootComponent())
		? FaerinActor->GetRootComponent()->GetComponentTransform()
		: SavedActorWorldTransform;
	const FTransform SavedMeshWorldTransform = bHasFaerinMeshComponent ? FaerinMeshComponent->GetComponentTransform() : FTransform::Identity;
	const FTransform SavedMeshRelativeTransform = bHasFaerinMeshComponent ? FaerinMeshComponent->GetRelativeTransform() : FTransform::Identity;
	AActor* ParentActor = FaerinActor->GetAttachParentActor();
	USceneComponent* FaerinRootComponent = FaerinActor->GetRootComponent();
	USceneComponent* AttachParentComponent = IsValid(FaerinRootComponent) ? FaerinRootComponent->GetAttachParent() : nullptr;
	USceneComponent* MeshAttachParentComponent = bHasFaerinMeshComponent ? FaerinMeshComponent->GetAttachParent() : nullptr;
	const bool bRootAttached = ParentActor != nullptr || AttachParentComponent != nullptr;
	const bool bMeshExternallyAttached = bHasFaerinMeshComponent
		&& MeshAttachParentComponent != nullptr
		&& MeshAttachParentComponent != FaerinRootComponent
		&& MeshAttachParentComponent->GetOwner() != FaerinActor;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[FaerinFightStartDetach] %s BEFORE: actor=%s parentActor=%s parentComponent=%s meshParent=%s rootAttached=%d meshExternal=%d worldLoc=%s meshWorldLoc=%s"),
		*ReasonString,
		*FaerinActor->GetName(),
		ParentActor != nullptr ? *ParentActor->GetName() : TEXT("None"),
		AttachParentComponent != nullptr ? *AttachParentComponent->GetName() : TEXT("None"),
		MeshAttachParentComponent != nullptr ? *MeshAttachParentComponent->GetName() : TEXT("None"),
		bRootAttached ? 1 : 0,
		bMeshExternallyAttached ? 1 : 0,
		*SavedActorWorldTransform.GetLocation().ToString(),
		bHasFaerinMeshComponent ? *SavedMeshWorldTransform.GetLocation().ToString() : TEXT("None"));

	if (bRootAttached || bMeshExternallyAttached)
	{
		if (bRootAttached)
		{
			FaerinActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

			if (IsValid(FaerinRootComponent) && FaerinRootComponent->GetAttachParent() != nullptr)
			{
				FaerinRootComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			}
		}

		if (bMeshExternallyAttached)
		{
			FaerinMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			const FTransform MeshAnchoredActorWorldTransform = SavedMeshRelativeTransform.Inverse() * SavedMeshWorldTransform;
			FaerinActor->SetActorTransform(MeshAnchoredActorWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			FaerinMeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
		}
		else
		{
			FaerinActor->SetActorTransform(SavedRootWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			if (bHasFaerinMeshComponent)
			{
				FaerinMeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
			}
		}
	}

	if (UCharacterMovementComponent* FaerinMovementComponent = FaerinActor->FindComponentByClass<UCharacterMovementComponent>())
	{
		FaerinMovementComponent->StopMovementImmediately();
	}

	FaerinActor->SetActorHiddenInGame(false);
	if (bHasFaerinMeshComponent)
	{
		FaerinMeshComponent->SetHiddenInGame(false, true);
		FaerinMeshComponent->SetVisibility(true, true);
	}

	ParentActor = FaerinActor->GetAttachParentActor();
	FaerinRootComponent = FaerinActor->GetRootComponent();
	AttachParentComponent = IsValid(FaerinRootComponent) ? FaerinRootComponent->GetAttachParent() : nullptr;
	MeshAttachParentComponent = bHasFaerinMeshComponent ? FaerinMeshComponent->GetAttachParent() : nullptr;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[FaerinFightStartDetach] %s AFTER: actor=%s parentActor=%s parentComponent=%s meshParent=%s worldLoc=%s meshWorldLoc=%s"),
		*ReasonString,
		*FaerinActor->GetName(),
		ParentActor != nullptr ? *ParentActor->GetName() : TEXT("None"),
		AttachParentComponent != nullptr ? *AttachParentComponent->GetName() : TEXT("None"),
		MeshAttachParentComponent != nullptr ? *MeshAttachParentComponent->GetName() : TEXT("None"),
		*FaerinActor->GetActorLocation().ToString(),
		bHasFaerinMeshComponent ? *FaerinMeshComponent->GetComponentLocation().ToString() : TEXT("None"));
}

AActor* APRFaerinEncounterDirector::ResolveDialogueVoiceActor() const
{
	if (IsValid(DialogueVoiceActor))
	{
		return DialogueVoiceActor;
	}

	return ResolveDialogueEmoteActor();
}

USceneComponent* APRFaerinEncounterDirector::ResolveDialogueVoiceAttachComponent(AActor* VoiceActor) const
{
	if (!IsValid(VoiceActor))
	{
		return nullptr;
	}

	if (USkeletalMeshComponent* MeshComponent = VoiceActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		return MeshComponent;
	}

	return VoiceActor->GetRootComponent();
}

void APRFaerinEncounterDirector::ScheduleSequenceVoiceCuesLocal(EFaerinEncounterSequence SequenceType)
{
	StopSequenceVoiceCuesLocal(TEXT("ScheduleNewSequence"));

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (const FPRFaerinSequenceVoiceCue& Cue : SequenceVoiceCues)
	{
		if (Cue.SequenceType != SequenceType || Cue.SoundPath.IsNull())
		{
			continue;
		}

		const float StartTimeSeconds = FMath::Max(Cue.StartTimeSeconds, 0.0f);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinSequenceVoice] Schedule sequence=%d cue=%s start=%.3f"),
			static_cast<int32>(SequenceType),
			*Cue.CueId.ToString(),
			StartTimeSeconds);

		if (StartTimeSeconds <= UE_SMALL_NUMBER)
		{
			PlaySequenceVoiceCueLocal(SequenceType, Cue.CueId, Cue.SoundPath, Cue.bAttachToVoiceActor);
			continue;
		}

		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(
			this,
			&ThisClass::PlaySequenceVoiceCueLocal,
			SequenceType,
			Cue.CueId,
			Cue.SoundPath,
			Cue.bAttachToVoiceActor);
		World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, StartTimeSeconds, false);
		SequenceVoiceTimerHandles.Add(TimerHandle);
	}
}

void APRFaerinEncounterDirector::StopSequenceVoiceCuesLocal(FName Reason)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& TimerHandle : SequenceVoiceTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
	SequenceVoiceTimerHandles.Reset();

	bool bStoppedAny = false;
	for (UAudioComponent* AudioComponent : ActiveSequenceVoiceAudioComponents)
	{
		if (IsValid(AudioComponent))
		{
			AudioComponent->Stop();
			bStoppedAny = true;
		}
	}
	ActiveSequenceVoiceAudioComponents.Reset();

	if (bStoppedAny)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinSequenceVoice] StopAll reason=%s"),
			*Reason.ToString());
	}

	if (UWorld* World = GetWorld())
	{
		if (APRPlayerController* LocalPC = Cast<APRPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
		{
			LocalPC->HideFaerinSubtitleLocal();
		}
	}
}

bool APRFaerinEncounterDirector::ResolveSequenceSubtitleText(
	EFaerinEncounterSequence SequenceType,
	FName CueId,
	FText& OutSpeakerText,
	FText& OutBodyText) const
{
	if (SequenceType != EFaerinEncounterSequence::Intro)
	{
		return false;
	}

	static const FName FirstIntroCueId(TEXT("VO_ET_Faerin_5F79_Faerin"));
	static const FName SecondIntroCueId(TEXT("VO_ET_Faerin_E35F_Faerin"));
	static const FName ThirdIntroCueId(TEXT("VO_ET_Faerin_0AF7_Faerin"));

	OutSpeakerText = FText::FromString(TEXT("파에린"));

	if (CueId == FirstIntroCueId)
	{
		OutBodyText = FText::FromString(TEXT("조심하라, 피조물이여. 너는 전능한 파에린의 앞에 서 있다."));
		return true;
	}

	if (CueId == SecondIntroCueId)
	{
		OutBodyText = FText::FromString(TEXT("우리는 유일한 참칭왕을 쓰러뜨린 자, 우리가 바라보는 모든 것을 정복한 자다. 우리 앞에서는 모두가 떨며 무릎 꿇어야 한다."));
		return true;
	}

	if (CueId == ThirdIntroCueId)
	{
		OutBodyText = FText::FromString(TEXT("그런데 너는? 참으로 기묘하고 가련한 작은 피조물이로군. 유감이지만 문을 잘못 찾아온 것 같구나, 작은 쥐야."));
		return true;
	}

	return false;
}

void APRFaerinEncounterDirector::ShowSequenceSubtitleCueLocal(EFaerinEncounterSequence SequenceType, FName CueId)
{
	FText SpeakerText;
	FText BodyText;
	if (!ResolveSequenceSubtitleText(SequenceType, CueId, SpeakerText, BodyText))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	APRPlayerController* LocalPC = Cast<APRPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
	if (!IsValid(LocalPC))
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinSequenceSubtitle] Show sequence=%d cue=%s body=%s"),
		static_cast<int32>(SequenceType),
		*CueId.ToString(),
		*BodyText.ToString());

	LocalPC->ShowFaerinSubtitleTextLocal(SpeakerText, BodyText);
}

void APRFaerinEncounterDirector::PlaySequenceVoiceCueLocal(
	EFaerinEncounterSequence SequenceType,
	FName CueId,
	FSoftObjectPath SoundPath,
	bool bAttachToVoiceActor)
{
	if (SoundPath.IsNull())
	{
		return;
	}

	const FString PathString = SoundPath.ToString();
	UObject* LoadedObject = SoundPath.TryLoad();
	if (!IsValid(LoadedObject))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Failed to load sequence=%d cue=%s path=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString(),
			*PathString);
		return;
	}

	USoundBase* Sound = Cast<USoundBase>(LoadedObject);
	if (!IsValid(Sound))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Loaded non-sound sequence=%d cue=%s path=%s class=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString(),
			*PathString,
			*LoadedObject->GetClass()->GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AActor* VoiceActor = ResolveDialogueVoiceActor();
	UAudioComponent* AudioComponent = nullptr;
	if (bAttachToVoiceActor)
	{
		if (USceneComponent* AttachComponent = ResolveDialogueVoiceAttachComponent(VoiceActor))
		{
			AudioComponent = UGameplayStatics::SpawnSoundAttached(
				Sound,
				AttachComponent,
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				false,
				1.0f,
				1.0f,
				0.0f);
		}
	}

	if (!IsValid(AudioComponent) && IsValid(VoiceActor))
	{
		AudioComponent = UGameplayStatics::SpawnSoundAtLocation(World, Sound, VoiceActor->GetActorLocation());
	}

	if (!IsValid(AudioComponent))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Voice actor is not set. sequence=%d cue=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString());
		return;
	}

	ActiveSequenceVoiceAudioComponents.Add(AudioComponent);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinSequenceVoice] Play sequence=%d cue=%s sound=%s"),
		static_cast<int32>(SequenceType),
		*CueId.ToString(),
		*Sound->GetName());

	ShowSequenceSubtitleCueLocal(SequenceType, CueId);
}

void APRFaerinEncounterDirector::PlayIdleAnimationOnPresentationActor(
	AActor* Actor,
	UAnimSequenceBase* Animation,
	bool bLoop) const
{
	PlaySingleNodeAnimationOnPresentationActor(Actor, Animation, 0.0f, bLoop, true);
}

void APRFaerinEncounterDirector::PlaySingleNodeAnimationOnPresentationActor(
	AActor* Actor,
	UAnimationAsset* Animation,
	float StartPositionSeconds,
	bool bLoop,
	bool bPreserveActorTransform) const
{
	if (!IsValid(Actor) || !IsValid(Animation))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
	if (!IsValid(MeshComponent))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("FaerinEncounterDirector presentation actor has no SkeletalMeshComponent. Actor=%s"),
			*Actor->GetName());
		return;
	}

	const FTransform SavedActorTransform = Actor->GetActorTransform();
	const FTransform SavedMeshRelativeTransform = MeshComponent->GetRelativeTransform();
	const float ClampedStartPosition = FMath::Max(StartPositionSeconds, 0.0f);

	MeshComponent->PlayAnimation(Animation, bLoop);
	if (UAnimSingleNodeInstance* SingleNodeInstance = Cast<UAnimSingleNodeInstance>(MeshComponent->GetAnimInstance()))
	{
		SingleNodeInstance->SetLooping(bLoop);
		SingleNodeInstance->SetPosition(ClampedStartPosition, false);
		SingleNodeInstance->SetPlaying(true);
	}

	if (bPreserveActorTransform)
	{
		Actor->SetActorTransform(SavedActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
		MeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
	}
}

void APRFaerinEncounterDirector::PlaySequenceLocal(EFaerinEncounterSequence SequenceType)
{
	if (SequenceType == EFaerinEncounterSequence::FightStart)
	{
		ClearDialogueEmotePlaybackLocal(false);
		StopDialogueVoiceLocal();
	}

	ULevelSequence* Sequence = SequenceType == EFaerinEncounterSequence::Intro
		? IntroSequence.Get()
		: FightStartSequence.Get();
	if (!IsValid(Sequence))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	if (SequenceType == EFaerinEncounterSequence::Intro)
	{
		PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;
	}

	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World,
		Sequence,
		PlaybackSettings,
		SequenceActor);
	if (!IsValid(SequencePlayer))
	{
		return;
	}

	if (IsValid(SequenceActor))
	{
		ActiveSequenceActors.Add(SequenceActor);
	}

	ActiveSequencePlayers.Add(SequencePlayer);

	SequencePlayer->Play();

	if (SequenceType == EFaerinEncounterSequence::FightStart)
	{
		const float DetachDelaySeconds = FMath::Max(FightStartFaerinDetachDelaySeconds, 0.0f);
		if (DetachDelaySeconds <= UE_SMALL_NUMBER)
		{
			ForceDetachFightStartFaerinPresentationActorLocal(TEXT("AfterFightStartInitialEvaluation"));
		}
		else if (UWorld* LocalWorld = GetWorld())
		{
			FTimerHandle DetachTimerHandle;
			LocalWorld->GetTimerManager().SetTimer(
				DetachTimerHandle,
				FTimerDelegate::CreateUObject(
					this,
					&ThisClass::ForceDetachFightStartFaerinPresentationActorLocal,
					FName(TEXT("AfterFightStartInitialEvaluation"))),
				DetachDelaySeconds,
				false);
		}
	}

	ScheduleSequenceVoiceCuesLocal(SequenceType);
}

void APRFaerinEncounterDirector::StopLocalSequences()
{
	StopSequenceVoiceCuesLocal(TEXT("StopLocalSequences"));

	for (ULevelSequencePlayer* SequencePlayer : ActiveSequencePlayers)
	{
		if (IsValid(SequencePlayer))
		{
			SequencePlayer->Stop();
		}
	}
	ActiveSequencePlayers.Reset();

	for (ALevelSequenceActor* SequenceActor : ActiveSequenceActors)
	{
		if (IsValid(SequenceActor))
		{
			SequenceActor->Destroy();
		}
	}
	ActiveSequenceActors.Reset();
}

void APRFaerinEncounterDirector::OpenChoiceUIForPlayer(APRPlayerCharacter* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientOpenFaerinEncounterChoice(this);
}

void APRFaerinEncounterDirector::CloseChoiceUIForInstigator(bool bRestoreDefaultBGM)
{
	APRPlayerCharacter* Player = ChoiceInstigator.Get();
	if (!IsValid(Player))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientCloseFaerinEncounterChoice(bRestoreDefaultBGM);
}
