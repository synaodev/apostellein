#include <limits>
#include <memory>
#include <map>
#include <optional>
#include <spdlog/spdlog.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./input.hpp"
#include "../util/buttons.hpp"
#include "../util/config-file.hpp"

namespace {
	constexpr i32 MAXIMUM_JOYSTICK_CODES = 32;
	constexpr i32 JOYSTICK_CODE_TRIGGER_LEFT = as<i32>(button_name::TRIGGER_LEFT);
	constexpr i32 JOYSTICK_CODE_TRIGGER_RIGHT = as<i32>(button_name::TRIGGER_RIGHT);
	constexpr i16 AXIS_DEAD_ZONE = std::numeric_limits<i16>::max() / 2;
}

// private
namespace input {
	// driver
	struct driver {
	public:
		config_file* config {};
		SDL_GameController* device {};
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
	std::string find_correct_joystick_name_(i32 code) {
		if (drv_->device) {
			switch (SDL_GameControllerGetType(drv_->device)) {
				case SDL_CONTROLLER_TYPE_XBOX360:
				case SDL_CONTROLLER_TYPE_XBOXONE:
				case SDL_CONTROLLER_TYPE_VIRTUAL: {
					switch (code) {
						case SDL_CONTROLLER_BUTTON_A: return "A";
						case SDL_CONTROLLER_BUTTON_B: return "B";
						case SDL_CONTROLLER_BUTTON_X: return "X";
						case SDL_CONTROLLER_BUTTON_Y: return "Y";
						case SDL_CONTROLLER_BUTTON_BACK: return "Back";
						case SDL_CONTROLLER_BUTTON_START: return "Start";
						case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "Left Stick";
						case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "Right Stick";
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "Left Shoulder";
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "Right Shoulder";
						case JOYSTICK_CODE_TRIGGER_LEFT: return "Left Trigger";
						case JOYSTICK_CODE_TRIGGER_RIGHT: return "Right Trigger";
						default: break;
					}
					break;
				}
				case SDL_CONTROLLER_TYPE_PS3:
				case SDL_CONTROLLER_TYPE_PS4:
				case SDL_CONTROLLER_TYPE_PS5: {
					switch (code) {
						case SDL_CONTROLLER_BUTTON_A: return "Cross";
						case SDL_CONTROLLER_BUTTON_B: return "Circle";
						case SDL_CONTROLLER_BUTTON_X: return "Square";
						case SDL_CONTROLLER_BUTTON_Y: return "Triangle";
						case SDL_CONTROLLER_BUTTON_BACK: return "Share";
						case SDL_CONTROLLER_BUTTON_START: return "Option";
						case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
						case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "L1";
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R1";
						case SDL_CONTROLLER_BUTTON_TOUCHPAD: return "Touchpad";
						case JOYSTICK_CODE_TRIGGER_LEFT: return "L2";
						case JOYSTICK_CODE_TRIGGER_RIGHT: return "R2";
						default: break;
					}
					break;
				}
				case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO: {
					switch (code) {
						case SDL_CONTROLLER_BUTTON_A: return "B";
						case SDL_CONTROLLER_BUTTON_B: return "A";
						case SDL_CONTROLLER_BUTTON_X: return "Y";
						case SDL_CONTROLLER_BUTTON_Y: return "X";
						case SDL_CONTROLLER_BUTTON_BACK: return "-";
						case SDL_CONTROLLER_BUTTON_START: return "+";
						case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "Left Stick";
						case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "Right Stick";
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "L";
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R";
						case JOYSTICK_CODE_TRIGGER_LEFT: return "ZL";
						case JOYSTICK_CODE_TRIGGER_RIGHT: return "ZR";
						default: break;
					}
					break;
				}
				default: {
					break;
				}
			}
		}
		return "?";
	}
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
			if (code >= 0 and code < SDL_CONTROLLER_BUTTON_MAX) {
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
			if (drv_->device = SDL_GameControllerOpen(0); !drv_->device) {
				spdlog::warn("Joystick couldn't open at startup! SDL Error: {}", SDL_GetError());
			}
		}

		return true;
	}

