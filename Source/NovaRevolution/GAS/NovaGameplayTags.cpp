#include "NovaGameplayTags.h"

namespace NovaGameplayTags
{
	// 기본 조작
	UE_DEFINE_GAMEPLAY_TAG(Input_Select, "Input.Select");
	UE_DEFINE_GAMEPLAY_TAG(Input_Command, "Input.Command");
	UE_DEFINE_GAMEPLAY_TAG(Input_Zoom, "Input.Zoom");

	// 표준 명령
	UE_DEFINE_GAMEPLAY_TAG(Input_Move, "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Stop, "Input.Stop");
	UE_DEFINE_GAMEPLAY_TAG(Input_Hold, "Input.Hold");
	UE_DEFINE_GAMEPLAY_TAG(Input_Patrol, "Input.Patrol");

	// 조합키
	UE_DEFINE_GAMEPLAY_TAG(Input_Modifier_Shift, "Input.Modifier.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Input_Modifier_Ctrl, "Input.Modifier.Ctrl");
	UE_DEFINE_GAMEPLAY_TAG(Input_Modifier_Alt, "Input.Modifier.Alt");

	// 고유 명령
	UE_DEFINE_GAMEPLAY_TAG(Input_Spread, "Input.Spread");
	UE_DEFINE_GAMEPLAY_TAG(Input_Halt, "Input.Halt");
	UE_DEFINE_GAMEPLAY_TAG(Input_SelectBase, "Input.SelectBase");

	// 숫자 슬롯 (1~0), 편의성을 위해 7~0 까지는 각각 Q, W, E, R로도 입력받을 수 있게 추가 매핑
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_1, "Input.Slot.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_2, "Input.Slot.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_3, "Input.Slot.3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_4, "Input.Slot.4");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_5, "Input.Slot.5");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_6, "Input.Slot.6");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_7, "Input.Slot.7");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_8, "Input.Slot.8");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_9, "Input.Slot.9");
	UE_DEFINE_GAMEPLAY_TAG(Input_Slot_0, "Input.Slot.0");

	// 추가 단축키
	UE_DEFINE_GAMEPLAY_TAG(Input_Camera_Reset, "Input.Camera.Reset"); // Home
	UE_DEFINE_GAMEPLAY_TAG(Input_Toggle_HealthBar, "Input.Toggle.HealthBar");
	// 자원 관련 데이터
	UE_DEFINE_GAMEPLAY_TAG(Data_Resource_Watt, "Data.Resource.Watt");
	UE_DEFINE_GAMEPLAY_TAG(Data_Resource_SP, "Data.Resource.SP");
	
	// --- GE 관련 식별 태그 ---
	UE_DEFINE_GAMEPLAY_TAG(Effect_Resource_Regen, "Effect.Resource.Regen");
	
	// --- 어빌리티 태그 ---
	// 유닛
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	
	// 사령관 스킬
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_ResourceLevelUp, "Ability.Skill.ResourceLevelUp");
	
	// --- 게임플레이 큐 태그 ---
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Rifle_Fire, "GameplayCue.Weapon.Rifle.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Rifle_Hit, "GameplayCue.Weapon.Rifle.Hit");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Rocketeer_Fire, "GameplayCue.Weapon.Rocketeer.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Rocketeer_Hit, "GameplayCue.Weapon.Rocketeer.Hit");
}
