#include <spdlog/spdlog.h>

#include "./speaker-unit.hpp"
#include "./noise-buffer.hpp"
#include "./openal.hpp"

void speaker_unit::create() {
	this->destroy();
	if (!handle_) {
		alCheck(alGenSources(1, &handle_));
	}
}

void speaker_unit::destroy() {
	this->unbind();
	if (handle_ != 0) {
		alCheck(alDeleteSources(1, &handle_));
		handle_ = 0;
	}
}

bool speaker_unit::bind(const noise_buffer* noise) {
	if (!handle_) {
		return false;
	}
	if (!ready_ or current_ != noise) {
		this->stop();
		if (noise and noise->ready_) {
			ready_ = true;
			current_ = noise;
			alCheck(alSourcei(handle_, AL_BUFFER, noise->handle_));
		} else {
			ready_ = false;
		}
	}
	return ready_;
}

void speaker_unit::unbind() {
	if (current_) {
		this->stop();
		ready_ = false;
		current_ = nullptr;
		alCheck(alSourcei(handle_, AL_BUFFER, 0));
	}
}

void speaker_unit::volume(r32 value) {
	if (handle_ != 0) {
		alCheck(alSourcef(handle_, AL_GAIN, value));
	}
}

r32 speaker_unit::volume() const {
	r32 result = 0.0f;
	if (handle_) {
		alCheck(alGetSourcef(handle_, AL_GAIN, &result));
	}
	return result;
}

void speaker_unit::play() {
	if (ready_) {
		alCheck(alSourcePlay(handle_));
	}
}

bool speaker_unit::playing() const {
	return this->matches_state_(AL_PLAYING);
}

void speaker_unit::stop() {
	if (ready_) {
		alCheck(alSourceStop(handle_));
	}
}

bool speaker_unit::stopped() const {
	return this->matches_state_(AL_STOPPED);
}

void speaker_unit::pause() {
	if (ready_) {
		alCheck(alSourceStop(handle_));
	}
}

bool speaker_unit::paused() const {
	return this->matches_state_(AL_PAUSED);
}

bool speaker_unit::matches_state_(i32 name) const {
	if (handle_ != 0 and ready_) {
		i32 state = 0;
		alCheck(alGetSourcei(handle_, AL_SOURCE_STATE, &state));
		return state == name;
	}
	return false;
}
