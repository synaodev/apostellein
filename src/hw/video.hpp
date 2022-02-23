#pragma once

#include <tuple>
#include <apostellein/struct.hpp>

struct config_file;

namespace video {
	void show();
	void flush();
	void full_screen(bool value);
	void scaling(i32 value);
	void vertical_sync(bool value);
	bool full_screen();
	i32 scaling();
	bool vertical_sync();
	std::tuple<void*, void*> pointers();
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
