#include <spdlog/spdlog.h>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>
#include <apostellein/konst.hpp>

#include "./collision.hpp"
#include "./kinematics.hpp"
#include "../x2d/tile-map.hpp"

namespace {
	bool is_slope_opposing_(const collision::result& res, side_type side) {
		if (res.tile.flags.floor) {
			return side == side_type::top;
		} else if (res.tile.flags.ceiling) {
			return side == side_type::bottom;
		}
		return false;
	}

	r32 slope_multiplier_(const collision::result& res) {
		if (res.tile.flags.positive) {
			return 0.5f;
		}
		return -0.5f;
	}

	r32 slope_height_(const collision::result& res) {
		if (res.tile.flags.ceiling) {
			if (res.tile.flags.negative) {
				if (res.tile.flags.high) {
					return konst::TILE<r32>();
				}
			} else if (res.tile.flags.positive) {
				if (res.tile.flags.low) {
					return 0.0f;
				}
			}
		} else if (res.tile.flags.floor) {
			if (res.tile.flags.negative) {
				if (res.tile.flags.low) {
					return konst::TILE<r32>();
				}
			} else if (res.tile.flags.positive) {
				if (res.tile.flags.high) {
					return 0.0f;
				}
			}
		}
		return konst::HALF_TILE<r32>();
	}

	collision::info test_result_(
		const rect& delta,
		const collision::result& res,
		side_type opposite,
		r32 perpendicular,
		r32 leading,
		bool should_test_slopes
	) {
		collision::info inf { leading };
		if (res.tile.flags.block) {
			const rect hitbox = res.hitbox();
			if (delta.overlaps(hitbox)) {
				switch (opposite) {
					case side_type::right: {
						if (!res.tile.flags.fall_through) {
							inf.valid = true;
							inf.coordinate = hitbox.right();
						}
						break;
					}
					case side_type::left: {
						if (!res.tile.flags.fall_through) {
							inf.valid = true;
							inf.coordinate = hitbox.x;
						}
						break;
					}
					case side_type::top: {
						if (res.tile.flags.fall_through) {
							if ((delta.bottom() - konst::HALF_TILE<r32>()) < hitbox.y) {
								inf.valid = true;
								inf.coordinate = hitbox.y;
							}
						} else {
							inf.valid = true;
							inf.coordinate = hitbox.y;
						}
						break;
					}
					case side_type::bottom: {
						inf.valid = true;
						inf.coordinate = hitbox.bottom();
						break;
					}
				}
			}
		} else if (
			should_test_slopes and
			res.tile.flags.sloped and
			is_slope_opposing_(res, opposite)
		) {
			const rect hitbox = res.hitbox();
			const r32 multiplier = slope_multiplier_(res);
			const r32 height = slope_height_(res);
			inf.coordinate = opposite.vertical() ?
				multiplier * (perpendicular - hitbox.x) + height + hitbox.y :
				(perpendicular - hitbox.y - height) / multiplier + hitbox.x;
			inf.valid = opposite.maximum() ?
				leading <= inf.coordinate :
				leading >= inf.coordinate;
		}
		return inf;
	}
}

rect collision::result::hitbox() const {
	return {
		glm::vec2(index) * konst::TILE<r32>(),
		{ konst::TILE<r32>(), konst::TILE<r32>() }
	};
}

std::optional<collision::result> collision::check(
	const tile_map& map,
	const ecs::kinematics& kin,
	const rect& delta,
	side_type side
) {
	const glm::vec2 center = delta.center();
	const auto first_primary = konst::TILE_ROUND(delta.side(side.opposite()));
	const auto final_primary = konst::TILE_ROUND(delta.side(side));
	const auto incrm_primary = side.maximum() ? 1 : -1;
	const bool is_horizontal = side.horizontal();
	const auto s_min = konst::TILE_ROUND(is_horizontal ? delta.y : delta.x);
	const auto s_mid = konst::TILE_ROUND(is_horizontal ? center.y : center.x);
	const auto s_max = konst::TILE_ROUND(is_horizontal ? delta.bottom() : delta.right());
	const bool s_positive = s_mid - s_min < s_max - s_mid;
	const auto incrm_secondary = s_positive ? 1 : -1;
	const auto first_secondary = s_positive ? s_min : s_max;
	const auto final_secondary = !s_positive ? s_min : s_max;
	for (i32 primary = first_primary;
		primary != final_primary + incrm_primary;
		primary += incrm_primary
	) {
		for (i32 secondary = first_secondary;
			secondary != final_secondary + incrm_secondary;
			secondary += incrm_secondary
		) {
			const auto y = !is_horizontal ? primary : secondary;
			const auto x = is_horizontal ? primary : secondary;
			const tile_type t = map.tile(x, y);
			collision::result res { x, y, t };
			if (res.tile.flags.out_of_bounds) {
				return res;
			}
			const side_type opposite = side.opposite();
			bool should_test_slopes = opposite.vertical();

			const auto perpendicular = opposite.vertical() ?
				center.x :
				center.y;
			const auto leading = delta.side(side);
			collision::info inf = test_result_(
				delta, res, opposite,
				perpendicular,
				leading,
				should_test_slopes
			);
			if (inf.valid) {
				res.coordinate = inf.coordinate;
				return res;
			} else if (
				(side == side_type::bottom and kin.flags.bottom) or
				(side == side_type::top and kin.flags.top)
			) {
				if ((kin.flags.sloped and res.tile.flags.sloped) or (!kin.flags.sloped and res.tile.flags.high)) {
					res.coordinate = inf.coordinate;
					return res;
				}
			}
		}
	}
	return std::nullopt;
}

