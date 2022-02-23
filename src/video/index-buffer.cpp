#include <vector>
#include <limits>
#include <spdlog/spdlog.h>

#include "./index-buffer.hpp"
#include "./opengl.hpp"

namespace {
	std::vector<u16> generate_quad_indices_(udx length) {
		std::vector<u16> result {};
		auto iter = std::back_inserter(result);

		result.reserve(length);
		u16 index = 0;

		while (1) {
			*iter++ = index; if (result.size() >= length) break;
			*iter++ = index + 1; if (result.size() >= length) break;
			*iter++ = index + 2; if (result.size() >= length) break;
			*iter++ = index + 1; if (result.size() >= length) break;
			*iter++ = index + 2; if (result.size() >= length) break;
			*iter++ = index + 3; if (result.size() >= length) break;
			index += 4;
		}
		return result;
	}
}

bool index_buffer::quads(udx count) {
	if (count == 0) {
		spdlog::error("Cannot generate indices for quads! Reason: Count must be greater than zero");
		return false;
	}
	if (count > std::numeric_limits<u16>::max()) {
		spdlog::error(
			"Cannot generate indices for quads! Reason: Count must be less than or equal to {}",
			std::numeric_limits<u16>::max()
		);
		return false;
	}
	this->destroy();
	length_ = count;

	const std::vector<u16> indices = generate_quad_indices_(count);

	if (ogl::direct_state_available()) {
		glCheck(glCreateBuffers(1, &handle_));
		glCheck(glNamedBufferStorage(
			handle_, sizeof(u16) * indices.size(),
			indices.data(), 0
		));
	} else if (ogl::buffer_storage_available()) {
		glCheck(glGenBuffers(1, &handle_));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_));
		glCheck(glBufferStorage(
			GL_ELEMENT_ARRAY_BUFFER,
			sizeof(u16) * indices.size(),
			indices.data(), 0
		));
	} else {
		glCheck(glGenBuffers(1, &handle_));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_));
		glCheck(glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			sizeof(u16) * indices.size(),
			indices.data(),
			GL_STATIC_DRAW
		));
	}

	return true;
}

void index_buffer::destroy() {
	if (handle_ != 0) {
		glCheck(glDeleteBuffers(1, &handle_));
		handle_ = 0;
	}
	length_ = 0;
}
