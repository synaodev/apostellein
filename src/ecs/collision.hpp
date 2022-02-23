#pragma once

#include <optional>
#include <apostellein/rect.hpp>

#include "../x2d/tile-type.hpp"

struct tile_map;

namespace ecs {
	struct kinematics;
}

namespace collision {
	struct info {
		info() noexcept = default;
		info(r32 _coordinate) noexcept :
			coordinate{ _coordinate } {}

		bool valid {};
		r32 coordinate {};
	};
	struct result {
		result() noexcept = default;
		result(i32 x, i32 y, tile_type _tile) noexcept :
			index{ x, y },
			tile{ _tile } {}

		glm::ivec2 index {};
		tile_type tile {};
		r32 coordinate {};
	public:
		rect hitbox() const;
	};
	std::optional<collision::result> check(
		const tile_map& map,
		const ecs::kinematics& kin,
		const rect& delta,
		side_type side
	);
	std::optional<glm::vec2> intersects(
		const glm::vec2& o,
		const glm::vec2& d,
		const glm::vec2& a,
		const glm::vec2& b
	);
	glm::vec2 trace(
		const tile_map& map,
		const glm::vec2& origin,
		const glm::vec2& direction,
		r32 maximum
	);
	glm::vec2 trace(
		const tile_map& map,
		const glm::vec2& origin,
		r32 angle,
		r32 maximum
	);
}
