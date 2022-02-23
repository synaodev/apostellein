#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <apostellein/konst.hpp>

#include "./kinematics.hpp"
#include "./collision.hpp"
#include "./aktor.hpp"
#include "../field/environment.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr chroma RED_TINT { 0xFFU, 0x00U, 0x00U, 0x7FU };
	constexpr chroma GREEN_TINT { 0x00U, 0xFFU, 0x00U, 0x7FU };
	constexpr chroma BLUE_TINT { 0x00U, 0x00U, 0xFFU, 0x7FU };
	constexpr chroma WHITE_TINT { 0xFFU, 0xFFU, 0xFFU, 0x7FU };
}

void ecs::kinematics::move_at(r32 angle, r32 speed) {
	velocity = {
		glm::cos(angle) * speed,
		glm::sin(angle) * speed
	};
}

void ecs::kinematics::accel_x(r32 speed, r32 limit, r32 detract) {
	limit = glm::abs(limit);
	if (velocity.x > limit or velocity.x < -limit) {
		detract = glm::abs(detract);
		velocity.x = velocity.x > 0.0f ?
			glm::max(velocity.x - detract, limit) :
			glm::min(velocity.x + detract, -limit);
	} else {
		velocity.x = speed > 0.0f ?
			glm::min(velocity.x + speed, limit) :
			glm::max(velocity.x + speed, -limit);
	}
}

void ecs::kinematics::accel_y(r32 speed, r32 limit, r32 detract) {
	limit = glm::abs(limit);
	if (velocity.y > limit or velocity.y < -limit) {
		detract = glm::abs(detract);
		velocity.y = velocity.y > 0.0f ?
			glm::max(velocity.y - detract, limit) :
			glm::min(velocity.y + detract, -limit);
	} else {
		velocity.y = speed > 0.0f ?
			glm::min(velocity.y + speed, limit) :
			glm::max(velocity.y + speed, -limit);
	}
}

void ecs::kinematics::decel_x(r32 rate, r32 zero) {
	rate = glm::abs(rate);
	velocity.x = velocity.x > zero ?
		glm::max(zero, velocity.x - rate) :
		glm::min(zero, velocity.x + rate);
}

void ecs::kinematics::decel_y(r32 rate, r32 zero) {
	rate = glm::abs(rate);
	velocity.y = velocity.y > zero ?
		glm::max(zero, velocity.y - rate) :
		glm::min(zero, velocity.y + rate);
}

void ecs::kinematics::handle(environment& env, const tile_map& map) {
	env.slice<ecs::location, ecs::kinematics>().each(
	[&env, &map](entt::entity e, location& loc, kinematics& kin) {
		if (kin.velocity.x != 0.0f) {
			kin.try_x_(loc, map, kin.velocity.x);
		}
		if (kin.velocity.y != 0.0f) {
			kin.try_y_(loc, map, kin.velocity.y);
		}
		if (env.has<ecs::anchor>(e)) {
			auto& anc = env.get<ecs::anchor>(e);
			if (anc.length > 0.0f) {
				kin.try_a_(loc, anc, kin.velocity);
			}
		}
	});
}

void ecs::kinematics::render(renderer& rdr, const environment& env) {
	auto& list = rdr.query(
		priority_type::automatic,
		blending_type::add,
		pipeline_type::blank
	);
	env.slice<ecs::location, ecs::kinematics>().each(
	[&list](entt::entity, const ecs::location& loc, const ecs::kinematics& kin) {
		list.batch_blank(
			kin.hitbox.left(loc.position, 0.0f),
			BLUE_TINT
		);
		list.batch_blank(
			kin.hitbox.right(loc.position, 0.0f),
			WHITE_TINT
		);
		list.batch_blank(
			kin.hitbox.top(loc.position, 0.0f),
			RED_TINT
		);
		list.batch_blank(
			kin.hitbox.bottom(loc.position, 0.0f),
			GREEN_TINT
		);
	});
}

void ecs::kinematics::try_x_(ecs::location& loc, const tile_map& map, r32 inertia) {
	if (!flags.no_clip) {
		const side_type side = inertia > 0.0f ?
			side_type::right :
			side_type::left;
		{
			const rect delta = hitbox.side(side, loc.position, inertia);
			const auto result = collision::check(map, *this, delta, side);
			if (result) {
				loc.position.x = result->coordinate - hitbox.bounds().side(side);
				velocity.x = 0.0f;
				flags.right = side == side_type::right;
				flags.left = side == side_type::left;
				flags.hooked = result->tile.flags.hookable;
			} else {
				loc.position.x += inertia;
				flags.right = false;
				flags.left = false;
			}
		}
	} else {
		loc.position.x += inertia;
		flags.right = false;
		flags.left = false;
	}
}

void ecs::kinematics::try_y_(ecs::location& loc, const tile_map& map, r32 inertia) {
	if (!flags.no_clip) {
		const side_type side = inertia > 0.0f ?
			side_type::bottom :
			side_type::top;
		{
			const rect delta = hitbox.side(side, loc.position, inertia);
			const auto result = collision::check(map, *this, delta, side);
			if (result) {
				auto& tile = result->tile;
				if (!tile.flags.out_of_bounds) {
					if (
						tile.flags.fall_through and
						(side != side_type::bottom or flags.will_drop)
					) {
						loc.position.y += inertia;
						flags.top = false;
						flags.bottom = false;
						flags.sloped = false;
					} else {
						loc.position.y = result->coordinate - hitbox.bounds().side(side);
						velocity.y = 0.0f;
						flags.hooked = tile.flags.hookable;
						flags.sloped = tile.flags.sloped;
						if (side == side_type::top) {
							flags.top = true;
							flags.bottom = false;
							flags.fall_through = false;
						} else {
							flags.top = false;
							flags.bottom = true;
							flags.fall_through = tile.flags.fall_through;
						}
					}
				} else {
					loc.position.y += inertia;
					flags.out_of_bounds = true;
					flags.no_clip = true;
				}
			} else {
				loc.position.y += inertia;
				flags.top = false;
				flags.bottom = false;
				flags.sloped = false;
				flags.will_drop = false;
			}
		}
	} else {
		loc.position.y += inertia;
		flags.top = false;
		flags.bottom = false;
		flags.sloped = false;
		flags.will_drop = false;
	}
}

void ecs::kinematics::try_a_(ecs::location& loc, ecs::anchor& anc, glm::vec2& inertia) {
	const glm::vec2 testing = loc.center();
	if (const auto distance = glm::distance(testing, anc.position); distance > anc.length) {
		flags.constrained = true;
		const auto difference = distance - anc.length;
		const auto angle = glm::atan(
			anc.position.x - testing.x,
			anc.position.y - testing.y
		);
		const glm::vec2 direction {
			glm::cos(angle),
			glm::sin(angle)
		};
		const glm::vec2 result = testing + (direction * difference);
		loc.position = result - loc.hitbox.center();
		if (inertia != glm::zero<glm::vec2>()) {
			const glm::vec2 normalized = glm::normalize(result - anc.position);
			const glm::vec2 perpendicular {
				normalized.y,
				-normalized.x
			};
			const glm::vec2 product = perpendicular * glm::dot(perpendicular, inertia);
			if (product != glm::zero<glm::vec2>()) {
				inertia = glm::normalize(product) * glm::length(inertia);
			}
		}
	} else {
		flags.constrained = false;
	}
}
