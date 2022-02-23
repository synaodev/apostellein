#include <limits>
#include <memory>
#include <map>
#include <optional>
#include <spdlog/spdlog.h>
#include <SDL2/SDL_events.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./input.hpp"
#include "../util/buttons.hpp"
#include "../util/config-file.hpp"

namespace {
	constexpr i32 MAXIMUM_JOYSTICK_CODES = 32;
	constexpr i16 AXIS_DEAD_ZONE = std::numeric_limits<i16>::max() / 2;
}

// private
namespace input {
	// driver
	struct driver {
	public:
		config_file* config {};
		SDL_Joystick* joystick {};
		bool(*callback)(const void*) {};
		std::optional<i32> stored_code {};
		std::optional<SDL_Scancode> debugger_code {};
		bool listening_for_keyboard {};
		bool listening_for_joystick {};
		std::map<SDL_Scancode, u32> keyboard_bindings {};
		std::map<i32, u32> joystick_bindings {};
	};
	std::unique_ptr<driver> drv_ {};
	// functions
	void init_keyboard_bindings_(const config_file& cfg) {
		drv_->keyboard_bindings.clear();
		if (auto code = cfg.debugger_binding(); code != SDL_SCANCODE_UNKNOWN) {
			drv_->debugger_code = static_cast<SDL_Scancode>(code);
		}
		for (auto it = button_name::FIRST_BUTTON; it != button_name::INVALID_BUTTON; ++it) {
			auto code = cfg.keyboard_binding(it);
			if (drv_->debugger_code and *drv_->debugger_code == code) {
				spdlog::error("Debugger key cannot share bindings with other keys!");
			} else if (code != SDL_SCANCODE_UNKNOWN and code < SDL_NUM_SCANCODES) {
				drv_->keyboard_bindings[static_cast<SDL_Scancode>(code)] = it;
			}
		}
	}

	void init_joystick_bindings_(const config_file& cfg) {
		drv_->joystick_bindings.clear();
		for (auto it = button_name::FIRST_BUTTON; it != button_name::INVALID_BUTTON; ++it) {
			const auto code = cfg.joystick_binding(it);
			if (code >= 0 and code < MAXIMUM_JOYSTICK_CODES) {
				drv_->joystick_bindings[code] = it;
			}
		}
	}

