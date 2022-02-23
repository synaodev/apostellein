#include <spdlog/spdlog.h>
#include <SDL2/SDL_audio.h>

#include "./noise-buffer.hpp"
#include "./openal.hpp"
#include "./speaker.hpp"

namespace {
	i32 format_from_spec_(const SDL_AudioSpec* spec) {
		if (spec->channels == 1) {
			if (spec->format == AUDIO_U8 or spec->format == AUDIO_S8) {
				return AL_FORMAT_MONO8;
			} else {
				return AL_FORMAT_MONO16;
			}
		} else {
			if (spec->format == AUDIO_U8 or spec->format == AUDIO_S8) {
				return AL_FORMAT_STEREO8;
			} else {
				return AL_FORMAT_STEREO16;
			}
		}
	}
}

void noise_buffer::load(const std::string& path) {
	if (ready_) {
		spdlog::critical("Noise buffer was almost overwritten by {}!", path);
		return;
	}
	if (!handle_) {
		alCheck(alGenBuffers(1, &handle_));
	}
	byte* data = nullptr;
	u32 length = 0;
	SDL_AudioSpec spec {};
	if (!SDL_LoadWAV(path.c_str(), &spec, &data, &length)) {
		spdlog::error("Failed to load noise from \"{}\"! SDL Error: {}", path, SDL_GetError());
		return;
	}
	alCheck(alBufferData(
		handle_,
		format_from_spec_(&spec),
		data,
		length,
		spec.freq
	));
	SDL_FreeWAV(data);
	ready_ = true;
}

void noise_buffer::destroy() {
	ready_ = false;
	if (handle_ != 0) {
		// There should no longer be any bound speakers
		alCheck(alDeleteBuffers(1, &handle_));
		handle_ = 0;
	}
}
