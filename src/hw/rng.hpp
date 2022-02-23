#pragma once

#include <apostellein/struct.hpp>

namespace rng {
	u32 seed();
	void seed(u32 value);
	i32 next(i32 low, i32 high);
	r32 next(r32 low, r32 high);
	// Init-Guard
	struct guard : public not_moveable {
		guard();
		~guard();
	public:
		operator bool() const { return ready_; }
	private:
		bool ready_ {};
	};
}
