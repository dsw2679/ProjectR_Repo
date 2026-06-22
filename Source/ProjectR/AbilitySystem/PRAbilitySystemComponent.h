// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 연동 기능 구현)
// Author: 배유찬 (어빌리티 시스템 컴포넌트 초기 구축 및 태그 이벤트 바인딩 구현)
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRAbilitySystemComponent.generated.h"

struct FPRAbilitySetHandles;
class UPRAbilitySet;
class UPRAbilitySystemRegistry;


// 프로젝트 공통 ASC. AbilitySet 부여/해제, 속성 초기화, 입력 라우팅, AI 활성화 헬퍼 제공
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

protected:
	/*~ UAbilitySystemComponent Interface ~*/
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool bTagExists) override;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTagUpdated(const FGameplayTag Tag, bool TagExists);
	
public:
	// AbilitySet 일괄 부여. 서버 전용. OutHandles에 Clear용 대칭 핸들 누적
	void GiveAbilitySet(const UPRAbilitySet* AbilitySet, FPRAbilitySetHandles& OutHandles, UObject* InSourceObject = nullptr);

	// 현재 ActorInfo의 AvatarActor를 어빌리티에 알림
	void NotifyAvatarSet();

	// 이전 부여 시 받은 핸들로 ClearAbility · RemoveActiveGameplayEffect 후 Reset
	void ClearAbilitySetByHandles(FPRAbilitySetHandles& Handles);

	// 리스폰 시 ASC에 남은 런타임 어빌리티와 GameplayEffect 정리
	void ResetSystem();

	// Registry 기반 Row 리플렉션 주입으로 속성 초기화. 서버 전용. 1회 호출
	bool InitializeAttributesFromRegistry(const UPRAbilitySystemRegistry* Registry,
	                                       EPRCharacterRole Role, FName RowName);

	// 지정 Attribute 목록의 Base 값 스냅샷
	FPRAttributeBaseSnapshot MakeAttributeBaseSnapshot(const TArray<FGameplayAttribute>& Attributes) const;

	// 스냅샷 기반 Attribute Base 값 복원
	void ApplyAttributeBaseSnapshot(const FPRAttributeBaseSnapshot& InSnapshot);

	// 플레이어 입력 Pressed. DynamicSpecSourceTags에 InputTag 있는 Spec들을 Pressed/Held 목록에 추가
	void AbilityInputPressed(const FGameplayTag& InputTag);

	// Released 콜백
	void AbilityInputReleased(const FGameplayTag& InputTag);

	// PlayerController Tick 말미에서 호출. 3개 Handle 목록 소비
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	// 폰 변경·일시정지 등에서 3개 Handle 목록을 모두 비움
	void ClearAbilityInput();

	// AbilityTag(= AbilityTags 매칭)로 서버에서 활성화. BT Task용
	bool TryActivateAbilityOnServer(const FGameplayTag& AbilityTag,
	                                 FGameplayAbilitySpecHandle& OutHandle);

	// 활성화 가능 여부 사전 검사. 실패 시 Fail.* 태그 누적
	bool CanActivateAbilityByTag(const FGameplayTag& AbilityTag,
	                              FGameplayTagContainer& OutFailureTags) const;

	/**  
	 * UAbilitySystemComponent::ConsumeClientReplicatedTargetData는 TargetData를 소모하지만, TargetData의 존재 여부에 대해서는 반환하지 않는다.
	 * 따라서 하나의 TargetData에 대해 여러 콜백이 동시에 실행되었을 때, 하나의 콜백만 성공적으로 처리될 수 있다.
	 * 이 함수는 TargetData Consume 성공 여부 (호출 시점 TargetData 존재 여부)를 반환함으로써 Ability Task가 Consume에 실패한 경우 TargetData 수신을 계속 기다리게 할 수 있다.
	 */
	bool TryConsumeClientReplicatedTargetData(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey AbilityOriginalPredictionKey);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTriggerEvent(FGameplayTag EventTag);
	
protected:
	// 어빌리티 종료 시 델리게이트 브로드캐스트
	UFUNCTION()
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	// 어빌리티 활성화 시 델리게이트 브로드캐스트
	void HandleAbilityActivated(UGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnRep_OwningTags(const FGameplayTagContainer& OldTags);
public:
	// 어빌리티 활성화 직후 발행 (Ability 첫 태그 기준)
	UPROPERTY(BlueprintAssignable)
	FPRAbilityActivatedSignature OnAbilityActivated;

	// 어빌리티 종료 시 발행. bCancelled == true 면 Cancel 경로
	UPROPERTY(BlueprintAssignable)
	FPRAbilityEndedSignature OnAbilityEnded;

	// 태그 신규 추가 혹은 완전 제거시 발행되는 이벤트
	FOnGameplayTagUpdatedSignature OnGameplayTagUpdated;
	
protected:
	// 이번 프레임 Pressed 후보 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// 입력이 계속 눌려있는 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	// 이번 프레임 Released된 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	
	UPROPERTY(ReplicatedUsing = OnRep_OwningTags)
	FGameplayTagContainer PROwningTags;
};
