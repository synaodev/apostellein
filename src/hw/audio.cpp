#include <memory>
#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./audio.hpp"
#include "./vfs.hpp"
#include "../audio/openal.hpp"
#include "../audio/noise-buffer.hpp"
#include "../audio/speaker.hpp"
#include "../util/config-file.hpp"

namespace {
	constexpr udx MAXIMUM_SPEAKERS = 12;
}

// private
namespace audio {
	// types
	struct task_info {
		task_info() noexcept = default;
		task_info(const noise_buffer* _payload, udx _index) noexcept :
			payload{ _payload },
			index{ _index } {}

		const noise_buffer* payload {};
		udx index {};
	};
	// driver
	struct driver {
	public:
		config_file* config {};
		ALCdevice* device {};
		ALCcontext* context {};
		std::vector<speaker> speakers {};
		std::vector<task_info> tasks {};
		r32 volume {};
	};
	std::unique_ptr<driver> drv_ {};

	// functions
	bool init_(config_file& cfg) {
		// Create driver
		if (drv_) {
			spdlog::critical("Audio system already has active driver!");
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Get config
		drv_->config = &cfg;
		drv_->volume = glm::clamp(cfg.audio_volume(), 0.0f, 1.0f);

		// Create device & context
		if (drv_->device = alcOpenDevice(nullptr); !drv_->device) {
			spdlog::critical("Audio device creation failed!");
			return false;
		}
		if (drv_->context = alcCreateContext(drv_->device, nullptr); !drv_->context) {
			spdlog::critical("Audio context creation failed!");
			return false;
		}
		if (alcMakeContextCurrent(drv_->context) == 0) {
			spdlog::critical("Audio context cannot activate!");
			return false;
		}
		if (!oal::load_extensions(drv_->device)) {
			spdlog::critical("OpenAL extensions cannot load!");
			return false;
		}

		// Create speakers
		drv_->speakers.resize(MAXIMUM_SPEAKERS);
		for (auto&& speak : drv_->speakers) {
			speak.create();
			speak.volume(drv_->volume);
		}

		// Preallocate tasks
		drv_->tasks.reserve(MAXIMUM_SPEAKERS);
		return true;
	}

	void drop_() {
		if (drv_) {
			if (drv_->context) {
				// clear buffers before clearing sources & deleting context
				drv_->speakers.clear();
				vfs::clear_noises();

				alcDestroyContext(drv_->context);
				drv_->context = nullptr;
			}
			if (drv_->device) {
				alcCloseDevice(drv_->device);
				drv_->device = nullptr;
			}
			drv_.reset();
		}
	}

	guard::guard(config_file& cfg) {
		if (audio::init_(cfg)) {
			ready_ = true;
		} else {
			audio::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			audio::drop_();
		}
	}
}

// public
bool audio::active() {
	return drv_ != nullptr;
}

void audio::play(const entt::hashed_string& id) {
	if (!drv_) {
		return;
	}
	for (auto&& speak : drv_->speakers) {
		if (!speak.playing()) {
			const auto index = std::distance(&drv_->speakers[0], &speak);
			const task_info task { vfs::find_noise(id), as<udx>(index) };
			drv_->tasks.push_back(task);
			break;
		}
	}
}

void audio::play(const entt::hashed_string& id, udx index) {
	if (!drv_) {
		return;
	}
	if (index < drv_->speakers.size()) {
		const task_info task { vfs::find_noise(id), index };
		drv_->tasks.push_back(task);
	}
}

void audio::play(const std::string& id) {
	const entt::hashed_string entry { id.c_str() };
	audio::play(entry);
}

void audio::play(const std::string& id, udx index) {
	const entt::hashed_string entry { id.c_str() };
	audio::play(entry, index);
}

void audio::pause(udx index) {
	if (!drv_) {
		return;
	}
	if (index < drv_->speakers.size()) {
		drv_->speakers[index].pause();
	}
}

void audio::resume(udx index) {
	if (!drv_) {
		return;
	}
	if (index < drv_->speakers.size()) {
		auto& speak = drv_->speakers[index];
		if (speak.paused()) {
			speak.play();
		}
	}
}

void audio::volume(r32 value) {
	if (!drv_) {
		return;
	}
	value = glm::clamp(value, 0.0f, 1.0f);
	for (auto&& speak : drv_->speakers) {
		speak.volume(value);
	}
	drv_->config->audio_volume(value);
}

r32 audio::volume() {
	if (!drv_) {
		return 0.0f;
	}
	return drv_->speakers[0].volume();
}

void audio::flush() {
	if (!drv_) {
		return;
	}
	if (!drv_->tasks.empty()) {
		for (auto&& task : drv_->tasks) {
			auto& speak = drv_->speakers[task.index];
			if (speak.bind(task.payload)) {
				speak.play();
			}
		}
		drv_->tasks.clear();
	}
}