std::optional<glm::vec2> collision::intersects(
	const glm::vec2& o, // ray origin
	const glm::vec2& d, // ray direction
	const glm::vec2& a, // line point a
	const glm::vec2& b  // line point b
) {
	// Cross product doesn't exist in 2D, so just calculate
	// the z component and return that.
	auto cross = [](const glm::vec2& v1, const glm::vec2& v2) {
		return v1.x * v2.y - v1.y * v2.x;
	};

	const glm::vec2 v1 = o - a;
	const glm::vec2 v2 = b - a;
	const glm::vec2 v3 { -d.y, d.x };
	const auto dot = glm::dot(v2, v3);

	const auto t2 = glm::dot(v1, v3) / dot;
	if (0.0f > t2 or t2 > 1.0f) {
		return std::nullopt;
	}
	const auto t1 = cross(v2, v1) / dot;
	if (t1 < 0.0f) {
		return std::nullopt;
	}
	return o + d * t1;
}

glm::vec2 collision::trace(const tile_map& map, const glm::vec2& origin, const glm::vec2& direction, r32 maximum) {
	if (direction.x == 0.0f or direction.y == 0.0f) {
		return origin;
	}
	const glm::vec2 step {
		direction.x > 0.0f ? 1.0f : -1.0f,
		direction.y > 0.0f ? 1.0f : -1.0f
	};
	const glm::vec2 length_delta = glm::abs(1.0f / direction);

	glm::vec2 index = glm::floor(origin);

	const glm::vec2 distance {
		step.x > 0.0f ?
			index.x + 1.0f - origin.x :
			origin.x - index.x,
		step.y > 0.0f ?
			index.y + 1.0f - origin.y :
			origin.y - index.y
	};
	glm::vec2 maximum_delta {
		length_delta.x * distance.x,
		length_delta.y * distance.y
	};
	r32 length = 0.0f;
	while (length <= maximum) {
		const glm::length_t I = maximum_delta.x < maximum_delta.y ? 0 : 1;
		index[I] += step[I];
		length = maximum_delta[I];
		maximum_delta[I] += length_delta[I];
		const auto x = konst::TILE_ROUND(index.x);
		const auto y = konst::TILE_ROUND(index.y);
		const tile_type tile = map.tile(x, y);
		if (tile.any() and !(tile.flags.fall_through or tile.flags.out_of_bounds)) {
			if (tile.flags.block) {
				if (tile.flags.hookable) {
					return {
						konst::TILE_ALIGN(x) + konst::HALF_TILE<r32>(),
						konst::TILE_ALIGN(y) + konst::HALF_TILE<r32>()
					};
				}
				return origin + direction * length;
			} else if (tile.flags.sloped) {
				const auto left = konst::TILE_ALIGN(x);
				const auto top = konst::TILE_ALIGN(y);
				const auto right = konst::TILE_ALIGN(x + 1);
				const auto bottom = konst::TILE_ALIGN(y + 1);
				const auto center = top + konst::HALF_TILE<r32>();

				std::optional<glm::vec2> maybe {};
				switch (const auto slope = tile.slope()) {
				case 1:
				case 7:
					maybe = collision::intersects(
						origin, direction,
						glm::vec2{ left, top },
						glm::vec2{ right, center }
					);
					break;
				case 2:
				case 8:
					maybe = collision::intersects(
						origin, direction,
						glm::vec2{ left, center },
						glm::vec2{ right, bottom }
					);
					break;
				case 3:
				case 5:
					maybe = collision::intersects(
						origin, direction,
						glm::vec2{ left, bottom },
						glm::vec2{ right, center }
					);
					break;
				case 4:
				case 6:
					maybe = collision::intersects(
						origin, direction,
						glm::vec2{ left, center },
						glm::vec2{ right, top }
					);
					break;
				default:
					spdlog::error("collision::trace() found a sloped tile with a slope type of {}!", slope);
					break;
				}
				if (maybe) {
					return *maybe;
				}
			}
		}
	}
	return origin + direction * length;
}

glm::vec2 collision::trace(const tile_map& map, const glm::vec2& origin, r32 angle, r32 maximum) {
	const glm::vec2 direction {
		glm::cos(angle),
		glm::sin(angle)
	};
	return collision::trace(
		map,
		origin,
		direction,
		maximum
	);
}
