#include <glm/vec2.hpp>
#include <apostellein/cast.hpp>
#include <apostellein/struct.hpp>

#include "./swap-chain.hpp"
#include "./opengl.hpp"

namespace swap_chain {
	// private
	glm::ivec2 dimensions_ {};
	color_type color_ { color_type::WHITE() };
	// public
	void reset() {
		dimensions_ = {};
		color_ = color_type::WHITE();
	}
	void clear(const color_type& color) {
		if (color_ != color) {
			color_ = color;
			glCheck(glClearColor(
				cast<r32>(color_.r) / 255.0f,
				cast<r32>(color_.g) / 255.0f,
				cast<r32>(color_.b) / 255.0f,
				cast<r32>(color_.a) / 255.0f
			));
		}
		glCheck(glClear(GL_COLOR_BUFFER_BIT));
	}
	void viewport(const glm::ivec2& dimensions) {
		if (dimensions_ != dimensions) {
			dimensions_ = dimensions;
			glCheck(glViewport(0, 0, dimensions_.x, dimensions_.y));
		}
	}
	const glm::ivec2& viewport() {
		return dimensions_;
	}
}