	void drop_() {
		if (drv_) {
			if (drv_->device) {
				SDL_GameControllerClose(drv_->device);
				drv_->device = nullptr;
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
			case SDL_CONTROLLERAXISMOTION: {
				if (event.caxis.which == 0) {
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
						if (event.caxis.value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding.right;
							bts.pressed.right = !holding;
							bts.holding.right = true;
							bts.holding.left = false;
						} else if (event.caxis.value < -AXIS_DEAD_ZONE) {
							bool holding = bts.holding.left;
							bts.pressed.left = !holding;
							bts.holding.left = true;
							bts.holding.right = false;
						} else {
							bts.holding.right = false;
							bts.holding.left = false;
						}
					} else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
						if (event.caxis.value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding.down;
							bts.pressed.down = !holding;
							bts.holding.down = true;
							bts.holding.up = false;
						} else if (event.caxis.value < -AXIS_DEAD_ZONE) {
							bool holding = bts.holding.up;
							bts.pressed.up = !holding;
							bts.holding.up = true;
							bts.holding.down = false;
						} else {
							bts.holding.up = false;
							bts.holding.down = false;
						}
					} else if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
						if (event.caxis.value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding._trigger_left;
							bts.pressed._trigger_left = !holding;
							bts.holding._trigger_left = true;
							if (drv_->listening_for_joystick) {
								drv_->stored_code = JOYSTICK_CODE_TRIGGER_LEFT;
							}
						} else {
							bts.holding._trigger_left = false;
							bts.released._trigger_left = true;
						}
					} else if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
						if (event.caxis.value > AXIS_DEAD_ZONE) {
							bool holding = bts.holding._trigger_right;
							bts.pressed._trigger_right = !holding;
							bts.holding._trigger_right = true;
							if (drv_->listening_for_joystick) {
								drv_->stored_code = JOYSTICK_CODE_TRIGGER_RIGHT;
							}
						} else {
							bts.holding._trigger_right = false;
							bts.released._trigger_right = true;
						}
					}
				}
				break;
			}
			case SDL_CONTROLLERBUTTONDOWN: {
				if (event.cdevice.which == 0) {
					const auto code = as<i32>(event.cbutton.button);
					if (code >= SDL_CONTROLLER_BUTTON_DPAD_UP and code <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
						switch (code) {
							case SDL_CONTROLLER_BUTTON_DPAD_UP: {
								bool holding = bts.holding.up;
								bts.pressed.up = !holding;
								bts.holding.up = true;
								bts.holding.down = false;
								break;
							}
							case SDL_CONTROLLER_BUTTON_DPAD_DOWN: {
								bool holding = bts.holding.down;
								bts.pressed.down = !holding;
								bts.holding.down = true;
								bts.holding.up = false;
								break;
							}
							case SDL_CONTROLLER_BUTTON_DPAD_LEFT: {
								bool holding = bts.holding.left;
								bts.pressed.left = !holding;
								bts.holding.left = true;
								bts.holding.right = false;
								break;
							}
							default: {
								bool holding = bts.holding.right;
								bts.pressed.right = !holding;
								bts.holding.right = true;
								bts.holding.left = false;
								break;
							}
						}
					} else {
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
				}
				break;
			}
			case SDL_CONTROLLERBUTTONUP: {
				if (event.cbutton.which == 0) {
					const auto code = as<i32>(event.cbutton.button);
					if (code >= SDL_CONTROLLER_BUTTON_DPAD_UP and code <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
						switch (code) {
							case SDL_CONTROLLER_BUTTON_DPAD_UP: {
								bts.holding.up = false;
								bts.released.down = true;
								break;
							}
							case SDL_CONTROLLER_BUTTON_DPAD_DOWN: {
								bts.holding.down = false;
								bts.released.up = true;
								break;
							}
							case SDL_CONTROLLER_BUTTON_DPAD_LEFT: {
								bts.holding.left = false;
								bts.released.right = true;
								break;
							}
							default: {
								bts.holding.right = false;
								bts.released.left = true;
								break;
							}
						}
					} else if (auto iter = drv_->joystick_bindings.find(code); iter != drv_->joystick_bindings.end()) {
						const auto name = iter->second;
						bts.holding._raw.set(name, false);
						bts.released._raw.set(name, true);
					}
				}
				break;
			}
			case SDL_CONTROLLERDEVICEADDED: {
				if (drv_->listening_for_keyboard) {
					drv_->listening_for_keyboard = false;
					drv_->listening_for_joystick = true;
				}
				if (event.cdevice.which == 0 and !drv_->device) {
					if (drv_->device = SDL_GameControllerOpen(0); !drv_->device) {
						spdlog::warn("Couldn't open joystick device! SDL Error: {}", SDL_GetError());
					}
				}
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED: {
				if (drv_->listening_for_joystick) {
					drv_->listening_for_keyboard = true;
					drv_->listening_for_joystick = false;
				}
				if (event.cdevice.which == 0 and drv_->device) {
					SDL_GameControllerClose(drv_->device);
					drv_->device = nullptr;
				}
				break;
			}
			default: {
				break;
			}
		}
	}
	return aty != activity_type::quitting;
}

bool input::joystick_attached() {
	if (!drv_) {
		return false;
	}
	return drv_->device;
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
			if (auto str = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(code)); str) {
				return str;
			}
		}
	}
	return {};
}