	bool init_(config_file& cfg) {
		// Create driver
		if (drv_) {
			spdlog::critical("Input system already has active driver!");
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Get config
		drv_->config = &cfg;
		input::init_keyboard_bindings_(cfg);
		input::init_joystick_bindings_(cfg);

		// Create joystick handle
		if (SDL_NumJoysticks() != 0) {
			if (drv_->joystick = SDL_JoystickOpen(0); !drv_->joystick) {
				spdlog::warn("Joystick couldn't open at startup! SDL Error: {}", SDL_GetError());
			}
		}

		return true;
	}

	void drop_() {
		if (drv_) {
			if (drv_->joystick) {
				SDL_JoystickClose(drv_->joystick);
				drv_->joystick = nullptr;
			}
			drv_.reset();
		}
	}

	guard::guard(config_file& cfg) {
		if (input::init_(cfg)) {
			ready_ = true;
		} else {
			input::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			input::drop_();
		}
	}
}

// public
void input::zero(buttons& bts) {
	bts.pressed._raw = {};
	bts.holding._raw = {};
	bts.released._raw = {};
}

void input::callback(bool(*function)(const void*)) {
	if (!drv_) {
		return;
	}
	if (function) {
		drv_->callback = function;
	}
}

bool input::poll(activity_type& aty, buttons& bts) {
	if (!drv_) {
		return false;
	}
	SDL_Event event {};
	while (SDL_PollEvent(&event)) {
		if (drv_->callback) {
			drv_->callback(&event);
		}
		switch (event.type) {
			case SDL_QUIT: {
				aty = activity_type::quitting;
				break;
			}
			case SDL_WINDOWEVENT: {
				if (event.window.type == SDL_WINDOWEVENT_FOCUS_GAINED and aty == activity_type::stopped) {
					aty = activity_type::running;
				} else if (event.window.type == SDL_WINDOWEVENT_FOCUS_LOST and aty == activity_type::running) {
					aty = activity_type::stopped;
					input::zero(bts);
				}
				break;
			}
			case SDL_KEYDOWN: {
				const auto code = event.key.keysym.scancode;
				if (auto iter = drv_->keyboard_bindings.find(code); iter != drv_->keyboard_bindings.end()) {
					const auto name = iter->second;
					bool holding = bts.holding._raw.get(name);
					bts.pressed._raw.set(name, !holding);
					bts.holding._raw.set(name, true);
				}
				if (drv_->debugger_code and *drv_->debugger_code == code) {
					bts.pressed.debugger = !bts.holding.debugger;
					bts.holding.debugger = true;
				} else if (drv_->listening_for_keyboard) {
					drv_->stored_code = code;
				}
				break;
			}
			case SDL_KEYUP: {
				const auto code = event.key.keysym.scancode;
				if (auto iter = drv_->keyboard_bindings.find(code); iter != drv_->keyboard_bindings.end()) {
					auto name = iter->second;
					bts.holding._raw.set(name, false);
					bts.released._raw.set(name, true);
				}
				if (drv_->debugger_code and *drv_->debugger_code == code) {
					bts.holding.debugger = false;
					bts.released.debugger = true;
				}
				break;
			}
			case SDL_JOYAXISMOTION: {
				if (event.jaxis.which == 0) {
					const auto index = event.jaxis.axis;
					const auto value = event.jaxis.value;
					if (index == 0) {
						if (value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding.right;
							bts.pressed.right = !holding;
							bts.holding.right = true;
							bts.holding.left = false;
						} else if (value < -AXIS_DEAD_ZONE) {
							bool holding = bts.holding.left;
							bts.pressed.left = !holding;
							bts.holding.left = true;
							bts.holding.right = false;
						} else {
							bts.holding.right = false;
							bts.holding.left = false;
						}
					} else if (index == 1) {
						if (value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding.down;
							bts.pressed.down = !holding;
							bts.holding.down = true;
							bts.holding.up = false;
						} else if (value < -AXIS_DEAD_ZONE) {
							bool holding = bts.holding.up;
							bts.pressed.up = !holding;
							bts.holding.up = true;
							bts.holding.down = false;
						} else {
							bts.holding.up = false;
							bts.holding.down = false;
						}
					}
				}
				break;
			}
			case SDL_JOYHATMOTION: {
				if (event.jhat.which == 0) {
					auto value = event.jhat.value;
					if (value & SDL_HAT_UP) {
						bool holding = bts.holding.up;
						bts.pressed.up = !holding;
						bts.holding.up = true;
						bts.holding.down = false;
					} else if (value & SDL_HAT_DOWN) {
						bool holding = bts.holding.down;
						bts.pressed.down = !holding;
						bts.holding.down = true;
						bts.holding.up = false;
					} else if (value & SDL_HAT_RIGHT) {
						bool holding = bts.holding.right;
						bts.pressed.right = !holding;
						bts.holding.right = true;
						bts.holding.left = false;
					} else if (value & SDL_HAT_LEFT) {
						bool holding = bts.holding.left;
						bts.pressed.left = !holding;
						bts.holding.left = true;
						bts.holding.right = false;
					} else {
						bts.holding.right = false;
						bts.holding.left = false;
						bts.holding.up = false;
						bts.holding.down = false;
					}
				}
				break;
			}
			case SDL_JOYBUTTONDOWN: {
				if (event.jbutton.which == 0) {
					const auto code = as<i32>(event.jbutton.button);
					if (auto iter = drv_->joystick_bindings.find(code); iter != drv_->joystick_bindings.end()) {
						const auto name = iter->second;
						bool holding = bts.holding._raw.get(name);
						bts.pressed._raw.set(name, !holding);
						bts.holding._raw.set(name, true);
					}
					if (drv_->listening_for_joystick) {
						drv_->stored_code = code;
					}
				}
				break;
			}
			case SDL_JOYBUTTONUP: {
				if (event.jbutton.which == 0) {
					const auto code = as<i32>(event.jbutton.button);
					if (auto iter = drv_->joystick_bindings.find(code); iter != drv_->joystick_bindings.end()) {
						const auto name = iter->second;
						bts.holding._raw.set(name, false);
						bts.released._raw.set(name, true);
					}
				}
				break;
			}
			case SDL_JOYDEVICEADDED: {
				if (drv_->listening_for_keyboard) {
					drv_->listening_for_keyboard = false;
					drv_->listening_for_joystick = true;
				}
				if (event.jdevice.which == 0 and !drv_->joystick) {
					if (drv_->joystick = SDL_JoystickOpen(0); !drv_->joystick) {
						spdlog::warn("Couldn't open joystick device! SDL Error: {}", SDL_GetError());
					}
				}
				break;
			}
			case SDL_JOYDEVICEREMOVED: {
				if (drv_->listening_for_joystick) {
					drv_->listening_for_keyboard = true;
					drv_->listening_for_joystick = false;
				}
				if (event.jdevice.which == 0 and drv_->joystick) {
					SDL_JoystickClose(drv_->joystick);
					drv_->joystick = nullptr;
				}
				break;
			}
			default:
				break;
		}
	}
	return aty != activity_type::quitting;
}

bool input::joystick_attached() {
	if (!drv_) {
		return false;
	}
	return drv_->joystick;
}

bool input::valid_stored_code() {
	if (!drv_) {
		return false;
	}
	if (drv_->listening_for_keyboard or drv_->listening_for_joystick) {
		return drv_->stored_code.has_value();
	}
	return false;
}

i32 input::receive_stored_code() {
	if (!drv_) {
		return SDL_SCANCODE_UNKNOWN;
	}
	if (!drv_->stored_code) {
		return SDL_SCANCODE_UNKNOWN;
	}
	auto value = *drv_->stored_code;
	drv_->stored_code = std::nullopt;
	drv_->listening_for_keyboard = false;
	drv_->listening_for_joystick = false;
	return value;
}

void input::listen_to_keyboard() {
	if (!drv_) {
		return;
	}
	drv_->listening_for_keyboard = true;
	drv_->listening_for_joystick = false;
	drv_->stored_code = std::nullopt;
}

void input::listen_to_joystick() {
	if (!drv_) {
		return;
	}
	drv_->listening_for_keyboard = false;
	drv_->listening_for_joystick = true;
	drv_->stored_code = std::nullopt;
}

void input::stop_listening() {
	if (!drv_) {
		return;
	}
	drv_->listening_for_keyboard = false;
	drv_->listening_for_joystick = false;
	drv_->stored_code = std::nullopt;
}

void input::swap_keyboard_bindings(i32 raw_code, u32 name) {
	if (!drv_) {
		return;
	}
	if (name > button_name::LAST_ORDINAL) {
		name = button_name::LAST_ORDINAL;
	}
	std::optional<SDL_Scancode> found;
	for (auto&& [code, btn] : drv_->keyboard_bindings) {
		if (btn == name) {
			found = code;
			break;
		}
	}
	if (found) {
		const auto code = static_cast<SDL_Scancode>(raw_code);
		if (drv_->keyboard_bindings.find(code) != drv_->keyboard_bindings.end()) {
			auto swapped = drv_->keyboard_bindings[code];
			drv_->keyboard_bindings[*found] = swapped;
			drv_->keyboard_bindings[code] = name;

			drv_->config->keyboard_binding(name, code);
			drv_->config->keyboard_binding(swapped, *found);
		} else {
			drv_->keyboard_bindings.erase(*found);
			drv_->keyboard_bindings[code] = name;

			drv_->config->keyboard_binding(name, code);
		}
	}
}

void input::swap_joystick_bindings(i32 raw_code, u32 name) {
	if (!drv_) {
		return;
	}
	if (name > button_name::LAST_BUTTON) {
		name = button_name::LAST_BUTTON;
	}
	std::optional<i32> found;
	for (auto&& [code, btn] : drv_->joystick_bindings) {
		if (btn == name) {
			found = code;
			break;
		}
	}
	if (found) {
		const auto code = raw_code;
		if (drv_->joystick_bindings.find(code) != drv_->joystick_bindings.end()) {
			auto swapped = drv_->joystick_bindings[code];
			drv_->joystick_bindings[*found] = swapped;
			drv_->joystick_bindings[code] = name;

			drv_->config->joystick_binding(name, code);
			drv_->config->joystick_binding(swapped, *found);
		} else {
			drv_->joystick_bindings.erase(*found);
			drv_->joystick_bindings[code] = name;

			drv_->config->joystick_binding(name, code);
		}
	}
}

std::string input::keyboard_string(u32 name) {
	if (!drv_) {
		return {};
	}
	for (auto&& [code, btn] : drv_->keyboard_bindings) {
		if (btn == name) {
			if (auto str = SDL_GetScancodeName(code); str) {
				return str;
			}
		}
	}
	return {};
}

std::string input::joystick_string(u32 name) {
	if (!drv_) {
		return {};
	}
	for (auto&& [code, btn] : drv_->joystick_bindings) {
		if (btn == name) {
			return std::to_string(code);
		}
	}
	return {};
}
