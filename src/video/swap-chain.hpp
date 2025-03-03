#pragma once

#include <glm/fwd.hpp>

struct color_type;

namespace swap_chain {
	void reset();
	void clear(const color_type& color);
	void viewport(const glm::ivec2& dimensions);
	const glm::ivec2& viewport();
}
