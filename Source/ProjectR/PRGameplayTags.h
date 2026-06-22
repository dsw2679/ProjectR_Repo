// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 이동/조준 및 피격/사망 관련 상태 태그 정의)
// Author: 배유찬 (코어 어빌리티, 상호작용, 핑/마커 및 데미지 파이프라인 태그 정의)
// Author: 손승우 (몬스터 및 보스 AI 전투 태그 정의)
// Author: 이건주 (배리어/드론 모드 및 소모품/퀵슬롯 관련 게임플레이 태그 정의)
#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
namespace PRGameplayTags
{
	// ===== System =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test); // 시스템 테스트용

	// ===== Player =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player); // 플레이어 관련 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ready); // 플레이어 관련 루트

	// ===== Ability.* — 어빌리티 식별 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Common_Die); // 사망 어빌리티
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Weapon_Fire_Primary); // 플레이어 주무기 발사 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Dodge); // 플레이어 구르기(회피) 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Crouch); // 플레이어 앉기 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Aim); // 플레이어 에임 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Sprint); // 플레이어 질주 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Interaction); // 플레이어 상호작용 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Ping); // 플레이어 핑 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Reload); // 플레이어 재장전 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_UseConsumable); // 플레이어 소비 아이템 사용 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_HitReact); // 플레이어 피격 리액션 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_SwapWeapon); // 플레이어 주무기/보조무기 교체 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Death); // 플레이어 사망 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Down); // 플레이어 다운 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_GetUp); // 플레이어 기상 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Revive); // 플레이어 동료 소생 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Action_Waypoint); // 크리스탈 상호작용 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_CombatEngaged); // 플레이어 전투 교전 갱신 어빌리티
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Weapon_Zoom); // 플레이어 볼트액션 줌 어빌리티

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Pattern); // 일반 적 패턴 공통 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Alert); // 일반 적 최초 발견 Alert
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Evade); // 일반 적 회피 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern); // 보스 패턴 공통 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_PhaseTransition); // 보스 페이즈 전환 공통 루트

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_Pattern); // Penitent 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_Projectile); // Penitent 기본 투사체
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_BarrierSummon); // Penitent 배리어 소환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_BarrierLaunch); // Penitent 배리어 발사
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_StaffSwing); // Penitent 근거리 휘두르기

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_Pattern); // RoyalArcher 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_WakeFromPerch); // RoyalArcher 전투 시작
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_FlameArrow); // RoyalArcher 주력 사격
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_TripleArrow); // RoyalArcher 3연속 화살 사격

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_Pattern); // Soldier_Armored 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerSwing01); // Soldier_Armored 1타
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerSwing02); // Soldier_Armored 2타
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerOverhead); // Soldier_Armored 강패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_ChargeThrust); // Soldier_Armored 돌진

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_Pattern); // Faerin 전투 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PhaseTransition); // Faerin 페이즈 전환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_MeleeQuickCombo); // Faerin 빠른 근접 콤보
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_SpokeCombo); // Faerin Spoke R/L 1타 연계 콤보
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalPairSequence); // Faerin 포털 연계
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalMissileSequence); // Faerin 원작형 미사일 포털 소환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalBarrageSequence); // Faerin 포털 연속 포격
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalAttachedSequence); // Faerin 부착 포털 소환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalTorrentSequence); // Faerin Torrent 포털 소환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_RainPortalSequence); // Faerin Rain Portal 소환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_TeleportLungeSequence); // Faerin 원작형 텔레포트 런지
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_ShiftPlayerClose); // Faerin Shift Player Close 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_TeleportDownSequence); // Faerin TeleportDown 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_CrashSequence); // Faerin Crash 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_CrashDropSequence); // Faerin CrashDrop 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_ThrowSequence); // Faerin Throw 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_CloneSequence); // Faerin 분신 소환 패턴 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_ApproachSprint); // Faerin 스프린트 접근 액션
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_TeleportDash); // Faerin 텔레포트 대시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_EnergyRain); // Faerin 범위 압박

	// ===== Pattern.* — 패턴 그룹 식별 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin); // Faerin 패턴 그룹 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Portal); // Faerin 포털 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Lunge); // Faerin 런지 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Godfall); // Faerin Godfall 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Shift); // Faerin Shift 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_TeleportDown); // Faerin TeleportDown 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Sword); // Faerin 검 근접 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Crash); // Faerin Crash 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Throw); // Faerin Throw 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Clone); // Faerin 분신 계열 패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pattern_Boss_Faerin_Fallback); // Faerin 패턴 실패 시 보조 이동 식별

	// ===== State.* — 캐릭터 상태 =====
	//UE_DECLARE_GAMEPLAY_TAG_EXTERN(State); // 상태 루트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead); // 사망 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy); // 그로기 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_StaminaDepleted); // 스태미너 고갈 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Down); // 전투불능 다운 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combating); // 전투 중 스태미나 제한 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable); // 무적 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging); // 구르기 동작중
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Crouching); // 구르기 동작중
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming); // 에이밍 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting); // 질주 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reloading); // 재장전 진행 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Channeling); // 캐스팅 또는 채널링 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PoiseRecoveryBlocked); // 강인도 회복 차단 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_RecoverableHealthRecoveryBlocked); // 회복 가능 체력 회복 차단 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PlayerInputLocked); // 플레이어 입력 차단 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PlayerHitReactLocked); // 피격 리액션 행동불능 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PhaseTransitioning); // 보스 페이즈 전환 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_PatternPlaying); // 보스 패턴 실행 중 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_Faerin_ForcedFollowUp); // Faerin Shift 이후 강제 연계 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_WeakpointOpen_Core); // 보스 코어 약점 오픈 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_Attacking); // 적 공격 실행중 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_Penitent_BarrierSummon); // Penitent 배리어 보유 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Armed); // 플레이어 무기 장착중 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Move); // 움직임 비활성화 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Interaction); // 상호작용 비활성화 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reviving); // 소생중 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_WeaponZooming); // 무기 줌 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SwappingWeapon); // 무기 교체 Draw 몽타주 진행 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Primary); // 현재 주무기 슬롯 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Primary_Base); // 현재 주무기 기본 사격 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Primary_Mod); // 현재 주무기 Mod 사격 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Secondary); // 현재 보조무기 슬롯 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Secondary_Base); // 현재 보조무기 기본 사격 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CurrentWeaponSlot_Secondary_Mod); // 현재 보조무기 Mod 사격 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Mod_Primary_GaugeLocked); // 주무기 Mod 게이지 축적 차단 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Mod_Secondary_GaugeLocked); // 보조무기 Mod 게이지 축적 차단 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Mod_Fire_Enabled); // 발사형 모드 활성 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_UsingConsumable); // 소비 아이템 사용중인 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Suppress_Fire_Primary); // 무기 기본 발사 억제 상태 (모드 스킬 발사로 전환된 상태) 
	
	// ===== Cooldown.* — 쿨다운 상태에서 부여 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Dodge); // 구르기 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Fire); // 무기 발사 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_PlayerHitReact); // 플레이어 피격 리액션 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_SwapWeapon); // 무기 교체 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Enemy_Evade); // 일반 적 회피 쿨다운
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_PortalMissile); // Faerin 포털 미사일 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_PortalPair); // Faerin 포털 2개 설치 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_PortalBarrage); // Faerin 포털 연속 포격 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_PortalAttached); // Faerin 부착 포털 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_PortalTorrent); // Faerin Torrent 포털 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_RainPortal); // Faerin Rain Portal 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_TeleportLunge); // Faerin Teleport Lunge 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_Shift); // Faerin Shift 계열 공용 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_TeleportDown); // Faerin TeleportDown 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_Crash); // Faerin Crash 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_Throw); // Faerin Throw 패턴 쿨다운
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Faerin_SpokeCombo); // Faerin Spoke Combo 패턴 쿨다운

	// ===== Event.* — SendGameplayEventToActor 이벤트 키 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit); // 피격 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_Engaged); // 전투 교전 유지 이벤트
	
	// Ability Triggers
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Death); // 사망 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Down); // 다운 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_GetUp); // 기상 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Revive_Start); // 소생 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Revive_Finished);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Revive_Canceled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Waypoint_Start); // 크리스탈 상호작용 시작
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Waypoint_End); // 크리스탈 상호작용 취소
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_GroggyEntered); // 그로기 진입 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PhaseTransition); // 페이즈 전환 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Reload); // 재장전 시작 이벤트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PlayerHitReact_Weak); // 플레이어 약한 경직 이벤트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PlayerHitReact_Strong); // 플레이어 강한 경직 이벤트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PlayerHitReact_Down); // 플레이어 다운 이벤트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PlayerDeathConfirmed); // 플레이어 다운 사망 전환 확정 이벤트

	// ===== Event.Player.* — 플레이어 액션/UI 알림용 EventManager 이벤트 키 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_HitShot); // 사격이 적중했을 때
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Recoil); // 사격 반동 발생 (FPRRecoilEventPayload 동반)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Aim_Start); // 에이밍 진입
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Aim_End); // 에이밍 해제
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_ChangeCrosshair); // 크로스헤어 Config 교체 (FPRChangeCrosshairEventPayload 동반)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_PreviewHit); // 히트스캔 미리보기 적중 상태 (FPRPreviewHitEventPayload 동반)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_ReloadCommit); // 재장전 몽타주 노티파이가 발행하는 자원 이동 트리거
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_ConsumableCommit); // 소비 아이템 몽타주 노티파이가 발행하는 사용 확정 트리거
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Interaction_Hold); // Hold 상호작용 단계 알림 (FPRInteractionHoldEventPayload 동반)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Interactable); // 상호작용 가능 알림
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_ModActivation); // Mod On / Off 이벤트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_WorldMarker); // 월드 마커 추가·제거 알림
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_HUDMessage); // HUD 안내 메시지 표시 알림
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_AttackTarget); // 플레이어 실제 피해 대상 알림

	// ===== Event.Boss.* — 보스 조우 이벤트 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_Spawn); // 보스 스폰 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_Encounter_Begin); // 보스 조우 시작 (FPRBossEncounterEventPayload 동반)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_Encounter_End); // 보스 조우 종료
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_Defeated); // 보스 처치 확정
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_PhaseChanged); // 보스 페이즈 변경
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_BGMPhasePreview); // 보스 BGM 페이즈 예고
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_BGMPatternCue); // 보스 BGM 특수패턴 Cue

	// ===== Cue.* — GameplayCue 식별 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Critical_Boss_PhaseTransition); // 보스 페이즈 전환 연출 Cue
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Critical_Boss_WeakpointOpen); // 보스 약점 창 오픈 연출 Cue

	// ===== SetByCaller.Trait.* — 특성 보너스 GameplayEffect 전달 값 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MaxHealth); // 특성 최대 체력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_Armor); // 특성 방어력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MovementSpeed); // 특성 이동속도 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_PlayerAttackPower); // 특성 플레이어 공격력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MaxStamina); // 특성 최대 스태미너 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_CriticalHitChance); // 특성 치명타 확률 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_CriticalDamageMultiplier); // 특성 치명타 피해 배율 보너스

	// ===== SetByCaller.Equipment.* — 장비 보너스 GameplayEffect 전달 값 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equipment_MaxHealth); // 장비 최대 체력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equipment_Armor); // 장비 방어력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equipment_PlayerAttackPower); // 장비 플레이어 공격력 보너스
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equipment_MaxStamina); // 장비 최대 스태미너 보너스

	// ===== Input.Ability.* — 플레이어 InputTag (AbilitySpec DynamicTags 매칭 키) =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Fire_Primary); // 기본 발사 입력 태그 (모드 스킬의 발사가 아닌 무기의 기본 발사)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Dodge); // 구르기 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Interact); // 상호작용 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Ping); // 핑 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Crouch); // 앉기 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Aim); // 에임 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Sprint); // 질주 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Mod); // 모드 스킬 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Reload); // 재장전 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_SwapWeapon); // 주무기/보조무기 교체 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Zoom); // 무기 줌 입력 태그
	
	// ===== Input.Locomotion.* — 플레이어 InputTag (Ability에 해당하지 않는 Input 키) =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Locomotion_Walk)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Locomotion_Sprint)

	// ===== Fail.* — CanActivateAbility 실패 사유 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cost); // Cost GE가 요구 자원 부족
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cooldown); // Cooldown 태그가 활성 중
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Blocked); // Block 태그에 의해 차단됨
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Invalid); // 어빌리티/소유자 상태가 유효하지 않음
	
	
}
