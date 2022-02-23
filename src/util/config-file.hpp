#pragma once

#include <nlohmann/json.hpp>
#include <apostellein/struct.hpp>

struct config_file : public not_copyable {
	config_file() noexcept = default;
	config_file(config_file&& that) noexcept {
		*this = std::move(that);
	}
	config_file& operator=(config_file&& that) noexcept {
		if (this != &that) {
			data_ = std::move(that.data_);
			that.data_ = {};
		}
		return *this;
	}
	~config_file() = default;
public:
	bool load(nlohmann::json& data);
	void create();
	std::string dump();
	bool valid() const;
	bool debugger() const;
	void debugger(bool value);
	bool logging() const;
	void logging(bool value);
	bool sandy_bridge() const;
	void sandy_bridge(bool value);
	std::string language() const;
	void language(const std::string& value);
	bool vertical_sync() const;
	void vertical_sync(bool value);
	bool adaptive_sync() const;
	void adaptive_sync(bool value);
	bool full_screen() const;
	void full_screen(bool value);
	i32 scaling() const;
	void scaling(i32 value);
	bool high_dpi() const;
	void high_dpi(bool value);
	bool yield() const;
	void yield(bool value);
	i32 frame_rate() const;
	void frame_rate(i32 value);
	r32 audio_volume() const;
	void audio_volume(r32 value);
	r32 music_volume() const;
	void music_volume(r32 value);
	i32 channels() const;
	void channels(i32 value);
	i32 sampling_rate() const;
	void sampling_rate(i32 value);
	r64 buffering_time() const;
	void buffering_time(r64 value);
	i32 keyboard_binding(u32 name) const;
	void keyboard_binding(u32 name, i32 code);
	i32 joystick_binding(u32 name) const;
	void joystick_binding(u32 name, i32 code);
	i32 debugger_binding() const;
	void debugger_binding(i32 code);
	static std::string keyboard_binding_label(u32 name);
	static std::string joystick_binding_label(u32 name);
private:
	nlohmann::json data_ {};
};
