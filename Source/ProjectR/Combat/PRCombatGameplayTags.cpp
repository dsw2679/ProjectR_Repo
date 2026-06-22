// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 및 피격 경직 관련 전투 태그 정의)
// Author: 배유찬 (사격, 탄약 및 기본 데미지 계산 관련 태그 정의)
// Author: 손승우 (아머드 솔저 해머 전투 및 그로기 관련 전투 태그 정의)
// Author: 이건주 (배리어/드론 및 Penitent 몬스터 관련 전투 태그 정의)
#include "PRCombatGameplayTags.h"

namespace PRCombatGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_GroggyDamage, "SetByCaller.GroggyDamage", "그로기 피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_AttackMultiplier, "SetByCaller.AttackMultiplier", "피해배수 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PoiseDamage, "SetByCaller.PoiseDamage", "강인도 피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_AmmoMagnitude, "SetByCaller.AmmoMagnitude", "탄약 획득량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_AmmoCost, "SetByCaller.AmmoCost", "사격 시 소모 발수");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Cooldown, "SetByCaller.Cooldown", "쿨다운 GE Duration SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ModDuration, "SetByCaller.Mod.Duration", "지속시간형 Mod 유지 시간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ModMaxGauge, "SetByCaller.Mod.MaxGauge", "지속시간형 Mod 게이지 총 소모량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CurrentWeapon_BaseDamage, "SetByCaller.CurrentWeapon.BaseDamage", "현재 무기 기본 피해");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CurrentWeapon_ArmorPenetration, "SetByCaller.CurrentWeapon.ArmorPenetration", "현재 무기 장갑 관통");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CurrentWeapon_WeakpointMultiplier, "SetByCaller.CurrentWeapon.WeakpointMultiplier", "현재 무기 약점 피해 배율");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CurrentWeapon_GroggyDamageMultiplier, "SetByCaller.CurrentWeapon.GroggyDamageMultiplier", "현재 무기 그로기 피해 배율");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryModGauge, "SetByCaller.Equip.Primary.ModGauge", "주무기 Mod 게이지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryMaxModGauge, "SetByCaller.Equip.Primary.MaxModGauge", "주무기 Mod 최대 게이지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryModStack, "SetByCaller.Equip.Primary.ModStack", "주무기 Mod 스택");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryMaxModStack, "SetByCaller.Equip.Primary.MaxModStack", "주무기 Mod 최대 스택");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryMagazineAmmo, "SetByCaller.Equip.Primary.MagazineAmmo", "주무기 탄창 현재값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryMaxMagazineAmmo, "SetByCaller.Equip.Primary.MaxMagazineAmmo", "주무기 탄창 최대값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryReserveAmmo, "SetByCaller.Equip.Primary.ReserveAmmo", "주무기 예비탄 현재값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_PrimaryMaxReserveAmmo, "SetByCaller.Equip.Primary.MaxReserveAmmo", "주무기 예비탄 최대값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryModGauge, "SetByCaller.Equip.Secondary.ModGauge", "보조무기 Mod 게이지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryMaxModGauge, "SetByCaller.Equip.Secondary.MaxModGauge", "보조무기 Mod 최대 게이지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryModStack, "SetByCaller.Equip.Secondary.ModStack", "보조무기 Mod 스택");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryMaxModStack, "SetByCaller.Equip.Secondary.MaxModStack", "보조무기 Mod 최대 스택");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryMagazineAmmo, "SetByCaller.Equip.Secondary.MagazineAmmo", "보조무기 탄창 현재값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryMaxMagazineAmmo, "SetByCaller.Equip.Secondary.MaxMagazineAmmo", "보조무기 탄창 최대값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryReserveAmmo, "SetByCaller.Equip.Secondary.ReserveAmmo", "보조무기 예비탄 현재값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Equip_SecondaryMaxReserveAmmo, "SetByCaller.Equip.Secondary.MaxReserveAmmo", "보조무기 예비탄 최대값");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ModGauge_Primary, "SetByCaller.ModGauge.Primary","주무기 모드게이지 충전량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ModGauge_Secondary,"SetByCaller.ModGauge.Secondary","보조무기 모드게이지 충전량"); 
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeHit, "Event.Ability.EnemyMeleeHit", "적 근접 공격 타격 프레임 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowBegin, "Event.Ability.EnemyMeleeWindowBegin", "적 근접 공격 판정 구간 시작 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowTick, "Event.Ability.EnemyMeleeWindowTick", "적 근접 공격 판정 구간 갱신 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowEnd, "Event.Ability.EnemyMeleeWindowEnd", "적 근접 공격 판정 구간 종료 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyComboWindow, "Event.Ability.EnemyComboWindow", "적 콤보 분기 윈도우 이벤트");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Weapon_Primary, "Ability.Source.Weapon.Primary", "주무기에 의한 대미지 컨텍스트 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Weapon_Secondary, "Ability.Source.Weapon.Secondary", "보조무기에 의한 대미지 컨텍스트 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Drone, "Ability.Source.Drone", "드론 보조 공격 대미지 컨텍스트 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyProjectileFire, "Event.Ability.EnemyProjectileFire", "적 투사체 발사 프레임 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PenitentBarrierSummon, "Event.Ability.Penitent.BarrierSummon", "Penitent 배리어 소환 프레임 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PenitentBarrierLaunch, "Event.Ability.Penitent.BarrierLaunch", "Penitent 배리어 발사 프레임 이벤트");
}
