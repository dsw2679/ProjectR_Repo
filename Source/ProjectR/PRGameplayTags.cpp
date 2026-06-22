// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (플레이어 이동/조준 및 피격/사망 관련 상태 태그 정의)
// Author: 배유찬 (코어 어빌리티, 상호작용, 핑/마커 및 데미지 파이프라인 태그 정의)
// Author: 손승우 (몬스터 및 보스 AI 전투 태그 정의)
// Author: 이건주 (배리어/드론 모드 및 소모품/퀵슬롯 관련 게임플레이 태그 정의)
#include "PRGameplayTags.h"

namespace PRGameplayTags
{
	// ===== System =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(System_Test, "System.Test", "시스템 테스트용 태그");

	// ===== Player =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Player", "플레이어 관련 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Ready, "Player.Ready", "플레이어 준비 완료 태그");

	// ===== Ability.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Common_Die, "Ability.Common.Die", "사망 어빌리티"); // 사망 어빌리티

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Weapon_Fire_Primary, "Ability.Player.Weapon.Fire.Primary",
	                               "플레이어 주무기 발사 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Dodge, "Ability.Player.Dodge", "플레이어 구르기 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Crouch, "Ability.Player.Crouch", "플레이어 에임 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Aim, "Ability.Player.Aim", "플레이어 구르기 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Sprint, "Ability.Player.Sprint", "플레이어 질주 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Interaction, "Ability.Player.Interaction", "플레이어 상호작용 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Action_Waypoint, "Ability.Player.Action.Waypoint", "플레이어 크리스탈 상호작용 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Ping, "Ability.Player.Ping", "플레이어 핑 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Reload, "Ability.Player.Reload", "플레이어 재장전 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_UseConsumable, "Ability.Player.UseConsumable", "플레이어 소비 아이템 사용 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_HitReact, "Ability.Player.HitReact", "플레이어 피격 리액션 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_SwapWeapon, "Ability.Player.SwapWeapon", "플레이어 주무기/보조무기 교체 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Death, "Ability.Player.Death", "플레이어 사망 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Down, "Ability.Player.Down", "플레이어 다운 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Weapon_Zoom, "Ability.Player.Weapon.Zoom", "플레이어 볼트액션 줌 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_GetUp, "Ability.Player.GetUp", "플레이어 기상 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Revive, "Ability.Player.Revive", "플레이어 동료 소생 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_CombatEngaged, "Ability.Player.CombatEngaged", "플레이어 전투 교전 갱신 어빌리티");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Pattern, "Ability.Enemy.Pattern", "일반 적 패턴 공통 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Alert, "Ability.Enemy.Alert", "일반 적 최초 발견 Alert");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Evade, "Ability.Enemy.Evade", "일반 적 회피 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern, "Ability.Boss.Pattern", "보스 패턴 공통 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_PhaseTransition, "Ability.Boss.PhaseTransition", "보스 페이즈 전환 공통 루트");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_Pattern, "Ability.Enemy.Penitent.Pattern", "Penitent 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_Projectile, "Ability.Enemy.Penitent.Projectile", "Penitent 기본 투사체");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_BarrierSummon, "Ability.Enemy.Penitent.BarrierSummon", "Penitent 배리어 소환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_BarrierLaunch, "Ability.Enemy.Penitent.BarrierLaunch", "Penitent 배리어 발사");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_StaffSwing, "Ability.Enemy.Penitent.StaffSwing", "Penitent 근거리 휘두르기");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_Pattern, "Ability.Enemy.RoyalArcher.Pattern", "RoyalArcher 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_WakeFromPerch, "Ability.Enemy.RoyalArcher.WakeFromPerch", "RoyalArcher 전투 시작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_FlameArrow, "Ability.Enemy.RoyalArcher.FlameArrow", "RoyalArcher 주력 사격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_TripleArrow, "Ability.Enemy.RoyalArcher.TripleArrow", "RoyalArcher 3연속 화살 사격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_Pattern, "Ability.Enemy.SoldierArmored.Pattern", "Soldier_Armored 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerSwing01, "Ability.Enemy.SoldierArmored.HammerSwing01", "Soldier_Armored 1타");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerSwing02, "Ability.Enemy.SoldierArmored.HammerSwing02", "Soldier_Armored 2타");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerOverhead, "Ability.Enemy.SoldierArmored.HammerOverhead", "Soldier_Armored 강패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_ChargeThrust, "Ability.Enemy.SoldierArmored.ChargeThrust", "Soldier_Armored 돌진");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_Pattern, "Ability.Boss.Faerin.Pattern", "Faerin 전투 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PhaseTransition, "Ability.Boss.Faerin.PhaseTransition", "Faerin 페이즈 전환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_MeleeQuickCombo, "Ability.Boss.Faerin.MeleeQuickCombo", "Faerin 빠른 근접 콤보");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_SpokeCombo, "Ability.Boss.Faerin.SpokeCombo", "Faerin Spoke R/L 1타 연계 콤보");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalPairSequence, "Ability.Boss.Faerin.PortalPairSequence", "Faerin 포털 연계");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalMissileSequence, "Ability.Boss.Faerin.PortalMissileSequence", "Faerin 원작형 미사일 포털 소환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalBarrageSequence, "Ability.Boss.Faerin.PortalBarrageSequence", "Faerin 포털 연속 포격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalAttachedSequence, "Ability.Boss.Faerin.PortalAttachedSequence", "Faerin 부착 포털 소환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalTorrentSequence, "Ability.Boss.Faerin.PortalTorrentSequence", "Faerin Torrent 포털 소환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_RainPortalSequence, "Ability.Boss.Faerin.RainPortalSequence", "Faerin Rain Portal 소환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_TeleportLungeSequence, "Ability.Boss.Faerin.TeleportLungeSequence", "Faerin 원작형 텔레포트 런지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_ShiftPlayerClose, "Ability.Boss.Faerin.ShiftPlayerClose", "Faerin Shift Player Close 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_TeleportDownSequence, "Ability.Boss.Faerin.TeleportDownSequence", "Faerin TeleportDown 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_CrashSequence, "Ability.Boss.Faerin.CrashSequence", "Faerin Crash 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_CrashDropSequence, "Ability.Boss.Faerin.CrashDropSequence", "Faerin CrashDrop 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_ThrowSequence, "Ability.Boss.Faerin.ThrowSequence", "Faerin Throw 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_CloneSequence, "Ability.Boss.Faerin.CloneSequence", "Faerin 분신 소환 패턴 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_ApproachSprint, "Ability.Boss.Faerin.ApproachSprint", "Faerin 스프린트 접근 액션");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_TeleportDash, "Ability.Boss.Faerin.TeleportDash", "Faerin 텔레포트 대시");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_EnergyRain, "Ability.Boss.Faerin.EnergyRain", "Faerin 범위 압박");

	// ===== Pattern.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin, "Pattern.Boss.Faerin", "Faerin 패턴 그룹 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Portal, "Pattern.Boss.Faerin.Portal", "Faerin 포털 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Lunge, "Pattern.Boss.Faerin.Lunge", "Faerin 런지 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Godfall, "Pattern.Boss.Faerin.Godfall", "Faerin Godfall 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Shift, "Pattern.Boss.Faerin.Shift", "Faerin Shift 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_TeleportDown, "Pattern.Boss.Faerin.TeleportDown", "Faerin TeleportDown 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Sword, "Pattern.Boss.Faerin.Sword", "Faerin 검 근접 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Crash, "Pattern.Boss.Faerin.Crash", "Faerin Crash 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Throw, "Pattern.Boss.Faerin.Throw", "Faerin Throw 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Clone, "Pattern.Boss.Faerin.Clone", "Faerin 분신 계열 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pattern_Boss_Faerin_Fallback, "Pattern.Boss.Faerin.Fallback", "Faerin 패턴 실패 시 보조 이동 식별");

	// ===== State.* ====
	//UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "상태 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "그로기 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaDepleted, "State.StaminaDepleted", "스태미너 고갈 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Down, "State.Down", "전투불능 다운 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combating, "State.Combating", "전투 중 스태미나 제한 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적 상태 (회피 i-frame 등)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "구르기 중 (무적이 아님)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Crouching, "State.Crouching", "앉기 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming, "State.Aiming", "에이밍 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "질주 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Reloading, "State.Reloading", "재장전 진행 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Channeling, "State.Channeling", "캐스팅 또는 채널링 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PoiseRecoveryBlocked, "State.PoiseRecoveryBlocked", "강인도 회복 차단 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_RecoverableHealthRecoveryBlocked, "State.RecoverableHealthRecoveryBlocked", "회복 가능 체력 회복 차단 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PlayerInputLocked, "State.PlayerInputLocked", "플레이어 입력 차단 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PlayerHitReactLocked, "State.PlayerHitReactLocked", "피격 리액션 행동불능 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PhaseTransitioning, "State.PhaseTransitioning", "보스 페이즈 전환 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_PatternPlaying, "State.Boss.PatternPlaying", "보스 패턴 실행 중 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_Faerin_ForcedFollowUp, "State.Boss.Faerin.ForcedFollowUp", "Faerin Shift 이후 강제 연계 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_WeakpointOpen_Core, "State.Boss.WeakpointOpen.Core", "보스 코어 약점 오픈 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Enemy_Attacking, "State.Enemy.Attacking", "적 공격 실행중 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Enemy_Penitent_BarrierSummon, "State.Enemy.Penitent.BarrierSummon", "Penitent 배리어 보유 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Armed, "State.Armed", "플레이어 무기 장착중 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Move, "State.Block.Move", "움직임 비활성화 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Interaction, "State.Block.Interaction", "상호작용 비활성화 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_WeaponZooming, "State.WeaponZooming", "움직임 비활성화 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SwappingWeapon, "State.SwappingWeapon", "무기 교체 Draw 몽타주 진행 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Primary, "State.CurrentWeaponSlot.Primary", "현재 주무기 슬롯 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Primary_Base, "State.CurrentWeaponSlot.Primary.Base", "현재 주무기 기본 사격 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Primary_Mod, "State.CurrentWeaponSlot.Primary.Mod", "현재 주무기 Mod 사격 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Secondary, "State.CurrentWeaponSlot.Secondary", "현재 보조무기 슬롯 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Secondary_Base, "State.CurrentWeaponSlot.Secondary.Base", "현재 보조무기 기본 사격 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CurrentWeaponSlot_Secondary_Mod, "State.CurrentWeaponSlot.Secondary.Mod", "현재 보조무기 Mod 사격 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Mod_Primary_GaugeLocked, "State.Mod.Primary.GaugeLocked", "주무기 Mod 게이지 축적 차단 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Mod_Secondary_GaugeLocked, "State.Mod.Secondary.GaugeLocked", "보조무기 Mod 게이지 축적 차단 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Mod_Fire_Enabled, "State.Mod.Fire.Enabled", "발사형 모드 활성 상태");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Reviving, "State.Reviving", "소생중 상태");	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_UsingConsumable, "State.UsingConsumable", "플레이어 소비템 사용중 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Suppress_Fire_Primary, "State.Suppress.Fire.Primary", "무기 기본 발사 억제 상태 (모드 스킬 발사로 전환된 상태)");

	// ===== Cooldown.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_Dodge, "Cooldown.Ability.Dodge", "구르기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_Fire, "Cooldown.Ability.Fire", "무기 발사 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_PlayerHitReact, "Cooldown.Ability.PlayerHitReact", "플레이어 피격 리액션 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_SwapWeapon, "Cooldown.Ability.SwapWeapon", "무기 교체 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Enemy_Evade, "Cooldown.Enemy.Evade", "일반 적 회피 쿨다운");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_TeleportLunge, "Cooldown.Boss.Faerin.TeleportLunge", "Faerin Teleport Lunge 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_PortalMissile, "Cooldown.Boss.Faerin.PortalMissile", "Faerin 포털 미사일 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_PortalPair, "Cooldown.Boss.Faerin.PortalPair", "Faerin 포털 2개 설치 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_PortalBarrage, "Cooldown.Boss.Faerin.PortalBarrage", "Faerin 포털 연속 포격 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_PortalAttached, "Cooldown.Boss.Faerin.PortalAttached", "Faerin 부착 포털 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_PortalTorrent, "Cooldown.Boss.Faerin.PortalTorrent", "Faerin Torrent 포털 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_RainPortal, "Cooldown.Boss.Faerin.RainPortal", "Faerin Rain Portal 패턴 쿨다운");


	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_Shift, "Cooldown.Boss.Faerin.Shift", "Faerin Shift 계열 공용 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_TeleportDown, "Cooldown.Boss.Faerin.TeleportDown", "Faerin TeleportDown 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_Crash, "Cooldown.Boss.Faerin.Crash", "Faerin Crash 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_Throw, "Cooldown.Boss.Faerin.Throw", "Faerin Throw 패턴 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Faerin_SpokeCombo, "Cooldown.Boss.Faerin.SpokeCombo", "Faerin Spoke Combo 패턴 쿨다운");

	// ===== Event.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hit, "Event.Hit", "피격 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Engaged, "Event.Combat.Engaged", "전투 교전 유지 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Death, "Event.Ability.Death", "사망 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Down, "Event.Ability.Down", "다운 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_GetUp, "Event.Ability.GetUp", "기상 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Revive_Start, "Event.Ability.Revive.Start", "소생 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Revive_Finished, "Event.Ability.Revive.Finished", "소생 완료 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Revive_Canceled, "Event.Ability.Revive.Canceled", "소생 취소 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Waypoint_Start, "Event.Ability.Waypoint.Start", "크리스탈 상호작용 시작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Waypoint_End, "Event.Ability.Waypoint.End", "크리스탈 상호작용 취소");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_GroggyEntered, "Event.Ability.GroggyEntered", "그로기 진입 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PhaseTransition, "Event.Ability.PhaseTransition", "페이즈 전환 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Reload, "Event.Ability.Reload", "재장전 시작 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PlayerHitReact_Weak, "Event.Ability.PlayerHitReact.Weak", "플레이어 약한 경직 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PlayerHitReact_Strong, "Event.Ability.PlayerHitReact.Strong", "플레이어 강한 경직 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PlayerHitReact_Down, "Event.Ability.PlayerHitReact.Down", "플레이어 다운 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PlayerDeathConfirmed, "Event.Ability.Player.DeathConfirmed", "플레이어 다운 사망 전환 확정 이벤트");

	// ===== Event.Player.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_HitShot, "Event.Player.HitShot", "사격이 적중했을 때");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Recoil, "Event.Player.Recoil", "사격 반동 발생 (FPRRecoilEventPayload 동반)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Aim_Start, "Event.Player.Aim.Start", "에이밍 진입");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Aim_End, "Event.Player.Aim.End", "에이밍 해제");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_ChangeCrosshair, "Event.Player.ChangeCrosshair", "크로스헤어 Config 교체");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_PreviewHit, "Event.Player.PreviewHit", "히트스캔 미리보기 적중 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_ReloadCommit, "Event.Player.ReloadCommit", "재장전 자원 이동 트리거");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_ConsumableCommit, "Event.Player.ConsumableCommit", "소비 아이템 사용 확정 트리거");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Interaction_Hold, "Event.Player.Interaction.Hold", "Hold 상호작용 단계 알림");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Interactable, "Event.Player.Interactable", "상호작용 가능 알림");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_ModActivation, "Event.Player.ModActivation", "Mod On/Off 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_WorldMarker, "Event.Player.WorldMarker", "월드 마커 추가 제거 알림");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_HUDMessage, "Event.Player.HUDMessage", "HUD 안내 메시지 표시 알림");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_AttackTarget, "Event.Player.AttackTarget", "플레이어 실제 피해 대상 알림");

	// ===== Event.Boss.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_Spawn, "Event.Boss.Spawn", "보스 스폰 요청");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_Encounter_Begin, "Event.Boss.Encounter.Begin", "보스 조우 시작 (FPRBossEncounterEventPayload 동반)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_Encounter_End, "Event.Boss.Encounter.End", "보스 조우 종료");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_Defeated, "Event.Boss.Defeated", "보스 처치 확정");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_PhaseChanged, "Event.Boss.PhaseChanged", "보스 페이즈 변경");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_BGMPhasePreview, "Event.Boss.BGMPhasePreview", "보스 BGM 페이즈 예고");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_BGMPatternCue, "Event.Boss.BGMPatternCue", "보스 BGM 특수패턴 Cue");

	// ===== Cue.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cue_Critical_Boss_PhaseTransition, "Cue.Critical.Boss.PhaseTransition", "보스 페이즈 전환 연출 Cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cue_Critical_Boss_WeakpointOpen, "Cue.Critical.Boss.WeakpointOpen", "보스 약점 창 오픈 연출 Cue");

	// ===== SetByCaller.Trait.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_MaxHealth, "SetByCaller.Trait.MaxHealth", "특성 최대 체력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_Armor, "SetByCaller.Trait.Armor", "특성 방어력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_MovementSpeed, "SetByCaller.Trait.MovementSpeed", "특성 이동속도 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_PlayerAttackPower, "SetByCaller.Trait.PlayerAttackPower", "특성 플레이어 공격력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_MaxStamina, "SetByCaller.Trait.MaxStamina", "특성 최대 스태미너 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_CriticalHitChance, "SetByCaller.Trait.CriticalHitChance", "특성 치명타 확률 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Trait_CriticalDamageMultiplier, "SetByCaller.Trait.CriticalDamageMultiplier", "특성 치명타 피해 배율 보너스");

	// ===== SetByCaller.Equipment.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equipment_MaxHealth, "SetByCaller.Equipment.MaxHealth", "장비 최대 체력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equipment_Armor, "SetByCaller.Equipment.Armor", "장비 방어력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equipment_PlayerAttackPower, "SetByCaller.Equipment.PlayerAttackPower", "장비 플레이어 공격력 보너스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equipment_MaxStamina, "SetByCaller.Equipment.MaxStamina", "장비 최대 스태미너 보너스");

	// ===== Input.Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Fire_Primary, "Input.Ability.Fire.Primary", "기본 발사 입력 태그 (모드 스킬의 발사가 아닌 무기의 기본 발사)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Dodge, "Input.Ability.Dodge", "구르기 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Interact, "Input.Ability.Interact", "상호작용용 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Ping, "Input.Ability.Ping", "핑 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Crouch, "Input.Ability.Crouch", "앉기용 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Aim, "Input.Ability.Aim", "에이밍 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Sprint, "Input.Ability.Sprint", "질주 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Mod, "Input.Ability.Mod", "모드 스킬 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Reload, "Input.Ability.Reload", "재장전 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_SwapWeapon, "Input.Ability.SwapWeapon", "주무기/보조무기 교체 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Zoom, "Input.Ability.Zoom", "무기 줌 입력 태그");

	// ===== Input.Locomotion.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Locomotion_Walk, "Input.Locomotion.Walk", "Walk 토글 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Locomotion_Sprint, "Input.Locomotion.Sprint", "Sprint 토글 입력 태그");
	
	// ===== Fail.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cost, "Fail.Cost", "Cost GE 자원 부족");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cooldown, "Fail.Cooldown", "쿨다운 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Blocked, "Fail.Blocked", "Block 태그에 의해 차단");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Invalid, "Fail.Invalid", "어빌리티/소유자 상태 유효하지 않음");
}
