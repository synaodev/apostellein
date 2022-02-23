#pragma once

#include <string>
#include <apostellein/struct.hpp>

struct buttons;
struct config_file;

enum class activity_type {
	running,
	stopped,
	quitting
};

namespace input {
	// functions
	void zero(buttons& bts);
	void callback(bool(*function)(const void*));
	bool poll(activity_type& aty, buttons& bts);
	bool joystick_attached();
	bool valid_stored_code();
	i32 receive_stored_code();
	void listen_to_keyboard();
	void listen_to_joystick();
	void stop_listening();
	void swap_keyboard_bindings(i32 raw_code, u32 name);
	void swap_joystick_bindings(i32 raw_code, u32 name);
	std::string keyboard_string(u32 name);
	std::string joystick_string(u32 name);
	// Init-Guard
	struct guard : public not_moveable {
		guard(config_file& cfg);
		~guard();
	public:
		operator bool() const { return ready_; }
	private:
		bool ready_ {};
	};
}
