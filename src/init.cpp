#include <chrono>
#include <csignal>
#include <atomic>
#include <thread>
#include <spdlog/spdlog.h>
#include <SDL2/SDL.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./hw/audio.hpp"
#include "./hw/music.hpp"
#include "./hw/input.hpp"
#include "./hw/rng.hpp"
#include "./hw/vfs.hpp"
#include "./hw/video.hpp"
#include "./ctrl/runtime.hpp"
#include "./util/buttons.hpp"
#include "./util/message-box.hpp"
#include "./x2d/renderer.hpp"

namespace {
	using namespace std::chrono_literals;
	constexpr auto MINIMUM_SLEEP = 30ms;
	constexpr udx MAXIMUM_TICKS = 10;
}

namespace {
	std::atomic<bool> interrupt_ = false;
	void apostellein_interrupt_handler_(int) {
		interrupt_ = true;
	}
}

int main_loop(config_file& cfg) {
	// timers
	auto delta_time = [
		then = std::chrono::steady_clock::now(),
		now = std::chrono::steady_clock::now()
	]() mutable -> i64 {
		now = std::chrono::steady_clock::now();
		auto result = now - then;
		then = std::chrono::steady_clock::now();
		return result.count();
	};
	auto elapsed_time = [
		start = std::chrono::steady_clock::now()
	]() -> i64 {
		const auto now = std::chrono::steady_clock::now();
		return (now - start).count();
	};
	auto accumulate_ticks = [&elapsed_time](auto& current) {
		udx ticks = 0;
		while (elapsed_time() >= current and ticks < MAXIMUM_TICKS) {
			current += konst::NANOSECONDS_PER_TICK();
			++ticks;
		}
		return ticks;
	};
	// init input data
	activity_type aty {};
	buttons bts {};
	// init renderer
	renderer rdr {};
	if (!rdr.build()) {
		return EXIT_FAILURE;
	}
	// init runtime
	runtime state {};
	if (!state.build(cfg, rdr)) {
		return EXIT_FAILURE;
	}
	// init timers
	i64 previous = 0;
	i64 current = 0;
	// show window
	video::show();
	// enter loop
	while (input::poll(aty, bts)) {
		if (interrupt_) {
			spdlog::info("Recieved interrupt! Closing gracefully...");
			break;
		}
		switch (aty) {
			case activity_type::running: {
				// Handle
				const auto preserve = current;
				if (const auto ticks = accumulate_ticks(current); ticks == MAXIMUM_TICKS) {
					spdlog::warn("Long frame time just occurred: {}!", ticks);
					previous = elapsed_time();
					current = previous + konst::NANOSECONDS_PER_TICK();
				} else if (ticks > 0) {
					previous = preserve;
					state.handle(ticks, aty, bts);
					audio::flush();
				}
				// Update
				state.update(delta_time());
				// Render
				const auto elapsed = elapsed_time();
				{
					if (current > previous) {
						const auto ratio = (
							as<r32>(elapsed - previous) /
							as<r32>(current - previous)
						);
						state.render(ratio, rdr);
					} else {
						state.render(1.0f, rdr);
					}
					video::flush();
				}
				break;
			}
			case activity_type::stopped: {
				std::this_thread::sleep_for(MINIMUM_SLEEP);
				break;
			}
			case activity_type::quitting: {
				break;
			}
		}
	}
	return EXIT_SUCCESS;
}

struct sdl2_guard : public not_moveable {
public:
	sdl2_guard() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
			const std::string message = fmt::format(
				"SDL Initialization failed! SDL Error: {}",
				SDL_GetError()
			);
			message_box::error(message);
		} else {
			ready_ = true;
		}
	}
	~sdl2_guard() { SDL_Quit(); }
	operator bool() const { return ready_; }
private:
	bool ready_ { false };
};

int init(int argc, char** argv) {
	// Handle arguments
	const std::string root = argc > 1 ?
		argv[1] :
		std::string{};

	// Hardware
	sdl2_guard sg {};
	if (!sg) return EXIT_FAILURE;
	vfs::guard hg { root };
	if (!hg) return EXIT_FAILURE;

	// Print info
	spdlog::info("Version: {}", konst::VERSION);
	spdlog::info("Platform: {} {}", konst::ARCHITECTURE, konst::PLATFORM);
	spdlog::info("Compiler: {}", konst::COMPILER);
	spdlog::info("Toolchain: {}", konst::TOOLCHAIN);
	spdlog::info("Build Type: {}", konst::BUILD_TYPE);

	// input, video, audio, music, rng
	auto& config = hg.config();
	input::guard ig { config };
	if (!ig) return EXIT_FAILURE;
	video::guard vg { config };
	if (!vg) return EXIT_FAILURE;
	audio::guard ag { config };
	if (!ag) return EXIT_FAILURE;
	music::guard mg { config };
	if (!mg) return EXIT_FAILURE;
	rng::guard rg {};
	if (!rg) return EXIT_FAILURE;

	// Main loop
	return main_loop(config);
}

int main(int argc, char** argv) {
	// Register signal handlers
	std::signal(SIGINT, apostellein_interrupt_handler_);

	// Ideally exceptions are never thrown at all,
	// but disabling them has gnarly side-effects.
	// Given how RAII is used in this project,
	// it's worth prioritizing exception safety.
	try {
		return init(argc, argv);
	} catch (const std::exception& exception) {
		message_box::error(exception.what());
	}
	return EXIT_FAILURE;
}
