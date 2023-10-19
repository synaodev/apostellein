#include <memory>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <SDL2/SDL.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./video.hpp"
#include "./vfs.hpp"
#include "../util/config-file.hpp"
#include "../util/message-box.hpp"
#include "../video/opengl.hpp"
#include "../video/swap-chain.hpp"

namespace {
	constexpr char ICON_NAME[] = "icon";
	constexpr i32 MINIMUM_SCALING = 1;
	constexpr i32 MAXIMUM_SCALING = 8;
	constexpr i32 DEFAULT_FRAME_RATE = 60;
	constexpr i32 MINIMUM_FRAME_RATE = DEFAULT_FRAME_RATE / 2;
	constexpr i32 MINIMUM_REFRESH_RATE = DEFAULT_FRAME_RATE;
	constexpr i32 MAXIMUM_REFRESH_RATE = DEFAULT_FRAME_RATE * 4;
	constexpr u32 DEFAULT_WINDOW_FLAGS = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
	constexpr u32 HIGH_DPI_WINDOW_FLAGS = DEFAULT_WINDOW_FLAGS | SDL_WINDOW_ALLOW_HIGHDPI;

	constexpr i32 clamp_frame_rate_(i32 fps) {
		if (fps <= 0) { // Unlimited
			return 0;
		}
		if (fps <= MINIMUM_FRAME_RATE) { // 30 Hz
			return MINIMUM_FRAME_RATE;
		}
		if (fps <= DEFAULT_FRAME_RATE) { // 60 Hz
			return DEFAULT_FRAME_RATE;
		}
		if (fps <= (DEFAULT_FRAME_RATE * 2)) { // 120 Hz
			return DEFAULT_FRAME_RATE * 2;
		}
		if (fps <= ((DEFAULT_FRAME_RATE * 12) / 5)) { // 144 Hz
			return (DEFAULT_FRAME_RATE * 12) / 5;
		}
		if (fps <= (DEFAULT_FRAME_RATE * 4)) { // 240 Hz
			return DEFAULT_FRAME_RATE * 4;
		}
		return fps;
	}
}

// private
namespace video {
	// driver
	struct driver {
	public:
		config_file* config {};
		SDL_Window* window {};
		SDL_GLContext context {};
		bool vertical_sync {};
		bool adaptive_sync {};
		bool full_screen {};
		i32 scaling { MINIMUM_SCALING };
		bool high_dpi {};
		bool yield {};
		i32 refresh_rate { MINIMUM_REFRESH_RATE };
		i32 frame_rate { DEFAULT_FRAME_RATE };
		std::chrono::steady_clock::time_point time {};
	};
	std::unique_ptr<driver> drv_ {};
	// functions
	i64 calculate_yield_duration_() {
		if (drv_->frame_rate == 0) {
			return 0;
		}
		const auto fps = as<r64>(drv_->frame_rate);
		if (drv_->yield) {
			return konst::SECONDS_TO_NANOSECONDS(1.0 / fps);
		}
		const auto now = std::chrono::steady_clock::now();
		const auto result = konst::SECONDS_TO_NANOSECONDS(1.0 / fps) - (now - drv_->time).count();
		return glm::max(as<i64>(0), result);
	}

	glm::ivec2 calculate_actual_viewport_() {
		if (drv_->high_dpi) {
			glm::ivec2 result {};
			SDL_GL_GetDrawableSize(
				drv_->window,
				&result.x,
				&result.y
			);
			return result;
		}
		if (drv_->full_screen) {
			SDL_DisplayMode mode {};
			if (SDL_GetDesktopDisplayMode(0, &mode) >= 0) {
				const glm::ivec2 result {
					mode.w / konst::WINDOW_WIDTH<i32>(),
					mode.h / konst::WINDOW_HEIGHT<i32>()
				};
				if (result.x != result.y) {
					spdlog::warn("Full screen mode could be stretched!");
				}
				return konst::WINDOW_DIMENSIONS<i32>() * result.y;
			}
			spdlog::critical("Retrieving desktop display mode failed! SDL Error: {}", SDL_GetError());
		}
		return konst::WINDOW_DIMENSIONS<i32>() * drv_->scaling;
	}

