#pragma once

#include <glad/glad.h>
#include <apostellein/def.hpp>

namespace ogl {
	void check_errors(const char* path, u32 line, const char* expr);
	void APIENTRY debug_callback(
		GLenum source,
		GLenum type,
		GLuint /* id */,
		GLenum severity,
		GLsizei /* length */,
		const GLchar* message,
		const void* /* param */
	) noexcept;
}

#if defined(APOSTELLEIN_OPENGL_LOGGING)
	#define glCheck(EXPR) do { EXPR; ogl::check_errors(__FILE__, __LINE__, #EXPR); } while(0)
#else
	#define glCheck(EXPR) (EXPR)
#endif
