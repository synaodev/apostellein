#include <spdlog/spdlog.h>

#include "./const-buffer.hpp"
#include "./opengl.hpp"

/*
 * glMapBuffer & glMapBufferRange are ridiculously
 * slow for streaming uniform buffer data, and
 * apparently this has been the state of affairs
 * since at least 2015, per the date of this blog post:
 * http://hacksoflife.blogspot.com/2015/06/glmapbuffer-no-longer-cool.html
*/

bool const_buffer::create(u32 binding) {
	if (binding == GL_INVALID_INDEX) {
		spdlog::error("Cannot create constant buffer! Reason: Invalid binding");
		return false;
	}
	this->destroy();
	if (ogl::direct_state_available()) {
		glCheck(glCreateBuffers(1, &handle_));
		glCheck(glNamedBufferStorage(
			handle_,
			this->length(),
			nullptr,
			GL_DYNAMIC_STORAGE_BIT
		));
	} else if (ogl::buffer_storage_available()) {
		glCheck(glGenBuffers(1, &handle_));
		glCheck(glBindBuffer(GL_UNIFORM_BUFFER, handle_));
		glCheck(glBufferStorage(
			GL_UNIFORM_BUFFER,
			this->length(),
			nullptr,
			GL_DYNAMIC_STORAGE_BIT
		));
	} else {
		glCheck(glGenBuffers(1, &handle_));
		glCheck(glBindBuffer(GL_UNIFORM_BUFFER, handle_));
		glCheck(glBufferData(
			GL_UNIFORM_BUFFER,
			this->length(),
			nullptr,
			GL_DYNAMIC_DRAW
		));
	}
	glCheck(glBindBufferBase(
		GL_UNIFORM_BUFFER,
		binding,
		handle_
	));
	binding_ = binding;
	return true;
}

void const_buffer::destroy() {
	if (handle_ != 0) {
		glCheck(glDeleteBuffers(1, &handle_));
		handle_ = 0;
	}
	binding_ = 0;
}
