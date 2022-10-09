#include <SDL2/SDL_scancode.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./config-file.hpp"
#include "./buttons.hpp"

namespace {
	constexpr char VERSION_ENTRY[] = "@";
	constexpr char SETUP_ENTRY[] = "Setup";
	constexpr char DEBUGGER_ENTRY[] = "Debugger";
	constexpr char LOGGING_ENTRY[] = "Logging";
	constexpr char SANDY_BRIDGE_ENTRY[] = "SandyBridge";
	constexpr char LANGUAGE_ENTRY[] = "Language";
	constexpr char VIDEO_ENTRY[] = "Video";
	constexpr char VERTICAL_SYNC_ENTRY[] = "VerticalSync";
	constexpr char ADAPTIVE_SYNC_ENTRY[] = "AdaptiveSync";
	constexpr char FULL_SCREEN_ENTRY[] = "FullScreen";
	constexpr char SCALING_ENTRY[] = "Scaling";
	constexpr char HIGH_DPI_ENTRY[] = "HighDPI";
	constexpr char YIELD_ENTRY[] = "Yield";
	constexpr char FRAME_RATE_ENTRY[] = "FrameRate";
	constexpr char AUDIO_ENTRY[] = "Audio";
	constexpr char MUSIC_ENTRY[] = "Music";
	constexpr char VOLUME_ENTRY[] = "Volume";
	constexpr char CHANNELS_ENTRY[] = "Channels";
	constexpr char SAMPLING_RATE_ENTRY[] = "SamplingRate";
	constexpr char BUFFERING_TIME_ENTRY[] = "BufferingTime";
	constexpr char INPUT_ENTRY[] = "Input";
	constexpr char DEBUGGER_BINDING_ENTRY[] = "KeyDebugger";

	constexpr char DEFAULT_LANGUAGE[] = "english";
	constexpr i32 DEFAULT_SCALING = 2;
	constexpr i32 DEFAULT_FRAME_RATE = 60;
	constexpr r32 DEFAULT_AUDIO_VOLUME = 1.0f;
	constexpr r32 DEFAULT_MUSIC_VOLUME = 0.35f;
	constexpr i32 DEFAULT_CHANNELS = 2;
	constexpr i32 DEFAULT_SAMPLING_RATE = 44100;
	constexpr r64 DEFAULT_BUFFERING_TIME = 0.1;
}

bool config_file::load(nlohmann::json& data) {
	if (
		!data.empty() and
		data.contains(VERSION_ENTRY) and
		data[VERSION_ENTRY].is_string() and
		data[VERSION_ENTRY] == konst::IDENTIFIER
	) {
		data_ = std::move(data);
		return true;
	}
	return false;
}

void config_file::create() {
	data_.clear();

	data_[VERSION_ENTRY] = konst::IDENTIFIER;

	data_[AUDIO_ENTRY][VOLUME_ENTRY] = DEFAULT_AUDIO_VOLUME;

	data_[INPUT_ENTRY][DEBUGGER_BINDING_ENTRY] =
		static_cast<i32>(SDL_SCANCODE_DELETE);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::JUMP)] =
		static_cast<i32>(SDL_SCANCODE_Z);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::ARMS)] =
		static_cast<i32>(SDL_SCANCODE_X);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::ITEM)] =
		static_cast<i32>(SDL_SCANCODE_LSHIFT);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::STRAFE)] =
		static_cast<i32>(SDL_SCANCODE_SPACE);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::APOSTLE)] =
		static_cast<i32>(SDL_SCANCODE_LCTRL);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::INVENTORY)] =
		static_cast<i32>(SDL_SCANCODE_TAB);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::OPTIONS)] =
		static_cast<i32>(SDL_SCANCODE_ESCAPE);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::UP)] =
		static_cast<i32>(SDL_SCANCODE_UP);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::DOWN)] =
		static_cast<i32>(SDL_SCANCODE_DOWN);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::LEFT)] =
		static_cast<i32>(SDL_SCANCODE_LEFT);
	data_[INPUT_ENTRY][this->keyboard_binding_label(button_name::RIGHT)] =
		static_cast<i32>(SDL_SCANCODE_RIGHT);

	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::JUMP)] = 1;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::ARMS)] = 0;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::ITEM)] = 2;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::STRAFE)] = 3;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::APOSTLE)] = 4;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::INVENTORY)] = 20;
	data_[INPUT_ENTRY][this->joystick_binding_label(button_name::OPTIONS)] = 6;

	data_[MUSIC_ENTRY][BUFFERING_TIME_ENTRY] = DEFAULT_BUFFERING_TIME;
	data_[MUSIC_ENTRY][CHANNELS_ENTRY] = DEFAULT_CHANNELS;
	data_[MUSIC_ENTRY][SAMPLING_RATE_ENTRY] = DEFAULT_SAMPLING_RATE;
	data_[MUSIC_ENTRY][VOLUME_ENTRY] = DEFAULT_MUSIC_VOLUME;

	data_[SETUP_ENTRY][DEBUGGER_ENTRY] = konst::IMGUI;
	data_[SETUP_ENTRY][LOGGING_ENTRY] = konst::DEBUG;
	data_[SETUP_ENTRY][LANGUAGE_ENTRY] = DEFAULT_LANGUAGE;
	data_[SETUP_ENTRY][SANDY_BRIDGE_ENTRY] = false;

	data_[VIDEO_ENTRY][FRAME_RATE_ENTRY] = DEFAULT_FRAME_RATE;
	data_[VIDEO_ENTRY][FULL_SCREEN_ENTRY] = false;
	data_[VIDEO_ENTRY][HIGH_DPI_ENTRY] = false;
	data_[VIDEO_ENTRY][SCALING_ENTRY] = DEFAULT_SCALING;
	data_[VIDEO_ENTRY][VERTICAL_SYNC_ENTRY] = true;
	data_[VIDEO_ENTRY][ADAPTIVE_SYNC_ENTRY] = false;
	data_[VIDEO_ENTRY][YIELD_ENTRY] = false;
}

