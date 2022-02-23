#pragma once

#include <string>
#include <entt/core/hashed_string.hpp>
#include <apostellein/struct.hpp>

struct config_file;

namespace audio {
	bool active();
	void play(const entt::hashed_string& id);
	void play(const entt::hashed_string& id, udx index);
	void play(const std::string& id);
	void play(const std::string& id, udx index);
	void pause(udx index);
	void resume(udx index);
	void volume(r32 value);
	r32 volume();
	void flush();
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
