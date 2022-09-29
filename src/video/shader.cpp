#include <vector>
#include <glm/common.hpp>
#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./shader.hpp"
#include "./opengl.hpp"
#include "./const-buffer.hpp"
#include "./frame-buffer.hpp"

namespace {
	constexpr i32 INVALID_POSITION = as<i32>(GL_INVALID_INDEX);
	constexpr udx TEMPORARY_LENGTH = 512;
}

bool shader_object::create(const std::string& source, shader_stage stage) {
	this->destroy();

	const auto type = [stage] {
		switch (stage) {
		case shader_stage::vertex: return GL_VERTEX_SHADER;
		case shader_stage::pixel: return GL_FRAGMENT_SHADER;
		default: return GL_COMPUTE_SHADER;
		}
	}();
	const auto view = source.c_str();

	glCheck(handle_ = glCreateShader(type));
	glCheck(glShaderSource(handle_, 1, &view, NULL));
	glCheck(glCompileShader(handle_));

	i32 status = 0;
	glCheck(glGetShaderiv(handle_, GL_COMPILE_STATUS, &status));
	if (!status) {
		i32 length = 0;
		glCheck(glGetShaderiv(
			handle_,
			GL_INFO_LOG_LENGTH,
			&length
		));
		std::string message {};
		message.resize(as<udx>(length));
		glCheck(glGetShaderInfoLog(
			handle_,
			as<i32>(message.size()),
			nullptr,
			message.data()
		));
		message.resize(as<udx>(
			glm::max(0, length - 2)
		));
		spdlog::error("Failed to compile shader object! OpenGL Error: {}", message);
		spdlog::info("Source for reference:\n{}", source);
		this->destroy();
		return false;
	}
	stage_ = stage;
	return true;
}

void shader_object::destroy() {
	if (handle_ != 0) {
		glCheck(glDeleteShader(handle_));
		handle_ = 0;
		stage_ = shader_stage::vertex;
	}
}

u32 shader_program::current_ = 0;

bool shader_program::create(const shader_object& obj) {
	if (!obj.handle_ or obj.stage_ == shader_stage::compute) {
		spdlog::error("Failed to create shader program! Reason: Compute shader object invalid");
		return false;
	}

	this->destroy();
	glCheck(handle_ = glCreateProgram());
	glCheck(glAttachShader(handle_, obj.handle_));
	glCheck(glLinkProgram(handle_));

	i32 status = 0;
	glCheck(glGetProgramiv(handle_, GL_LINK_STATUS, &status));
	if (!status) {
		i32 length = 0;
		glCheck(glGetProgramiv(
			handle_,
			GL_INFO_LOG_LENGTH,
			&length
		));
		std::string message {};
		message.resize(as<udx>(length));
		glCheck(glGetProgramInfoLog(
			handle_,
			as<i32>(message.size()),
			nullptr,
			message.data()
		));
		message.resize(as<udx>(
			glm::max(0, length - 2)
		));
		spdlog::error("Failed to link shader program! OpenGL Error: {}", message);
		this->destroy();
		return false;
	}
	glCheck(glDetachShader(handle_, obj.handle_));
	return true;
}

bool shader_program::create(const shader_object& vtx, const shader_object& pix) {
	if (!vtx.handle_ or vtx.stage_ != shader_stage::vertex) {
		spdlog::error("Failed to create shader program! Reason: Vertex shader object invalid");
		return false;
	}
	if (!pix.handle_ or pix.stage_ != shader_stage::pixel) {
		spdlog::error("Failed to create shader program! Reason: Pixel shader object invalid");
		return false;
	}

	this->destroy();
	glCheck(handle_ = glCreateProgram());
	glCheck(glAttachShader(handle_, vtx.handle_));
	glCheck(glAttachShader(handle_, pix.handle_));
	glCheck(glLinkProgram(handle_));

	i32 status = 0;
	glCheck(glGetProgramiv(handle_, GL_LINK_STATUS, &status));
	if (!status) {
		i32 length = 0;
		std::string output {};
		glCheck(glGetProgramiv(
			handle_,
			GL_INFO_LOG_LENGTH,
			&length
		));
		output.resize(as<udx>(length));
		glCheck(glGetProgramInfoLog(
			handle_,
			length,
			nullptr,
			output.data()
		));
		spdlog::error("Failed to link shader program! OpenGL Error: {}", output);
		this->destroy();
		return false;
	}
	glCheck(glDetachShader(handle_, vtx.handle_));
	glCheck(glDetachShader(handle_, pix.handle_));

	i32 attributes = 0;
	glCheck(glGetProgramiv(
		handle_,
		GL_ACTIVE_ATTRIBUTES,
		&attributes
	));

	std::vector<u32> types {};
	types.resize(as<udx>(attributes));
	std::string buffer {};
	buffer.resize(TEMPORARY_LENGTH);
	for (i32 idx = 0; idx < attributes; ++idx) {
		i32 length = 0;
		i32 size = 0;
		u32 type = 0;
		glCheck(glGetActiveAttrib(
			handle_,
			as<u32>(idx),
			as<i32>(buffer.size()),
			&length,
			&size,
			&type,
			buffer.data()
		));
		i32 position = INVALID_POSITION;
		glCheck(position = glGetAttribLocation(handle_, buffer.c_str()));
		if (position < as<i32>(types.size())) {
			types[as<udx>(position)] = type;
		}
	}

	format_ = vertex_format::from(types);
	if (format_.size == 0) {
		spdlog::error("Shader program input format is invalid!");
		this->destroy();
		return false;
	}
	return true;
}

void shader_program::destroy() {
	current_ = 0;
	if (handle_ != 0) {
		glCheck(glUseProgram(0));
		glCheck(glDeleteProgram(handle_));
		handle_ = 0;
	}
	format_ = {};
}

void shader_program::bind() const {
	if (current_ != handle_) {
		current_ = handle_;
		glCheck(glUseProgram(handle_));
	}
}

void shader_program::buffer(const std::string& name, const const_buffer& data) const {
	if (handle_ != 0) {
		u32 index = GL_INVALID_INDEX;
		glCheck(index = glGetUniformBlockIndex(handle_, name.c_str()));
		if (index != GL_INVALID_INDEX) {
			glCheck(glUniformBlockBinding(
				handle_,
				index,
				data.binding()
			));
		}
	} else {
		spdlog::error("Cannot set shader program buffer! Reason: Invalid");
	}
}

void shader_program::sampler(const std::string& name, const frame_buffer& data) const {
	this->sampler(name, data.binding());
}

void shader_program::sampler(const std::string& name, i32 value) const {
	if (handle_ != 0) {
		i32 position = INVALID_POSITION;
		glCheck(position = glGetUniformLocation(handle_, name.c_str()));
		if (position != INVALID_POSITION) {
			glCheck(glUseProgram(handle_));
			glCheck(glUniform1i(
				position,
				value
			));
		}
	} else {
		spdlog::error("Cannot set shader program sampler! Reason: Invalid");
	}
}
