#include <spdlog/spdlog.h>

#include "./matrix-buffer.hpp"
#include "./opengl.hpp"

namespace {
	constexpr udx MAXIMUM_MATRICES = 2;
}

udx matrix_buffer::maximum() {
	return MAXIMUM_MATRICES;
}

udx matrix_buffer::length() const {
	return sizeof(glm::mat4) * matrix_buffer::maximum();
}

bool matrix_buffer::projection(const glm::mat4& value) {
	if (!handle_) {
		spdlog::error("Cannot update matrix buffer projection! Reason: Invalid");
		return false;
	}
	glCheck(glNamedBufferSubData(
		handle_, 0,
		sizeof(glm::mat4),
		&value[0][0]
	));
	return this->viewport(value);
}

bool matrix_buffer::viewport(const glm::mat4& value) {
	if (!handle_) {
		spdlog::error("Cannot update matrix buffer viewport! Reason: Invalid");
		return false;
	}
	if (cached_ == value) {
		return true;
	}
	cached_ = value;
	glCheck(glNamedBufferSubData(
		handle_,
		sizeof(glm::mat4),
		sizeof(glm::mat4),
		&value[0][0]
	));
	return true;
}
