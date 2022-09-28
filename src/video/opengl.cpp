#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./opengl.hpp"
#include "./shader.hpp"

namespace ogl {
	context_type version { context_type::none };
}

u32 ogl::major_version() noexcept {
	switch (version) {
	case ogl::context_type::none: return 0;
	case ogl::context_type::v31:
	case ogl::context_type::v32:
	case ogl::context_type::v33: return 3;
	default: return 4;
	}
}

u32 ogl::minor_version() noexcept {
	switch (version) {
	case ogl::context_type::v31:
	case ogl::context_type::v41: return 1;
	case ogl::context_type::v32:
	case ogl::context_type::v42: return 2;
	case ogl::context_type::v33:
	case ogl::context_type::v43: return 3;
	case ogl::context_type::v44: return 4;
	case ogl::context_type::v45: return 5;
	case ogl::context_type::v46: return 6;
	default: return 0;
	}
}

/*
 * OpenGL 3.1 should actually map to GLSL version 140, but the
 * entire reason we even allow 3.1 contexts is to ensure that
 * Intel's shitty Sandy Bridge GPUs can run without incident.
 * Given that Sandy Bridge GPUs support GL_ARB_shader_bit_encoding,
 * it's reasonable to assume that shader compilation won't fail
 * even if we return the "wrong" GLSL version.
 */

u32 ogl::glsl_version() noexcept {
	switch (version) {
	case ogl::context_type::v31: /* return 140; */
	case ogl::context_type::v32: return 150;
	case ogl::context_type::v33: return 330;
	case ogl::context_type::v40: return 400;
	case ogl::context_type::v41: return 410;
	case ogl::context_type::v42: return 420;
	case ogl::context_type::v43: return 430;
	case ogl::context_type::v44: return 440;
	case ogl::context_type::v45: return 450;
	case ogl::context_type::v46: return 460;
	default: return 0;
	}
}

u32 ogl::memory_available() noexcept {
	i32 value = 0;
	glCheck(glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value));
	return static_cast<u32>(value);
}

bool ogl::modern_shaders_available() noexcept {
	return ogl::version >= ogl::context_type::v33;
}

bool ogl::binding_points_available() noexcept {
	return ogl::version >= ogl::context_type::v42;
}

bool ogl::debug_callback_available() noexcept {
	return ogl::version >= ogl::context_type::v43;
}

bool ogl::buffer_storage_available() noexcept {
	return ogl::version >= ogl::context_type::v44;
}

bool ogl::direct_state_available() noexcept {
	return ogl::version >= ogl::context_type::v45;
}

bool ogl::texture_storage_available() noexcept {
	return ogl::binding_points_available();
}

void ogl::check_errors(const char* path, u32 line, const char* expr) {
	if (const auto code = glGetError(); code != GL_NO_ERROR) {
		const char* error = "Unknown OpenGL error";
		const char* details = "Description unavailable.";
		switch (code) {
		case GL_INVALID_ENUM:
			error = "GL_INVALID_ENUM";
			details = "An unacceptable value has been specified for an enumerated argument.";
			break;
		case GL_INVALID_VALUE:
			error = "GL_INVALID_VALUE";
			details = "A numeric argument is out of range.";
			break;
		case GL_INVALID_OPERATION:
			error = "GL_INVALID_OPERATION";
			details = "The specified operation is not allowed in the current state.";
			break;
		case GL_OUT_OF_MEMORY:
			error = "GL_OUT_OF_MEMORY";
			details = "There is not enough memory left to execute the command.";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "GL_INVALID_FRAMEBUFFER_OPERATION";
			details = "The object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\".";
			break;
		default:
			break;
		}
		const std::string_view file { path };
		spdlog::critical(
			"An internal OpenGL call failed at {} ({})! Expression: \"{}\", Error: {}, Details: {}",
			file.substr(file.find_last_of("\\/") + 1),
			line, expr,
			error, details
		);
	}
}

void APIENTRY ogl::debug_callback(
	GLenum source,
	GLenum type,
	GLuint /* id */,
	GLenum severity,
	GLsizei /* length */,
	const GLchar* message,
	const void* /* param */
) noexcept {
	const auto source_name = [source] {
		switch (source) {
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window System";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third Party";
		default: return "Other";
		}
	}();
	const auto type_name = [type] {
		switch (type) {
		case GL_DEBUG_TYPE_ERROR: return "Error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined Behavior";
		case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
		case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
		case GL_DEBUG_TYPE_MARKER: return "Marker";
		case GL_DEBUG_TYPE_PUSH_GROUP: return "Push Group";
		case GL_DEBUG_TYPE_POP_GROUP: return "Pop Group";
		default: return "Other";
		}
	}();
	const auto level = [severity] {
		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH: return spdlog::level::critical;
		case GL_DEBUG_SEVERITY_MEDIUM: return spdlog::level::err;
		case GL_DEBUG_SEVERITY_LOW: return spdlog::level::warn;
		default: return spdlog::level::info;
		}
	}();
	auto logger = spdlog::get(konst::GRAPHICS);
	if (logger) {
		logger->log(level, "[{} {}]: {}", source_name, type_name, message);
	}
}
