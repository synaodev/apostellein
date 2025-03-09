#include <glm/vec2.hpp>
#include <apostellein/cast.hpp>
#include <apostellein/struct.hpp>

#include "./swap-chain.hpp"
#include "./opengl.hpp"

namespace swap_chain {
	// private
	glm::ivec2 dimensions_ {};
	color_type color_ { color_type::BASE() };
	blending_type blending_ { blending_type::alpha };
}

// public
void swap_chain::reset() {
	glCheck(glEnable(GL_BLEND));
	glCheck(glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD));
	glCheck(glBlendFuncSeparate(
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE,
		GL_ONE_MINUS_SRC_ALPHA
	));

	dimensions_ = {};
	color_ = color_type::BASE();
	blending_ = blending_type::alpha;
}

void swap_chain::clear(const color_type& color) {
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

void swap_chain::viewport(const glm::ivec2& dimensions) {
	if (dimensions_ != dimensions) {
		dimensions_ = dimensions;
		glCheck(glViewport(0, 0, dimensions_.x, dimensions_.y));
	}
}

const glm::ivec2& swap_chain::viewport() {
	return dimensions_;
}

void swap_chain::blend(blending_type blending) {
	if (blending_ != blending) {
		switch (blending_ = blending; blending_) {
		case blending_type::alpha: {
			glCheck(glBlendFuncSeparate(
				GL_SRC_ALPHA,
				GL_ONE_MINUS_SRC_ALPHA,
				GL_ONE,
				GL_ONE_MINUS_SRC_ALPHA
			));
			break;
		}
		case blending_type::add: {
			glCheck(glBlendFuncSeparate(
				GL_SRC_ALPHA,
				GL_ONE,
				GL_ONE,
				GL_ONE
			));
			break;
		}
		case blending_type::multiply: {
			glCheck(glBlendFuncSeparate(
				GL_DST_COLOR,
				GL_ZERO,
				GL_DST_COLOR,
				GL_ZERO
			));
			break;
		}
		default:
			break;
		}
	}
}
