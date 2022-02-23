#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <array>
#include <vector>
#include <limits>
#include <glm/common.hpp>
#include <spdlog/spdlog.h>
#include <pxtone/pxtnService.h>
#include <apostellein/cast.hpp>

#include "./music.hpp"
#include "./audio.hpp"
#include "./vfs.hpp"
#include "../audio/openal.hpp"
#include "../util/config-file.hpp"

namespace {
	constexpr r64 DELAY_FACTOR = 750.0;
	constexpr i32 MONO_CHANNEL = 1;
	constexpr i32 STEREO_CHANNELS = 2;
	constexpr i32 MAXIMUM_SAMPLING_RATE = 44100;
	constexpr r64 MINIMUM_BUFFERING_TIME = 0.05;
	constexpr r64 MAXIMUM_BUFFERING_TIME = 1.0;
	constexpr i32 MAXIMUM_FILE_SIZE = 10485760;

	template<typename T>
	constexpr T calculate_buffer_length_(
		r64 buffering_time,
		i32 channels,
		i32 sampling_rate
	) {
		static_assert(std::is_integral<T>::value);
		const auto result =
			as<T>(channels) *
			as<T>(sampling_rate) *
			as<T>(pxtnBITPERSAMPLE / 8);
		return as<T>(as<r64>(result) * buffering_time);
	}
}

// private
namespace music {
	// driver
	struct driver {
	public:
		config_file* config {};
		pxtnService service {};
		std::thread thread {};
		std::string title {};
		std::atomic<bool> playing {};
		std::atomic<bool> looping { true };
		std::atomic<r32> fade_length {};
		std::atomic<r32> volume {};
		i32 channels {};
		i32 sampling_rate {};
		r64 buffering_time {};
		u32 source {};
		std::array<u32, 3> buffers {};
	};
	std::unique_ptr<driver> drv_ {};

