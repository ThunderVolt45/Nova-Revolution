#include "NovaGameplayTags.h"

namespace NovaGameplayTags
{
	// 기본 조작
	UE_DEFINE_GAMEPLAY_TAG(Input_Select, "Input.Select");
	UE_DEFINE_GAMEPLAY_TAG(Input_Command, "Input.Command");

	// 표준 명령
	UE_DEFINE_GAMEPLAY_TAG(Input_Move, "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Stop, "Input.Stop");
	UE_DEFINE_GAMEPLAY_TAG(Input_Hold, "Input.Hold");
	UE_DEFINE_GAMEPLAY_TAG(Input_Patrol, "Input.Patrol");

	// 고유 명령
	UE_DEFINE_GAMEPLAY_TAG(Input_Spread, "Input.Spread");
	UE_DEFINE_GAMEPLAY_TAG(Input_Halt, "Input.Halt");
	UE_DEFINE_GAMEPLAY_TAG(Input_SelectBase, "Input.SelectBase");

	// 숫자 슬롯 (1~0)
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

	// 자원 관련 데이터
	UE_DEFINE_GAMEPLAY_TAG(Data_Resource_Watt, "Data.Resource.Watt");
	UE_DEFINE_GAMEPLAY_TAG(Data_Resource_SP, "Data.Resource.SP");
}