std::string config_file::dump() {
	if (this->valid()) {
		const std::string result = data_.dump(
			1, '\t', true,
			nlohmann::detail::error_handler_t::ignore
		);
		data_.clear();
		return result;
	}
	return {};
}

bool config_file::valid() const {
	return (
		data_.contains(VERSION_ENTRY) and
		data_[VERSION_ENTRY] == konst::IDENTIFIER
	);
}

bool config_file::debugger() const {
	if constexpr (konst::IMGUI) {
		if (
			data_.contains(SETUP_ENTRY) and
			data_[SETUP_ENTRY].contains(DEBUGGER_ENTRY) and
			data_[SETUP_ENTRY][DEBUGGER_ENTRY].is_boolean()
		) {
			return data_[SETUP_ENTRY][DEBUGGER_ENTRY].get<bool>();
		}
	}
	return false;
}

void config_file::debugger(bool value) {
	data_[SETUP_ENTRY][DEBUGGER_ENTRY] = value;
}

bool config_file::logging() const {
	if (
		data_.contains(SETUP_ENTRY) and
		data_[SETUP_ENTRY].contains(LOGGING_ENTRY) and
		data_[SETUP_ENTRY][LOGGING_ENTRY].is_boolean()
	) {
		return data_[SETUP_ENTRY][LOGGING_ENTRY].get<bool>();
	}
	return konst::DEBUG;
}

void config_file::logging(bool value) {
	data_[SETUP_ENTRY][LOGGING_ENTRY] = value;
}

bool config_file::sandy_bridge() const {
	if (
		data_.contains(SETUP_ENTRY) and
		data_[SETUP_ENTRY].contains(SANDY_BRIDGE_ENTRY) and
		data_[SETUP_ENTRY][SANDY_BRIDGE_ENTRY].is_boolean()
	) {
		return data_[SETUP_ENTRY][SANDY_BRIDGE_ENTRY].get<bool>();
	}
	return false;
}

void config_file::sandy_bridge(bool value) {
	data_[SETUP_ENTRY][SANDY_BRIDGE_ENTRY] = value;
}

std::string config_file::language() const {
	if (
		data_.contains(SETUP_ENTRY) and
		data_[SETUP_ENTRY].contains(LANGUAGE_ENTRY) and
		data_[SETUP_ENTRY][LANGUAGE_ENTRY].is_string()
	) {
		return data_[SETUP_ENTRY][LANGUAGE_ENTRY].get<std::string>();
	}
	return DEFAULT_LANGUAGE;
}

void config_file::language(const std::string& value) {
	data_[SETUP_ENTRY][LANGUAGE_ENTRY] = value;
}

bool config_file::vertical_sync() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(VERTICAL_SYNC_ENTRY) and
		data_[VIDEO_ENTRY][VERTICAL_SYNC_ENTRY].is_boolean()
	) {
		return data_[VIDEO_ENTRY][VERTICAL_SYNC_ENTRY].get<bool>();
	}
	return true;
}

