#pragma once

#include <glad/glad.h>
#include <apostellein/def.hpp>

namespace ogl {
	enum class context_type {
		none, v31, v32,
		v33, v40, v41,
		v42, v43, v44,
		v45, v46
	};
	inline context_type& operator--(context_type& ctx) {
		if (ctx > context_type::none) {
			ctx = static_cast<context_type>(
				static_cast<std::underlying_type_t<context_type> >(ctx) - 1
			);
		}
		return ctx;
	}
	extern context_type version;
	u32 major_version() noexcept;
	u32 minor_version() noexcept;
	u32 glsl_version() noexcept;
	u32 memory_usage() noexcept;
	bool modern_shaders_available() noexcept;
	bool binding_points_available() noexcept;
	bool debug_callback_available() noexcept;
	bool buffer_storage_available() noexcept;
	bool direct_state_available() noexcept;
	bool texture_storage_available() noexcept;
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
