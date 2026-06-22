// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Game 인스턴스 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "PRGameTypes.h"
#include "PRGameInstance.generated.h"

class UPRSessionSubsystem;

// 프로세스 전체 수명의 최상위 컨텍스트. 로컬 세이브와 세션 진입점을 보유한다
// 실제 세션 Host/Join 동작은 UPRSessionSubsystem에 위임한다
UCLASS()
class PROJECTR_API UPRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/*~ UGameInstance Interface ~*/
	virtual void Init() override;
	virtual void Shutdown() override;

public:
	// 세션 Host 진입점. 실패 사유는 SessionSubsystem의 OnSessionFailed로 통지
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void HostSession(const FPRHostSessionParams& Params);

	// 세션 Join 진입점. 주소 포맷은 IP 방식과 OSS 결과 모두 동일하게 취급
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void JoinSession(const FPRJoinSessionParams& Params);

	// 현재 세션에서 퇴장. 클라는 메뉴 맵으로 복귀, 호스트는 서버 종료 후 복귀
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void LeaveSession();

	// 로컬 월드 저장 상태를 기준으로 Host 세션 시작 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	bool RequestHostSession();

	// OSS 검색 결과의 첫 번째 세션 참가 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	bool RequestJoinFirstSession();

	// 레거시 주소 입력 기반 Host 또는 Join 세션 요청
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	bool RequestStartSession(const FString& Address);

	// 전체 Scalability 그룹에 그래픽 품질 프로필 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Settings")
	bool ApplyGraphicsQualityProfile(EPRGraphicsQualityProfile QualityProfile, bool bSaveSettings = false);

	// 소프트 참조된 맵으로 ServerTravel 진입. 호스트 권위에서만 동작
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	bool ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute = false);

	// 다음 맵 진입 시 사용할 SpawnPoint 태그를 저장
	void SetPendingTravelSpawnPointId(FGameplayTag SpawnPointId);

	// 다음 맵 진입 SpawnPoint 태그를 반환하고 초기화
	FGameplayTag ConsumePendingTravelSpawnPointId();

	// 다음 맵 진입 SpawnPoint 태그 존재 여부 반환
	bool HasPendingTravelSpawnPointId() const { return PendingTravelSpawnPointId.IsValid(); }

	// 다음 맵 진입 시 복원할 월드 저장 데이터 설정
	void SetPendingWorldSaveData(const FPRWorldSaveData& WorldSaveData);

	// 다음 맵 진입 월드 저장 데이터 소비
	bool ConsumePendingWorldSaveData(FPRWorldSaveData& OutWorldSaveData);

	// 로컬 캐릭터 세이브 로드. 메뉴에서 "이어하기" 선택 시 호출
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool LoadLocalCharacter(FName SlotName);

	// 로컬 캐릭터 세이브 저장
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool SaveLocalCharacter(FName SlotName);

	// 1~4번 로컬 캐릭터 슬롯 존재 여부 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Save")
	bool DoesLocalCharacterSaveExist(int32 SlotIndex) const;

	// 1~4번 로컬 캐릭터 슬롯 로드
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool LoadLocalCharacterSlot(int32 SlotIndex);

	// 1~4번 로컬 캐릭터 슬롯 데이터를 조회만 함
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool TryGetLocalCharacterSaveSlotData(int32 SlotIndex, FPRCharacterSaveData& OutSaveData) const;

	// 1~4번 로컬 캐릭터 슬롯 저장
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool SaveLocalCharacterSlot(int32 SlotIndex);

	// 1~4번 로컬 캐릭터 슬롯 파일 삭제
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool DeleteLocalCharacterSaveSlot(int32 SlotIndex);

	// 1~4번 로컬 캐릭터 슬롯 기본 세이브 재생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool ResetLocalCharacterSaveSlot(int32 SlotIndex);

	// 현재 활성 로컬 캐릭터 슬롯 저장
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool SaveActiveLocalCharacterSlot();

	// 로컬 캐릭터 저장 파일이 하나도 없을 때 1번 슬롯 초기 저장 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool EnsureInitialLocalCharacterSave();

	// 신규 게임 진입용 빈 로컬 캐릭터 슬롯을 하나 보장
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool EnsureEmptyLocalCharacterSaveSlot();

	// 세션 진입 직전 로컬 캐릭터 페이로드 준비
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool EnsureLocalCharacterReadyForSession();

	// 보상 이벤트 발생 시 서버가 푸시한 Grant를 로컬 캐릭터에 즉시 반영
	// PlayerController.ClientGrantReward 수신 경로의 종착점
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	void ApplyRewardGrant(const FPRRewardGrant& Grant);

	// 현재 로컬 캐릭터 데이터 조회. Join 시 PlayerController가 서버로 제출할 때 사용
	const FPRCharacterSaveData& GetLocalCharacter() const { return LocalCharacterSave; }

	// 현재 로컬 월드 진행 데이터 조회. Host 시작 맵과 저장 위치 복원에 사용
	const FPRWorldSaveData& GetLocalWorldSave() const { return LocalWorldSave; }

	// 현재 플레이 중인 로컬 캐릭터 슬롯 번호 조회
	UFUNCTION(BlueprintPure, Category = "ProjectR|Save")
	int32 GetActiveLocalCharacterSlotIndex() const { return ActiveLocalCharacterSlotIndex; }

	// 현재 플레이 중인 로컬 캐릭터 슬롯 존재 여부 조회
	UFUNCTION(BlueprintPure, Category = "ProjectR|Save")
	bool HasActiveLocalCharacterSlot() const { return IsValidLocalCharacterSlotIndex(ActiveLocalCharacterSlotIndex); }

	// 로컬 캐릭터 직접 갱신. 신규 캐릭터 생성 UI 등에서 호출
	void SetLocalCharacterSave(const FPRCharacterSaveData& NewData) { LocalCharacterSave = NewData; }

	// 로컬 월드 진행 상태 직접 갱신. GameState 스냅샷 저장 시 사용
	void SetLocalWorldSave(const FPRWorldSaveData& NewData) { LocalWorldSave = NewData; }

