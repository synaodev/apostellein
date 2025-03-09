#pragma once

#include <glm/fwd.hpp>
#include <apostellein/def.hpp>

struct color_type;

enum class blending_type : udx {
	alpha,
	add,
	multiply
};

namespace swap_chain {
	void reset();
	void clear(const color_type& color);
	void viewport(const glm::ivec2& dimensions);
	const glm::ivec2& viewport();
	void blend(blending_type blending);
}
