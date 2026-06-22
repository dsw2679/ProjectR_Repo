// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 및 피격 경직 관련 전투 태그 정의)
// Author: 배유찬 (사격, 탄약 및 기본 데미지 계산 관련 태그 정의)
// Author: 손승우 (아머드 솔저 해머 전투 및 그로기 관련 전투 태그 정의)
// Author: 이건주 (배리어/드론 및 Penitent 몬스터 관련 전투 태그 정의)
#pragma once

#include "NativeGameplayTags.h"

// 전투 파이프라인 내부 GameplayTag 모음
// 전투 전용 SetByCaller/Event Native Tag 등록 위치
namespace PRCombatGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage); // 모드 데미지에 활용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_GroggyDamage); // 모드 그로기 데미지에 활용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AttackMultiplier); // 적 어빌리티 공격 배수
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseDamage); // 적 어빌리티 강인도 피해
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AmmoMagnitude); // 탄약 획득량
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AmmoCost); // 사격 시 소모 발수
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cooldown); // 쿨다운 GE Duration SetByCaller 키
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModDuration); // 지속시간형 Mod 유지 시간
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModMaxGauge); // 지속시간형 Mod 게이지 총 소모량
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CurrentWeapon_BaseDamage); // 현재 무기 기본 피해
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CurrentWeapon_ArmorPenetration); // 현재 무기 장갑 관통
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CurrentWeapon_WeakpointMultiplier); // 현재 무기 약점 피해 배율
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CurrentWeapon_GroggyDamageMultiplier); // 현재 무기 그로기 피해 배율
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryModGauge); // 주무기 Mod 게이지
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryMaxModGauge); // 주무기 Mod 최대 게이지
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryModStack); // 주무기 Mod 스택
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryMaxModStack); // 주무기 Mod 최대 스택
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryMagazineAmmo); // 주무기 탄창 현재값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryMaxMagazineAmmo); // 주무기 탄창 최대값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryReserveAmmo); // 주무기 예비탄 현재값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_PrimaryMaxReserveAmmo); // 주무기 예비탄 최대값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryModGauge); // 보조무기 Mod 게이지
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryMaxModGauge); // 보조무기 Mod 최대 게이지
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryModStack); // 보조무기 Mod 스택
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryMaxModStack); // 보조무기 Mod 최대 스택
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryMagazineAmmo); // 보조무기 탄창 현재값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryMaxMagazineAmmo); // 보조무기 탄창 최대값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryReserveAmmo); // 보조무기 예비탄 현재값
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Equip_SecondaryMaxReserveAmmo); // 보조무기 예비탄 최대값
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModGauge_Primary); // 주무기 모드게이지 충전량
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModGauge_Secondary); // 보조무기 모드게이지 충전량

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeHit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowBegin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowTick);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowEnd);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyComboWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PenitentBarrierSummon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PenitentBarrierLaunch);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Weapon_Primary);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Weapon_Secondary);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Drone);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyProjectileFire);
}
