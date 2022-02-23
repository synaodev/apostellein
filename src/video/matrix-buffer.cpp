#include <spdlog/spdlog.h>

#include "./matrix-buffer.hpp"
#include "./opengl.hpp"

bool matrix_buffer::projection(const glm::mat4& value) {
	if (!handle_) {
		spdlog::error("Cannot update matrix buffer projection! Reason: Invalid");
		return false;
	}
	if (ogl::direct_state_available()) {
		glCheck(glNamedBufferSubData(
			handle_, 0,
			sizeof(glm::mat4),
			&value[0][0]
		));
	} else {
		glCheck(glBindBuffer(GL_UNIFORM_BUFFER, handle_));
		glCheck(glBufferSubData(
			GL_UNIFORM_BUFFER, 0,
			sizeof(glm::mat4),
			&value[0][0]
		));
	}
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
	if (ogl::direct_state_available()) {
		glCheck(glNamedBufferSubData(
			handle_,
			sizeof(glm::mat4),
			sizeof(glm::mat4),
			&value[0][0]
		));
	} else {
		glCheck(glBindBuffer(GL_UNIFORM_BUFFER, handle_));
		glCheck(glBufferSubData(
			GL_UNIFORM_BUFFER,
			sizeof(glm::mat4),
			sizeof(glm::mat4),
			&value[0][0]
		));
	}
	return true;
}