void config_file::vertical_sync(bool value) {
	data_[VIDEO_ENTRY][VERTICAL_SYNC_ENTRY] = value;
}

bool config_file::adaptive_sync() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(ADAPTIVE_SYNC_ENTRY) and
		data_[VIDEO_ENTRY][ADAPTIVE_SYNC_ENTRY].is_boolean()
	) {
		return data_[VIDEO_ENTRY][ADAPTIVE_SYNC_ENTRY].get<bool>();
	}
	return true;
}

void config_file::adaptive_sync(bool value) {
	data_[VIDEO_ENTRY][ADAPTIVE_SYNC_ENTRY] = value;
}

bool config_file::full_screen() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(FULL_SCREEN_ENTRY) and
		data_[VIDEO_ENTRY][FULL_SCREEN_ENTRY].is_boolean()
	) {
		return data_[VIDEO_ENTRY][FULL_SCREEN_ENTRY].get<bool>();
	}
	return false;
}

void config_file::config_file::full_screen(bool value) {
	data_[VIDEO_ENTRY][FULL_SCREEN_ENTRY] = value;
}

i32 config_file::scaling() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(SCALING_ENTRY) and
		data_[VIDEO_ENTRY][SCALING_ENTRY].is_number_unsigned()
	) {
		return data_[VIDEO_ENTRY][SCALING_ENTRY].get<i32>();
	}
	return DEFAULT_SCALING;
}

void config_file::scaling(i32 value) {
	data_[VIDEO_ENTRY][SCALING_ENTRY] = value;
}

bool config_file::high_dpi() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(HIGH_DPI_ENTRY) and
		data_[VIDEO_ENTRY][HIGH_DPI_ENTRY].is_boolean()
	) {
		return data_[VIDEO_ENTRY][HIGH_DPI_ENTRY].get<bool>();
	}
	return false;
}

void config_file::high_dpi(bool value) {
	data_[VIDEO_ENTRY][HIGH_DPI_ENTRY] = value;
}

bool config_file::yield() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(YIELD_ENTRY) and
		data_[VIDEO_ENTRY][YIELD_ENTRY].is_boolean()
	) {
		return data_[VIDEO_ENTRY][YIELD_ENTRY].get<bool>();
	}
	return false;
}

void config_file::yield(bool value) {
	data_[VIDEO_ENTRY][YIELD_ENTRY] = value;
}

i32 config_file::frame_rate() const {
	if (
		data_.contains(VIDEO_ENTRY) and
		data_[VIDEO_ENTRY].contains(FRAME_RATE_ENTRY) and
		data_[VIDEO_ENTRY][FRAME_RATE_ENTRY].is_number_unsigned()
	) {
		return data_[VIDEO_ENTRY][FRAME_RATE_ENTRY].get<i32>();
	}
	return DEFAULT_FRAME_RATE;
}

void config_file::frame_rate(i32 value) {
	data_[VIDEO_ENTRY][FRAME_RATE_ENTRY] = value;
}

r32 config_file::audio_volume() const {
	if (
		data_.contains(AUDIO_ENTRY) and
		data_[AUDIO_ENTRY].contains(VOLUME_ENTRY) and
		data_[AUDIO_ENTRY][VOLUME_ENTRY].is_number_float()
	) {
		return data_[AUDIO_ENTRY][VOLUME_ENTRY].get<r32>();
	}
	return DEFAULT_AUDIO_VOLUME;
}

void config_file::audio_volume(r32 value) {
	data_[AUDIO_ENTRY][VOLUME_ENTRY] = value;
}

r32 config_file::music_volume() const {
	if (
		data_.contains(MUSIC_ENTRY) and
		data_[MUSIC_ENTRY].contains(VOLUME_ENTRY) and
		data_[MUSIC_ENTRY][VOLUME_ENTRY].is_number_float()
	) {
		return data_[MUSIC_ENTRY][VOLUME_ENTRY].get<r32>();
	}
	return DEFAULT_MUSIC_VOLUME;
}

void config_file::music_volume(r32 value) {
	data_[MUSIC_ENTRY][VOLUME_ENTRY] = value;
}

i32 config_file::channels() const {
	if (
		data_.contains(MUSIC_ENTRY) and
		data_[MUSIC_ENTRY].contains(CHANNELS_ENTRY) and
		data_[MUSIC_ENTRY][CHANNELS_ENTRY].is_number_unsigned()
	) {
		return data_[MUSIC_ENTRY][CHANNELS_ENTRY].get<i32>();
	}
	return DEFAULT_CHANNELS;
}

