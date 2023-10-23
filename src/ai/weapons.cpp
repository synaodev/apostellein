#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "./weapons.hpp"
#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/health.hpp"
#include "../ecs/kinematics.hpp"
#include "../util/id-table.hpp"

namespace {
	constexpr i32 ARM_SPRITE_LAYER = 3;
}

static void hand_tick(entt::entity s, kernel&, camera&, player&, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	auto& chr = env.get<ecs::chroniker>(s);
	auto& kin = env.get<ecs::kinematics>(s);

	if (--chr[0]; chr[0] > 0) {

	} else {
		env.dispose(s);
	}
}

static void hand_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 0.0f, 0.0f, 1.0f, 1.0f };

	auto& chr = env.emplace<ecs::chroniker>(s);
	chr[0] = 10;

	auto& kin = env.emplace<ecs::kinematics>(s);
	kin.flags.no_clip = true;

	auto& spt = env.emplace<ecs::sprite>(s, anim::ArmHand);
	spt.layer = ARM_SPRITE_LAYER;

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
		loc.position = plr_center;
		spt.angle = angle;
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
	spt.pivot = { 0.0f, 1.0f };

	env.emplace<ecs::thinker>(s, arm_tick);
}

APOSTELLEIN_THINKER_TABLE(weapon) {
	APOSTELLEIN_THINKER_ENTRY(ai::hand, hand_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::arm, arm_ctor);
}
