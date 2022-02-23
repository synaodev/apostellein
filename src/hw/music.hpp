#pragma once

#include <string>
#include <apostellein/struct.hpp>

struct config_file;

namespace music {
	bool load(const std::string& title);
	bool play(r32 start_point, r32 fade_length);
	void pause();
	void fade(r32 fade_length);
	void resume(r32 fade_length);
	void clear();
	bool playing();
	void loop(bool value);
	bool looping();
	void volume(r32 value);
	r32 volume();
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