void config_file::channels(i32 value) {
	data_[MUSIC_ENTRY][CHANNELS_ENTRY] = value;
}

i32 config_file::sampling_rate() const {
	if (
		data_.contains(MUSIC_ENTRY) and
		data_[MUSIC_ENTRY].contains(SAMPLING_RATE_ENTRY) and
		data_[MUSIC_ENTRY][SAMPLING_RATE_ENTRY].is_number_unsigned()
	) {
		return data_[MUSIC_ENTRY][SAMPLING_RATE_ENTRY].get<i32>();
	}
	return DEFAULT_SAMPLING_RATE;
}

void config_file::sampling_rate(i32 value) {
	data_[MUSIC_ENTRY][SAMPLING_RATE_ENTRY] = value;
}

r64 config_file::buffering_time() const {
	if (
		data_.contains(MUSIC_ENTRY) and
		data_[MUSIC_ENTRY].contains(BUFFERING_TIME_ENTRY) and
		data_[MUSIC_ENTRY][BUFFERING_TIME_ENTRY].is_number_float()
	) {
		return data_[MUSIC_ENTRY][BUFFERING_TIME_ENTRY].get<r64>();
	}
	return DEFAULT_BUFFERING_TIME;
}

void config_file::buffering_time(r64 value) {
	data_[MUSIC_ENTRY][BUFFERING_TIME_ENTRY] = value;
}

i32 config_file::keyboard_binding(u32 name) const {
	const std::string label = config_file::keyboard_binding_label(name);
	if (
		!label.empty() and
		data_.contains(INPUT_ENTRY) and
		data_[INPUT_ENTRY].contains(label) and
		data_[INPUT_ENTRY][label].is_number_integer()
	) {
		return data_[INPUT_ENTRY][label].get<i32>();
	}
	return SDL_SCANCODE_UNKNOWN;
}

void config_file::keyboard_binding(u32 name, i32 code) {
	const std::string label = config_file::keyboard_binding_label(name);
	if (!label.empty()) {
		data_[INPUT_ENTRY][label] = code;
	}
}

i32 config_file::joystick_binding(u32 name) const {
	const std::string label = config_file::joystick_binding_label(name);
	if (
		!label.empty() and
		data_.contains(INPUT_ENTRY) and
		data_[INPUT_ENTRY].contains(label) and
		data_[INPUT_ENTRY][label].is_number_integer()
	) {
		return data_[INPUT_ENTRY][label].get<i32>();
	}
	return -1;
}

void config_file::joystick_binding(u32 name, i32 code) {
	const std::string label = config_file::joystick_binding_label(name);
	if (!label.empty()) {
		data_[INPUT_ENTRY][label] = code;
	}
}

i32 config_file::debugger_binding() const {
	if constexpr (konst::IMGUI) {
		if (
			data_.contains(INPUT_ENTRY) and
			data_[INPUT_ENTRY].contains(DEBUGGER_BINDING_ENTRY) and
			data_[INPUT_ENTRY][DEBUGGER_BINDING_ENTRY].is_number_integer()
		) {
			return data_[INPUT_ENTRY][DEBUGGER_BINDING_ENTRY].get<i32>();
		}
	}
	return SDL_SCANCODE_UNKNOWN;
}

void config_file::debugger_binding(i32 code) {
	data_[INPUT_ENTRY][DEBUGGER_BINDING_ENTRY] = code;
}

std::string config_file::keyboard_binding_label(u32 name) {
	switch (name) {
		case button_name::JUMP: return "KeyJump";
		case button_name::ARMS: return "KeyArms";
		case button_name::ITEM: return "KeyItem";
		case button_name::STRAFE: return "KeyStrafe";
		case button_name::APOSTLE: return "KeyApostle";
		case button_name::INVENTORY: return "KeyInventory";
		case button_name::OPTIONS: return "KeyOptions";
		case button_name::UP: return "KeyUp";
		case button_name::DOWN: return "KeyDown";
		case button_name::LEFT: return "KeyLeft";
		case button_name::RIGHT: return "KeyRight";
		default: return {};
	}
}

std::string config_file::joystick_binding_label(u32 name) {
	switch (name) {
		case button_name::JUMP: return "JoyJump";
		case button_name::ARMS: return "JoyArms";
		case button_name::ITEM: return "JoyItem";
		case button_name::STRAFE: return "JoyStrafe";
		case button_name::APOSTLE: return "JoyApostle";
		case button_name::INVENTORY: return "JoyInventory";
		case button_name::OPTIONS: return "JoyOptions";
		default: return {};
	}
}
