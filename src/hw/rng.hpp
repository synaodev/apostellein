#pragma once

#include <apostellein/struct.hpp>

namespace rng {
	namespace effective {
		r32 between(r32 low, r32 high);
		i32 between(i32 low, i32 high);
	}
	namespace logical {
		bool chance(u32 divisor);
	}
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
