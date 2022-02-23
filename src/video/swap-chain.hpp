#pragma once

#include <glm/fwd.hpp>

struct chroma;

namespace swap_chain {
	void reset();
	void clear(const chroma& color);
	void viewport(const glm::ivec2& dimensions);
	const glm::ivec2& viewport();
}
