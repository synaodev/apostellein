#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./opengl.hpp"

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