	// functions
	bool init_(config_file& cfg) {
		// Check if audio system is active
		if (!audio::active()) {
			spdlog::warn("Music system can't function if audio system is down!");
			return false;
		}

		// Create driver
		if (drv_) {
			spdlog::critical("Music system already has active driver!");
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Get config
		drv_->config = &cfg;
		drv_->volume = glm::clamp(cfg.music_volume(), 0.0f, 1.0f);
		drv_->channels = glm::clamp(
			cfg.channels(),
			MONO_CHANNEL,
			STEREO_CHANNELS
		);
		if (auto temp = cfg.sampling_rate();
			temp == MAXIMUM_SAMPLING_RATE / 4 or
			temp == MAXIMUM_SAMPLING_RATE / 2 or
			temp == MAXIMUM_SAMPLING_RATE
		) {
			drv_->sampling_rate = temp;
		} else {
			spdlog::warn("Music sampling rate cannot be {}!", temp);
			drv_->sampling_rate = MAXIMUM_SAMPLING_RATE;
		}
		drv_->buffering_time = glm::clamp(
			cfg.buffering_time(),
			MINIMUM_BUFFERING_TIME,
			MAXIMUM_BUFFERING_TIME
		);

		// Initialize pxtone service
		if (auto result = drv_->service.init(); result != pxtnERR::pxtnOK) {
			spdlog::critical("Pxtone service initialization failed! Error: {}", pxtnError_get_string(result));
			return false;
		}
		if (!drv_->service.set_destination_quality(drv_->channels, drv_->sampling_rate)) {
			spdlog::critical("Pxtone quality setting failed!");
			return false;
		}

		// Create buffers & source
		drv_->buffers.fill(0);
		const auto size = as<i32>(drv_->buffers.size());
		alCheck(alGenSources(1, &drv_->source));
		alCheck(alGenBuffers(size, drv_->buffers.data()));

		return true;
	}

	void drop_() {
		if (drv_) {
			music::clear();
			if (drv_->source != 0) {
				i32 state = 0;
				alCheck(alGetSourcei(drv_->source, AL_SOURCE_STATE, &state));
				if (state == AL_PLAYING) {
					alCheck(alSourceStop(drv_->source));
				}
				const auto size = as<i32>(drv_->buffers.size());
				alCheck(alSourcei(drv_->source, AL_BUFFER, 0));
				alCheck(alDeleteBuffers(size, drv_->buffers.data()));
				alCheck(alDeleteSources(1, &drv_->source));
				drv_->buffers.fill(0);
				drv_->source = 0;
			}
			if (drv_->service.master) {
				drv_->service.clear();
			}
			drv_.reset();
		}
	}

	void process_() {
		// Initialize constants
		const auto length = calculate_buffer_length_<i32>(
			drv_->buffering_time,
			drv_->channels,
			drv_->sampling_rate
		);
		const auto format = drv_->channels == STEREO_CHANNELS ?
			AL_FORMAT_STEREO16 :
			AL_FORMAT_MONO16;
		const std::chrono::milliseconds buffering_delay {
			as<i64>(drv_->buffering_time * DELAY_FACTOR)
		};

		// Initialize stream data
		bool looping = drv_->looping;
		r32 volume = drv_->volume;
		i32 state = 0;
		i32 processed = 0;
		auto pointer = std::make_unique<char[]>(as<udx>(length));

		// Queue tune beginning
		for (auto&& buffer : drv_->buffers) {
			if (drv_->service.Moo(pointer.get(), length)) {
				alCheck(alBufferData(
					buffer,
					format,
					pointer.get(),
					length,
					drv_->sampling_rate
				));
				alCheck(alSourceQueueBuffers(drv_->source, 1, &buffer));
			} else {
				drv_->playing = false;
			}
		}

		// Main loop
		while (drv_->playing) {
			// Check if looping has changed
			if (drv_->looping != looping) {
				looping = drv_->looping;
				drv_->service.moo_set_loop(looping);
			}
			// Check if volume has changed
			if (drv_->volume != volume) {
				volume = glm::clamp(drv_->volume.load(), 0.0f, 1.0f);
				drv_->service.moo_set_master_volume(drv_->volume);
			}
			// Check if fade out has started
			if (drv_->fade_length != 0.0f) {
				drv_->service.moo_set_fade(-1, drv_->fade_length);
				drv_->fade_length = 0.0f;
			}

			// Play sound & process buffers
			alCheck(alGetSourcei(drv_->source, AL_SOURCE_STATE, &state));
			if (state != AL_PLAYING) {
				alCheck(alSourcePlay(drv_->source));
			}
			alCheck(alGetSourcei(drv_->source, AL_BUFFERS_PROCESSED, &processed));
			while (processed > 0) {
				u32 buffer = 0;
				alCheck(alSourceUnqueueBuffers(drv_->source, 1, &buffer));
				if (drv_->service.Moo(pointer.get(), length)) {
					alCheck(alBufferData(
						buffer,
						format,
						pointer.get(),
						length,
						drv_->sampling_rate
					));
					alCheck(alSourceQueueBuffers(drv_->source, 1, &buffer));
					processed--;
				} else {
					drv_->playing = false;
					break;
				}
			}

			// Don't hog the CPU
			std::this_thread::sleep_for(buffering_delay);
		}

		// Clean Up
		alCheck(alGetSourcei(drv_->source, AL_SOURCE_STATE, &state));
		if (state == AL_PLAYING) {
			alCheck(alSourceStop(drv_->source));
		}
		alCheck(alSourcei(drv_->source, AL_BUFFER, 0));

		drv_->looping = true;
	}

	guard::guard(config_file& cfg) {
		if (music::init_(cfg)) {
			ready_ = true;
		} else {
			music::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			music::drop_();
		}
	}
}

// public
bool music::load(const std::string& title) {
	if (!drv_) {
		return false;
	}
	if (title == drv_->title) {
		return true;
	}
	music::clear();

	std::vector<char> file = vfs::buffer_chars(vfs::tune_path(title));
	if (file.empty()) {
		spdlog::error("Pxtone file loading failed!");
		return false;
	}

	const auto size = as<i32>(file.size());
	if (size >= MAXIMUM_FILE_SIZE) {
		spdlog::error("Pxtone file too large!");
		return false;
	}

	pxtnDescriptor descriptor {};
	if (!descriptor.set_memory_r(file.data(), size)) {
		spdlog::error("Pxtone descriptor creation failed!");
		return false;
	}

	if (auto result = drv_->service.read(&descriptor); result != pxtnERR::pxtnOK) {
		spdlog::error("Pxtone descriptor reading failed! Pxtone Error: {}", pxtnError_get_string(result));
		drv_->service.clear();
		return false;
	}

	if (auto result = drv_->service.tones_ready(); result != pxtnERR::pxtnOK) {
		spdlog::error("Pxtone tone readying failed! Pxtone Error: {}", pxtnError_get_string(result));
		drv_->service.clear();
		return false;
	}

	drv_->title = title;
	return true;
}

bool music::play(r32 start_point, r32 fade_length) {
	if (!drv_) {
		return false;
	}
	if (drv_->playing) {
		return false;
	}
	const auto sanity_check = calculate_buffer_length_<udx>(
		drv_->buffering_time,
		drv_->channels,
		drv_->sampling_rate
	);
	if (as<udx>(std::numeric_limits<i32>::max()) < sanity_check) {
		spdlog::error("Estimated pxtone buffer will overflow!");
		return false;
	}

	pxtnVOMITPREPARATION preparation {};
	if (drv_->looping) {
		preparation.flags |= pxtnVOMITPREPFLAG_loop;
	}
	preparation.start_pos_float = start_point / 1000.0f;
	preparation.fadein_sec = fade_length / 1000.0f;
	preparation.master_volume = drv_->volume;
	if (!drv_->service.moo_preparation(&preparation)) {
		spdlog::error("Pxtone couldn't prepare tune!");
	}
	drv_->playing = true;
	drv_->thread = std::thread(music::process_);
	return true;
}

void music::pause() {
	if (!drv_) {
		return;
	}
	if (drv_->playing) {
		drv_->playing = false;
	}
	if (drv_->thread.joinable()) {
		drv_->thread.join();
	}
}

void music::fade(r32 fade_length) {
	if (!drv_) {
		return;
	}
	if (drv_->playing) {
		drv_->fade_length = fade_length;
	}
}

void music::resume(r32 fade_length) {
	if (!drv_) {
		return;
	}
	if (!drv_->playing and drv_->service.moo_is_valid_data()) {
		music::play(0.0f, fade_length);
	}
}

void music::clear() {
	if (!drv_) {
		return;
	}
	music::pause();
	if (!drv_->title.empty()) {
		drv_->service.clear();
		drv_->title.clear();
	}
	drv_->looping = true;
}

bool music::playing() {
	if (!drv_) {
		return false;
	}
	return drv_->playing;
}

void music::loop(bool value) {
	if (!drv_) {
		return;
	}
	drv_->looping = value;
}

bool music::looping() {
	if (!drv_) {
		return false;
	}
	return drv_->looping;
}

void music::volume(r32 value) {
	if (!drv_) {
		return;
	}
	value = glm::clamp(value, 0.0f, 1.0f);
	drv_->volume = value;
	drv_->config->music_volume(value);
}

r32 music::volume() {
	if (!drv_) {
		return 0.0f;
	}
	return drv_->volume;
}
