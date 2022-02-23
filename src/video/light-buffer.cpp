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
		konst::WINDOW_DIMENSIONS<r32>() * as<r32>(scaling)
	};
	const auto count = as<u32>(drawable_);
	if (ogl::direct_state_available()) {
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
	} else {
		glCheck(glBindBuffer(GL_UNIFORM_BUFFER, handle_));
		glCheck(glBufferSubData(
			GL_UNIFORM_BUFFER, 0,
			sizeof(glm::vec4),
			&value[0]
		));
		glCheck(glBufferSubData(
			GL_UNIFORM_BUFFER,
			sizeof(glm::vec4),
			sizeof(u32),
			&count
		));
		glCheck(glBufferSubData(
			GL_UNIFORM_BUFFER,
			sizeof(glm::vec4) * 2,
			MAXIMUM_LIGHTS * sizeof(light_param),
			staging_.get()
		));
	}
	drawable_ = 0;
	return true;
}

udx light_buffer::length() const {
	return (sizeof(glm::vec4) * 2) + (MAXIMUM_LIGHTS * sizeof(light_param));
}
