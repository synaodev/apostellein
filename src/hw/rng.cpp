#include <memory>
#include <random>
#include <chrono>
#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./rng.hpp"

// private
namespace rng {
	// driver
	struct driver {
	public:
		std::mt19937 effective {};
		std::mt19937 logical {};
	};
	std::unique_ptr<driver> drv_ {};

	// functions
	bool init_() {
		// Create driver
		if (drv_) {
			spdlog::critical("RNG system already has active driver!");
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Set Seed
		const auto nanoseconds = std::chrono::high_resolution_clock::now()
			.time_since_epoch()
			.count();
		const auto seed = as<u32>(nanoseconds);

		// Create RNG
		drv_->effective.seed(seed);
		drv_->logical.seed(seed);

		return true;
	}

	void drop_() {
		if (drv_) {
			drv_.reset();
		}
	}

	guard::guard() {
		if (rng::init_()) {
			ready_ = true;
		} else {
			rng::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			rng::drop_();
		}
	}
}

// public

i32 rng::effective::between(i32 low, i32 high) {
	if (!drv_) {
		return low;
	}
	if (low >= high) {
		spdlog::warn("In i32 RNG, given \"low\" value is greater than or equal to high value!");
		return (low + high) / 2;
	}
	std::uniform_int_distribution<i32> distribution { low, high };
	return distribution(drv_->effective);
}

r32 rng::effective::between(r32 low, r32 high) {
	if (!drv_) {
		return low;
	}
	if (low >= high) {
		spdlog::warn("In r32 RNG, given \"low\" value is greater than or equal to high value!");
		return (low + high) / 2;
	}
	std::uniform_real_distribution<r32> distribution { low, high };
	return distribution(drv_->effective);
}

bool rng::logical::chance(u32 divisor) {
	if (!drv_ or divisor == 0) {
		return false;
	}
	if (divisor == 1) {
		return true;
	}
	std::uniform_int_distribution<u32> distribution { 1, divisor };
	const auto result = distribution(drv_->logical);
	return result == divisor;
}
