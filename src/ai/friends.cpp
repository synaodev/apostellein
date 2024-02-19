#include <apostellein/konst.hpp>

#include "./friends.hpp"
#include "./particles.hpp"

#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/kinematics.hpp"
#include "../ecs/liquid.hpp"
#include "../hw/audio.hpp"
#include "../util/id-table.hpp"

namespace {
	constexpr i32 SHOSHI_SPRITE_LAYER = 1;
	constexpr r32 COLLISION_Y_TOP = 1.0f;
	constexpr r32 COLLISION_Y_HEIGHT = 15.0f;
	constexpr r32 COLLISION_TOP_WIDTH = 9.0f;
	constexpr r32 COLLISION_BOTTOM_WIDTH = 5.0f;
	constexpr r32 COLLISION_TOP_LEFT = (konst::TILE<r32>() - COLLISION_TOP_WIDTH) / 2.0f;
	constexpr r32 COLLISION_BOTTOM_LEFT = (konst::TILE<r32>() - COLLISION_BOTTOM_WIDTH) / 2.0f;
	constexpr composite_rect COMPOSITE_COLLISION_RECT {
		{ 3.0f, 5.0f, 5.0f, 6.0f },
		{ 8.0f, 5.0f, 5.0f, 6.0f },
		{
			COLLISION_TOP_LEFT,
			COLLISION_Y_TOP,
			COLLISION_TOP_WIDTH,
			COLLISION_Y_HEIGHT / 2.0f
		},
		{
			COLLISION_BOTTOM_LEFT,
			COLLISION_Y_TOP + COLLISION_Y_HEIGHT / 2.0f,
			COLLISION_BOTTOM_WIDTH,
			COLLISION_Y_HEIGHT / 2.0f
		}
	};
}

struct shoshi_state {
	bool augment {};
	bool facing_left {};
};

static bool test_shoshi_sprite_mirroring(mirror_type mirroring, bool facing_left) {
	return (
		(!mirroring and !facing_left) or
		(mirroring.horizontally and facing_left)
	);
}

static void shoshi_tick(entt::entity s, kernel&, camera&, player& plr, environment& env) {
	auto& shoshi = env.get<shoshi_state>(s);
	auto& kinematics = env.get<ecs::kinematics>(s);
	auto& sprite = env.get<ecs::sprite>(s);
	auto& thinker = env.get<ecs::thinker>(s);

	if (!thinker.state) {
		if (kinematics.velocity.x != 0.0f) {
			shoshi.facing_left = kinematics.velocity.x < 0.0f;
			if (test_shoshi_sprite_mirroring(sprite.mirror, shoshi.facing_left)) {
				sprite.mirror.horizontally = shoshi.facing_left;
			}
			sprite.state(kinematics.flags.bottom ? 1 : 2);
		} else if (!kinematics.flags.bottom) {
			sprite.state(2);
		} else if (sprite.state() != 4) {
			sprite.state(0);
		}
	}
	kinematics.accel_y(0.1f, 6.0f);
}

static void shoshi_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 4.0f, 4.0f, 8.0f, 12.0f };

	auto& sprite = env.emplace<ecs::sprite>(s, anim::Shoshi);
	sprite.layer = SHOSHI_SPRITE_LAYER;

	auto& blinker = env.emplace<ecs::blinker>(s);
	blinker.return_state = 0;
	blinker.blink_state = 4;

	env.emplace<ecs::thinker>(s, shoshi_tick);
}

static void carried_shoshi_tick(entt::entity s, kernel&, camera&, player& plr, environment& env) {
	auto& naomi_location = env.get<ecs::location>(plr.entity());
	auto& location = env.get<ecs::location>(s);
	location.position = naomi_location.position;

	auto& naomi_sprite = env.get<ecs::sprite>(plr.entity());
	auto& sprite = env.get<ecs::sprite>(s);
	sprite.mirror = naomi_sprite.mirror;
}

static void carried_shoshi_ctor(entt::entity s, environment& env) {
	auto& location = env.get<ecs::location>(s);
	location.hitbox = { 4.0f, 4.0f, 8.0f, 12.0f };

	auto& sprite = env.emplace<ecs::sprite>(s, anim::Shoshi);
	sprite.state(3);
	sprite.layer = SHOSHI_SPRITE_LAYER;

	auto& blinker = env.emplace<ecs::blinker>(s);
	blinker.return_state = 3;
	blinker.blink_state = 5;

	env.emplace<ecs::thinker>(s, carried_shoshi_tick);
}

