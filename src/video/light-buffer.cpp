#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./light-buffer.hpp"
#include "./opengl.hpp"

namespace {
	constexpr udx MAXIMUM_LIGHTS = 64;
}

udx light_buffer::maximum() {
	return MAXIMUM_LIGHTS;
}

bool light_buffer::flush(i32 scaling) {
	if (!handle_) {
		spdlog::error("Cannot flush light buffer! Reason: Invalid");
		return false;
	}
	if (drawable_ == 0) {
		return true;
	}
	const glm::vec4 value {
		konst::WINDOW_DIMENSIONS<r32>(),
		konst::WINDOW_DIMENSIONS<r32>() * cast<r32>(scaling)
	};
	const auto count = cast<u32>(drawable_);
	glCheck(glNamedBufferSubData(
		handle_, 0,
		sizeof(glm::vec4),
		&value[0]
	));
	glCheck(glNamedBufferSubData(
		handle_,
		sizeof(glm::vec4),
		sizeof(u32),
		&count
	));
	glCheck(glNamedBufferSubData(
		handle_,
		sizeof(glm::vec4) * 2,
		MAXIMUM_LIGHTS * sizeof(light_param),
		staging_.get()
	));
	drawable_ = 0;
	return true;
}

udx light_buffer::length() const {
	return (sizeof(glm::vec4) * 2) + (MAXIMUM_LIGHTS * sizeof(light_param));
}
