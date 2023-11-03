#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

#include "./weapons.hpp"
#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/health.hpp"
#include "../ecs/kinematics.hpp"
#include "../util/id-table.hpp"

namespace {
	constexpr i32 ARM_SPRITE_LAYER = 2;
	constexpr i32 HAND_SPRITE_LAYER = 3;
	constexpr i32 HAND_TIMER = 40;
	constexpr r32 HAND_SPEED = 3.0f;
}

static void hand_tick(entt::entity s, kernel&, camera&, player& plr, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	auto& plr_loc = env.get<ecs::location>(plr.entity());
	auto& chr = env.get<ecs::chroniker>(s);
	auto& kin = env.get<ecs::kinematics>(s);
	auto& spr = env.get<ecs::sprite>(s);

	const glm::vec2 plr_center = plr_loc.center();
	const glm::vec2 hand_center = loc.center();
	const auto angle = glm::atan(
		plr_center.y - hand_center.y,
		plr_center.x - hand_center.x
	);
	spr.angle = -angle + glm::pi<r32>();

	if (--chr[0]; chr[0] <= 0) {
		kin.move_at(angle, HAND_SPEED);
		if (loc.overlaps(plr_loc)) {
			env.dispose(s);
		}
	}
}

static void hand_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	auto& dir = env.get<ecs::direction>(s);
	auto& chr = env.emplace<ecs::chroniker>(s);
	auto& kin = env.emplace<ecs::kinematics>(s);
	auto& spt = env.emplace<ecs::sprite>(s, anim::ArmHand);

	chr[0] = HAND_TIMER;
	kin.flags.no_clip = true;

	switch (dir) {
	case ecs::direction::right:
		kin.velocity.x = HAND_SPEED;
		break;
	case ecs::direction::left:
		kin.velocity.x = -HAND_SPEED;
		break;
	case ecs::direction::up:
		kin.velocity.y = -HAND_SPEED;
		break;
	default:
		kin.velocity.y = HAND_SPEED;
		break;
	}

	spt.state(1);
	spt.layer = HAND_SPRITE_LAYER;
	spt.pivot = { 8.0f, 8.0f };

	env.emplace<ecs::thinker>(s, hand_tick);
}

static void arm_tick(entt::entity s, kernel&, camera&, player& plr, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	auto& spt = env.get<ecs::sprite>(s);
	if (auto hand = env.search(ai::hand); hand != entt::null) {
		const glm::vec2 plr_center = env.get<ecs::location>(plr.entity()).center();
		const glm::vec2 hand_center = env.get<ecs::location>(hand).center();
		const auto angle = glm::atan(
			plr_center.y - hand_center.y,
			plr_center.x - hand_center.x
		);
		const auto distance = glm::distance(plr_center, hand_center);
		loc.position = hand_center;
		spt.angle = -angle;
		spt.scale = { distance, 1.0f };
	} else {
		env.dispose(s);
	}
}

static void arm_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 0.0f, 0.0f, 1.0f, 1.0f };

	auto& kin = env.emplace<ecs::kinematics>(s);
	kin.flags.no_clip = true;

	auto& spt = env.emplace<ecs::sprite>(s, anim::ArmHand);
	spt.layer = ARM_SPRITE_LAYER;

	env.emplace<ecs::thinker>(s, arm_tick);
}

APOSTELLEIN_THINKER_TABLE(weapon) {
	APOSTELLEIN_THINKER_ENTRY(ai::hand, hand_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::arm, arm_ctor);
}