static void accompanied_shoshi_tick(entt::entity s, kernel&, camera&, player& plr, environment& env) {
	auto& shoshi = env.get<shoshi_state>(s);
	auto& location = env.get<ecs::location>(s);
	auto& kinematics = env.get<ecs::kinematics>(s);
	auto& sprite = env.get<ecs::sprite>(s);
	auto& submersible = env.get<ecs::submersible>(s);
	auto& naomi_location = env.get<ecs::location>(plr.entity());
	auto& naomi_kinematics = env.get<ecs::kinematics>(plr.entity());

	bool should_jump = false;
	const glm::vec2 naomi_center = naomi_location.center();
	const glm::vec2 shoshi_center = location.center();
	const glm::vec2 box_distance {
		glm::abs(naomi_center.x - shoshi_center.x),
		glm::abs(naomi_center.y - shoshi_center.y)
	};

	r32 kMaxHsp = 2.0f;
	r32 kMaxVsp = 6.0f;
	r32 kAccelX = 0.1f;
	r32 kDecelX = 0.24f;
	r32 kJumpPw = 3.7f;
	r32 kJumpHd = 0.16f;
	r32 kGravSp = 0.28f;

	if (env.valid(submersible.entity)) {
		kMaxHsp /= 2.0f;
		kMaxVsp /= 2.0f;
		kAccelX /= 2.0f;
		kDecelX /= 2.0f;
		kJumpPw /= 1.5f;
		kJumpHd /= 1.5f;
		kGravSp /= 2.0f;
	}

	if (box_distance.y > 16.0f) {
		if (shoshi_center.y < naomi_center.y) {
			if (kinematics.flags.fall_through and kinematics.flags.bottom) {
				kinematics.flags.will_drop = true;
				kinematics.flags.bottom = false;
			}
		} else if (naomi_kinematics.flags.fall_through) {
			should_jump = true;
		}
	}
	if (box_distance.x > 16.0f) {
		if (naomi_center.x > shoshi_center.x) {
			shoshi.facing_left = false;
			kinematics.accel_x(kAccelX, kMaxHsp);
			if (kinematics.flags.right) {
				should_jump = true;
			}
		} else {
			shoshi.facing_left = true;
			kinematics.accel_x(-kAccelX, kMaxHsp);
			if (kinematics.flags.left) {
				should_jump = true;
			}
		}
		if (kinematics.flags.bottom) {
			sprite.state(1);
			shoshi.augment = true;
		} else if (shoshi.augment) {
			sprite.state(2);
			shoshi.augment = false;
			kinematics.velocity.y = -kJumpPw;
			should_jump = true;
			audio::play(sfx::Jump, 0);
		} else {
			sprite.state(2);
		}
		if (should_jump and kinematics.flags.bottom) {
			kinematics.velocity.y = -kJumpPw;
		}
		if (kinematics.velocity.y < 0.0f) {
			kinematics.velocity.y -= kJumpHd;
		}
	} else if (kinematics.flags.bottom) {
		kinematics.decel_x(kDecelX);
		sprite.state(0);
		shoshi.augment = true;
	}
	kinematics.accel_y(kGravSp, kMaxVsp);
	sprite.mirror.horizontally = shoshi.facing_left;
}

static void accompanied_shoshi_ctor(entt::entity s, environment& env) {
	env.emplace<shoshi_state>(s);

	auto& location = env.get<ecs::location>(s);
	location.hitbox = { 4.0f, 4.0f, 8.0f, 12.0f };

	auto& sprite = env.emplace<ecs::sprite>(s, anim::Shoshi);
	sprite.layer = SHOSHI_SPRITE_LAYER;

	auto& kinematics = env.emplace<ecs::kinematics>(s);
	kinematics.hitbox = COMPOSITE_COLLISION_RECT;

	auto& blinker = env.emplace<ecs::blinker>(s);
	blinker.return_state = 0;
	blinker.blink_state = 4;

	env.emplace<ecs::thinker>(s, accompanied_shoshi_tick);
	env.emplace<ecs::submersible>(s, ai::splash, sfx::Splash);
}

// Tables

APOSTELLEIN_THINKER_TABLE(shoshi) {
	APOSTELLEIN_THINKER_ENTRY(ai::shoshi, shoshi_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::carried_shoshi, carried_shoshi_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::accompanied_shoshi, accompanied_shoshi_ctor);
}
