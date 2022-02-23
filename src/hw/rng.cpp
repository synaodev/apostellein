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
		i64 seed {};
		std::mt19937 generator {};
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
		drv_->seed = std::chrono::high_resolution_clock::now()
			.time_since_epoch()
			.count();

		// Create RNG
		drv_->generator.seed(as<u32>(drv_->seed));

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
u32 rng::seed() {
	if (!drv_) {
		return 0;
	}
	return as<u32>(drv_->seed);
}

void rng::seed(u32 value) {
	if (!drv_) {
		return;
	}
	drv_->seed = as<i64>(value);
	drv_->generator.seed(as<u32>(drv_->seed));
}

i32 rng::next(i32 low, i32 high) {
	if (!drv_) {
		return low;
	}
	std::uniform_int_distribution<i32> distribution_ { low, high };
	return distribution_(drv_->generator);
}

r32 rng::next(r32 low, r32 high) {
	if (!drv_) {
		return low;
	}
	std::uniform_real_distribution<r32> distribution { low, high };
	return distribution(drv_->generator);
}