	i32 calculate_refresh_rate_() {
		const auto index = SDL_GetWindowDisplayIndex(drv_->window);
		if (index < 0) {
			spdlog::critical("Retrieving window display index failed! SDL Error: {}", SDL_GetError());
			return MINIMUM_REFRESH_RATE;
		}
		SDL_DisplayMode mode {};
		if (SDL_GetDesktopDisplayMode(index, &mode) < 0) {
			spdlog::critical("Retrieving display mode failed! SDL Error: {}", SDL_GetError());
			return MINIMUM_REFRESH_RATE;
		}
		return mode.refresh_rate;
	}

	void high_resolution_sleep_(i64 nanoseconds) {
		if (nanoseconds <= 0) {
			return;
		}
#if defined(APOSTELLEIN_POSIX_COMPLIANT)
		timespec spec{};
		spec.tv_nsec = nanoseconds;
		spec.tv_sec = 0;
		while ((nanosleep(&spec, &spec) == -1) and (errno == EINTR)) {}
#else
		std::this_thread::sleep_for(std::chrono::nanoseconds{ nanoseconds });
#endif
	}

	bool init_(config_file& cfg) {
		// Create driver
		if (drv_) {
			spdlog::critical("Video system already has active driver!");
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Get config
		ogl::version = cfg.sandy_bridge() ?
			ogl::context_type::v31 :
			ogl::context_type::v46;
		drv_->config = &cfg;
		drv_->vertical_sync = cfg.vertical_sync();
		drv_->adaptive_sync = cfg.adaptive_sync();
		drv_->full_screen = cfg.full_screen();
		drv_->scaling = glm::clamp(
			cfg.scaling(),
			MINIMUM_SCALING,
			MAXIMUM_SCALING
		);
		drv_->high_dpi = cfg.high_dpi();
		drv_->yield = cfg.yield();
		{
			const auto fps = clamp_frame_rate_(cfg.frame_rate());
			drv_->frame_rate = fps;
			cfg.frame_rate(fps);
		}

		// Set OpenGL attributes before creating window
		if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
			spdlog::critical("Setting core profile failed! SDL Error: {}", SDL_GetError());
			return false;
		}
		if (ogl::debug_callback_available() and cfg.logging()) {
			if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) < 0) {
				spdlog::critical("Setting debug profile flags failed! SDL Error: {}", SDL_GetError());
				return false;
			}
		}
		if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0) < 0) {
			spdlog::critical("Setting depth size failed! SDL Error: {}", SDL_GetError());
			return false;
		}
		if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0) {
			spdlog::critical("Setting double-buffering failed! SDL Error: {}", SDL_GetError());
			return false;
		}

		// Create a window
		const u32 creation_flags = drv_->high_dpi ?
			HIGH_DPI_WINDOW_FLAGS :
			DEFAULT_WINDOW_FLAGS;

		if (drv_->window = SDL_CreateWindow(
			konst::APPLICATION,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			konst::WINDOW_WIDTH<i32>() * drv_->scaling,
			konst::WINDOW_HEIGHT<i32>() * drv_->scaling,
			creation_flags
		); !drv_->window) {
			spdlog::critical("Window creation failed! SDL Error: {}", SDL_GetError());
			return false;
		}
		if (drv_->full_screen and SDL_SetWindowFullscreen(drv_->window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
			drv_->full_screen = false;
			cfg.full_screen(false);
			spdlog::warn("Fullscreen after window creation failed! SDL Error: {}", SDL_GetError());
		}

		// Get refresh rate
		drv_->refresh_rate = glm::clamp(
			video::calculate_refresh_rate_(),
			MINIMUM_REFRESH_RATE,
			MAXIMUM_REFRESH_RATE
		);

		// Generate a valid OpenGL context
		while (1) {
			if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, ogl::major_version()) < 0) {
				spdlog::critical("Setting OpenGL major version failed! SDL Error: {}", SDL_GetError());
				return false;
			}
			if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, ogl::minor_version()) < 0) {
				spdlog::critical("Setting OpenGL minor version failed! SDL Error: {}", SDL_GetError());
				return false;
			}
			drv_->context = SDL_GL_CreateContext(drv_->window);
			if (drv_->context or ogl::version == ogl::context_type::none) {
				break;
			} else {
				--ogl::version;
			}
		}
		if (!drv_->context) {
			const std::string message = fmt::format(
				"At least OpenGL 3.1 is required! SDL Error: {}",
				SDL_GetError()
			);
			message_box::error(message);
			spdlog::critical(message);
			return false;
		}

		// Load OpenGL extensions with GLAD
		if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) != 0) {
			glm::ivec2 version {};
			if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &version[0]) < 0) {
				spdlog::critical("Getting OpenGL major version failed! SDL Error: {}", SDL_GetError());
				return false;
			}
			if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &version[1]) < 0) {
				spdlog::critical("Getting OpenGL minor version failed! SDL Error: {}", SDL_GetError());
				return false;
			}
			spdlog::info("OpenGL Version is {}.{}!", version[0], version[1]);
		} else {
			spdlog::critical("OpenGL extension loading failed!");
			return false;
		}

		// Add OpenGL debug callback if available
		if (ogl::debug_callback_available() and cfg.logging()) {
			auto logger = spdlog::rotating_logger_st(
				konst::GRAPHICS,
				vfs::log_path(konst::GRAPHICS),
				konst::MAXIMUM_BYTES,
				konst::MAXIMUM_SINKS,
				true
			);
			logger->set_pattern(konst::PATTERN);

			glCheck(glDebugMessageCallback(ogl::debug_callback, nullptr));
			glCheck(glEnable(GL_DEBUG_OUTPUT));
		}

		// Print vendor information for debugging purposes
		const byte* vendor = nullptr;
		glCheck(vendor = glGetString(GL_VENDOR));
		if (vendor) {
			spdlog::info("GPU vendor is {}!", reinterpret_cast<const char*>(vendor));
		}

		// Properly scale/position window
		if (!drv_->full_screen) {
			SDL_SetWindowSize(
				drv_->window,
				konst::WINDOW_WIDTH<i32>() * drv_->scaling,
				konst::WINDOW_HEIGHT<i32>() * drv_->scaling
			);
		}
		SDL_SetWindowPosition(
			drv_->window,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED
		);

		// Clear frame buffer
		swap_chain::viewport(video::calculate_actual_viewport_());
		swap_chain::clear(chroma::BASE());
		SDL_GL_SwapWindow(drv_->window);

		// Set swap interval
		if (drv_->vertical_sync) {
			if (drv_->adaptive_sync and SDL_GL_SetSwapInterval(-1) < 0) {
				spdlog::warn("Failed to activate adaptive sync after context creation! SDL Error: {}", SDL_GetError());
				drv_->adaptive_sync = false;
				cfg.adaptive_sync(false);
			} else if (SDL_GL_SetSwapInterval(1) < 0) {
				spdlog::warn("Failed to activate vertical sync after context creation! SDL Error: {}", SDL_GetError());
				drv_->vertical_sync = false;
				cfg.vertical_sync(false);
			}
		} else if (SDL_GL_SetSwapInterval(0) < 0) {
			spdlog::warn("Failed to deactivate vertical sync after context creation! SDL Error: {}", SDL_GetError());
		}

		// Load window icon image
		const std::string path = vfs::image_path(ICON_NAME);
		if (auto icon = vfs::buffer_image(path); icon.valid()) {
			auto& dimensions = icon.dimensions();
			if (auto surface = SDL_CreateRGBSurfaceWithFormatFrom(
				icon.pixels(),
				dimensions.x,
				dimensions.y,
				sizeof(u32) * 8, // bits per byte
				sizeof(u32) * dimensions.x,
				SDL_PIXELFORMAT_RGBA32
			); surface) {
				SDL_SetWindowIcon(drv_->window, surface);
				SDL_FreeSurface(surface);
				surface = nullptr;
			} else {
				spdlog::error("Icon surface creation failed! SDL Error: {}", SDL_GetError());
			}
		}

		return true;
	}

	void drop_() {
		if (drv_) {
			if (drv_->context) {
				// clear resources before deleting context
				vfs::clear_animations();
				vfs::clear_fonts();
				vfs::clear_materials();

				SDL_GL_DeleteContext(drv_->context);
				drv_->context = nullptr;
			}
			if (drv_->window) {
				SDL_DestroyWindow(drv_->window);
				drv_->window = nullptr;
			}
			drv_.reset();
		}
	}

	guard::guard(config_file& cfg) {
		if (video::init_(cfg)) {
			ready_ = true;
		} else {
			video::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			video::drop_();
		}
	}
}

