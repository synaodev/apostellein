#pragma once

#include <apostellein/def.hpp>

namespace button_name {
	constexpr u32 JUMP = 0;
	constexpr u32 CONFIRM = JUMP;
	constexpr u32 ARMS = 1;
	constexpr u32 CANCEL = ARMS;
	constexpr u32 ITEM = 2;
	constexpr u32 PROVISION = ITEM;
	constexpr u32 STRAFE = 3;
	constexpr u32 APOSTLE = 4;
	constexpr u32 INVENTORY = 5;
	constexpr u32 OPTIONS = 6;
	constexpr u32 UP = 7;
	constexpr u32 DOWN = 8;
	constexpr u32 LEFT = 9;
	constexpr u32 RIGHT = 10;
	constexpr u32 DEBUGGER = 11;
	constexpr u32 TRIGGER_LEFT = 29;
	constexpr u32 TRIGGER_RIGHT = 30;

	constexpr u32 FIRST_BUTTON = JUMP;
	constexpr u32 LAST_BUTTON = OPTIONS;
	constexpr u32 FIRST_ORDINAL = UP;
	constexpr u32 LAST_ORDINAL = RIGHT;
	constexpr u32 INVALID_BUTTON = DEBUGGER;
}

struct buttons {
	constexpr buttons() noexcept = default;
	union internals {
		bitfield_raw<u32> _raw {};
		bitfield_index<u32, button_name::JUMP> jump;
		bitfield_index<u32, button_name::CONFIRM> confirm;
		bitfield_index<u32, button_name::ARMS> arms;
		bitfield_index<u32, button_name::ARMS> cancel;
		bitfield_index<u32, button_name::ITEM> item;
		bitfield_index<u32, button_name::PROVISION> provision;
		bitfield_index<u32, button_name::STRAFE> strafe;
		bitfield_index<u32, button_name::APOSTLE> apostle;
		bitfield_index<u32, button_name::INVENTORY> inventory;
		bitfield_index<u32, button_name::OPTIONS> options;
		bitfield_index<u32, button_name::UP> up;
		bitfield_index<u32, button_name::DOWN> down;
		bitfield_index<u32, button_name::LEFT> left;
		bitfield_index<u32, button_name::RIGHT> right;
		bitfield_index<u32, button_name::DEBUGGER> debugger;
		bitfield_index<u32, button_name::TRIGGER_LEFT> _trigger_left;
		bitfield_index<u32, button_name::TRIGGER_RIGHT> _trigger_right;
	};
	internals pressed {};
	internals holding {};
	internals released {};
public:
	constexpr void clear() {
		pressed._raw = {};
		released._raw = {};
	}
};
