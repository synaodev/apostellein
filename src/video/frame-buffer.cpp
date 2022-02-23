#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./frame-buffer.hpp"
#include "./swap-chain.hpp"
#include "./opengl.hpp"

namespace {
	constexpr u32 DEFAULT_FORMAT = GL_RGBA4;
	constexpr i32 DEFAULT_MIPMAP = 1;
}

bool frame_buffer::create(const glm::ivec2& dimensions, i32 binding) {
	if (dimensions.x <= 0 or dimensions.y <= 0) {
		spdlog::error("Frame buffers cannot have non-positive dimensions!");
		return false;
	}
	if (binding < 1) {
		spdlog::error("Frame buffers cannot have a sampler binding lower than one!");
		return false;
	}
	this->destroy();
	dimensions_ = dimensions;
	binding_ = binding;

	const u32 attachment = GL_COLOR_ATTACHMENT0;
	bool result = false;
	if (ogl::direct_state_available()) {
		glCheck(glCreateFramebuffers(1, &handle_));
		glCheck(glCreateTextures(GL_TEXTURE_2D, 1, &buffer_));
		glCheck(glTextureStorage2D(
			buffer_,
			DEFAULT_MIPMAP,
			DEFAULT_FORMAT,
			dimensions.x,
			dimensions.y
		));
		glCheck(glTextureParameteri(buffer_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		glCheck(glTextureParameteri(buffer_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		glCheck(glTextureParameteri(buffer_, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		glCheck(glTextureParameteri(buffer_, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		glCheck(glBindTextureUnit(binding, buffer_));
		glCheck(glNamedFramebufferTexture(handle_, GL_COLOR_ATTACHMENT0, buffer_, 0));
		glCheck(glNamedFramebufferDrawBuffers(handle_, 1, &attachment));
		glCheck(result = (
			glCheckNamedFramebufferStatus(GL_FRAMEBUFFER, handle_) == GL_FRAMEBUFFER_COMPLETE
		));
	} else {
		glCheck(glGenFramebuffers(1, &handle_));
		glCheck(glBindFramebuffer(GL_FRAMEBUFFER, handle_));

		i32 previous = 0;
		glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous));
		glCheck(glGenTextures(1, &buffer_));
		glCheck(glActiveTexture(GL_TEXTURE0 + binding));
		glCheck(glBindTexture(GL_TEXTURE_2D, buffer_));
		if (ogl::texture_storage_available()) {
			glCheck(glTexStorage2D(
				GL_TEXTURE_2D,
				DEFAULT_MIPMAP,
				DEFAULT_FORMAT,
				dimensions.x,
				dimensions.y
			));
		} else {
			glCheck(glTexImage2D(
				GL_TEXTURE_2D, 0,
				DEFAULT_FORMAT,
				dimensions.x,
				dimensions.y,
				0, GL_RGBA, GL_UNSIGNED_BYTE,
				nullptr
			));
		}
		glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		glCheck(glBindTexture(GL_TEXTURE_2D, as<u32>(previous)));

		glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer_, 0));
		glCheck(glDrawBuffers(1, &attachment));
		glCheck(result = (
			glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE
		));
		glCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	if (!result) {
		spdlog::error("Frame buffer is not complete!");
		this->destroy();
	}
	return result;
}

void frame_buffer::destroy() {
	if (buffer_) {
		glCheck(glDeleteTextures(1, &buffer_));
		buffer_ = 0;
	}
	if (handle_) {
		glCheck(glDeleteFramebuffers(1, &handle_));
		handle_ = 0;
	}
	dimensions_ = {};
	binding_ = 0;
}

bool frame_buffer::blit(frame_buffer& target) const {
	if (!handle_ or !target.handle_) {
		spdlog::error("Cannot blit frame buffer! Reason: Invalid");
		return false;
	}
	if (ogl::direct_state_available()) {
		glCheck(glBlitNamedFramebuffer(
			handle_, target.handle_,
			0, 0, dimensions_.x, dimensions_.y,
			0, 0, target.dimensions_.x, target.dimensions_.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		));
	} else {
		glCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_));
		glCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.handle_));
		glCheck(glBlitFramebuffer(
			0, 0, dimensions_.x, dimensions_.y,
			0, 0, target.dimensions_.x, target.dimensions_.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		));
	}
	return true;
}

bool frame_buffer::blit() const {
	if (!handle_) {
		spdlog::error("Cannot blit frame buffer! Reason: Invalid");
		return false;
	}
	auto& viewport = swap_chain::viewport();
	if (ogl::direct_state_available()) {
		glCheck(glBlitNamedFramebuffer(
			handle_, 0,
			0, 0, dimensions_.x, dimensions_.y,
			0, 0, viewport.x, viewport.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		));
	} else {
		glCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_));
		glCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
		glCheck(glBlitFramebuffer(
			0, 0, dimensions_.x, dimensions_.y,
			0, 0, viewport.x, viewport.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		));
	}
	return true;
}

void frame_buffer::start_(const chroma& color) {
	const std::array values {
		as<r32>(color.r) / 255.0f,
		as<r32>(color.g) / 255.0f,
		as<r32>(color.b) / 255.0f,
		as<r32>(color.a) / 255.0f
	};
	glCheck(glBindFramebuffer(GL_FRAMEBUFFER, handle_));
	if (ogl::direct_state_available()) {
		glCheck(glClearNamedFramebufferfv(
			handle_,
			GL_COLOR, 0,
			values.data()
		));
	} else {
		glCheck(glClearBufferfv(GL_COLOR, 0, values.data()));
	}
	glCheck(glViewport(0, 0, dimensions_.x, dimensions_.y));
}

void frame_buffer::finish_() {
	glCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	auto& viewport = swap_chain::viewport();
	glCheck(glViewport(0, 0, viewport.x, viewport.y));
}