// public
void video::show() {
	if (!drv_) {
		return;
	}
	SDL_ShowWindow(drv_->window);
}

void video::flush() {
	if (!drv_) {
		return;
	}
	SDL_GL_SwapWindow(drv_->window);
	if (!drv_->vertical_sync and drv_->frame_rate > 0) {
		// pre-calculate before tight loop
		const auto duration = video::calculate_yield_duration_();
		if (drv_->yield) {
			while (1) {
				const auto now = std::chrono::steady_clock::now();
				if ((now - drv_->time).count() < duration) {
					std::this_thread::yield();
				} else {
					break;
				}
			}
		} else {
			video::high_resolution_sleep_(duration);
		}
	}
	drv_->time = std::chrono::steady_clock::now();
}

void video::full_screen(bool value) {
	if (!drv_) {
		return;
	}
	if (drv_->full_screen != value) {
		if (SDL_SetWindowFullscreen(drv_->window, value ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0) {
			spdlog::error("Window mode change failed! SDL Error: {}", SDL_GetError());
		} else {
			drv_->full_screen = value;
			drv_->config->full_screen(value);
		}

		if (!drv_->full_screen) {
			SDL_SetWindowSize(
				drv_->window,
				konst::WINDOW_WIDTH<i32>() * drv_->scaling,
				konst::WINDOW_HEIGHT<i32>() * drv_->scaling
			);
			SDL_SetWindowPosition(
				drv_->window,
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED
			);
		}

		drv_->refresh_rate = glm::clamp(
			video::calculate_refresh_rate_(),
			MINIMUM_REFRESH_RATE,
			MAXIMUM_REFRESH_RATE
		);
		swap_chain::viewport(video::calculate_actual_viewport_());
	}
}

void video::scaling(i32 value) {
	if (!drv_) {
		return;
	}
	value = glm::clamp(value, MINIMUM_SCALING, MAXIMUM_SCALING);
	if (drv_->scaling != value) {
		drv_->scaling = value;
		drv_->config->scaling(value);
		if (!drv_->full_screen) {
			SDL_SetWindowSize(
				drv_->window,
				konst::WINDOW_WIDTH<i32>() * drv_->scaling,
				konst::WINDOW_HEIGHT<i32>() * drv_->scaling
			);
			SDL_SetWindowPosition(
				drv_->window,
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED
			);
			swap_chain::viewport(video::calculate_actual_viewport_());
		}
	}
}

void video::vertical_sync(bool value) {
	if (!drv_) {
		return;
	}
	if (drv_->vertical_sync != value) {
		// do the check first
		if (value) {
			if (drv_->adaptive_sync and SDL_GL_SetSwapInterval(-1) == 0) {
				drv_->vertical_sync = true;
				drv_->config->vertical_sync(true);
			} else if (SDL_GL_SetSwapInterval(1) < 0) {
				spdlog::error("Failed to activate vertical sync! SDL Error: {}", SDL_GetError());
			} else {
				drv_->vertical_sync = true;
				drv_->config->vertical_sync(true);
			}
		} else {
			if (SDL_GL_SetSwapInterval(0) < 0) {
				spdlog::error("Failed to deactivate vertical sync! SDL Error: {}", SDL_GetError());
			} else {
				drv_->vertical_sync = false;
				drv_->config->vertical_sync(false);
			}
		}

		// then reset the timer
		drv_->time = std::chrono::steady_clock::now();
	}
}

bool video::full_screen() {
	if (!drv_) {
		return false;
	}
	return drv_->full_screen;
}

i32 video::scaling() {
	if (!drv_) {
		return MINIMUM_SCALING;
	}
	return drv_->scaling;
}

bool video::vertical_sync() {
	if (!drv_) {
		return false;
	}
	return drv_->vertical_sync;
}

std::tuple<void*, void*> video::pointers() {
	if (!drv_) {
		return { nullptr, nullptr };
	}
	if (!drv_->window or !drv_->context) {
		spdlog::error("Video subsystem shouldn't have a null window or OpenGL context!");
		return { nullptr, nullptr };
	}
	return std::make_tuple(drv_->window, drv_->context);
}