private:
	// 1~4번 로컬 캐릭터 슬롯 이름 생성
	FString BuildLocalCharacterSlotName(int32 SlotIndex) const;

	// 로컬 캐릭터 슬롯 번호 유효성 확인
	bool IsValidLocalCharacterSlotIndex(int32 SlotIndex) const;

	// 신규 게임용 기본 캐릭터 저장 데이터 여부 확인
	bool IsDefaultLocalCharacterSaveData(const FPRCharacterSaveData& SaveData) const;

	// 현재 월드 GameState의 월드 진행 상태를 로컬 저장 캐시에 반영
	void RefreshLocalWorldSaveFromGameState();

	// 저장된 스폰 위치 기준 Host 시작 맵 이름 결정
	FName ResolveHostMapNameFromSave(const FPRWorldSaveData& WorldSaveData) const;

protected:
	// 게임 시작 시 기본으로 적용할 그래픽 품질 프로필
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Settings")
	EPRGraphicsQualityProfile DefaultGraphicsQualityProfile = EPRGraphicsQualityProfile::High;

	// 호스트 시작 시 사용할 기본 맵
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Session")
	FName HostMapName = TEXT("L_Sector09");

	// 호스트 시작 시 허용할 최대 인원
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Session", meta = (ClampMin = "1"))
	int32 HostMaxPlayers = 4;

	// 현재 플레이어가 들고 다니는 캐릭터 스펙. Join 시 이 데이터가 호스트로 전송됨
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Save")
	FPRCharacterSaveData LocalCharacterSave;

	// 현재 플레이어가 보유한 월드 진행 상태. Host 시작 시 월드와 SpawnPoint 복원에 사용됨
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Save")
	FPRWorldSaveData LocalWorldSave;

	// 현재 플레이 중인 로컬 캐릭터 저장 슬롯 번호
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Save")
	int32 ActiveLocalCharacterSlotIndex = INDEX_NONE;

	// ServerTravel 이후 최초 스폰에 사용할 SpawnPoint 태그
	FGameplayTag PendingTravelSpawnPointId;

	// ServerTravel 이후 복원할 월드 진행 상태
	FPRWorldSaveData PendingWorldSaveData;

	// 월드 진행 상태 대기 여부
	bool bHasPendingWorldSaveData = false;
};
