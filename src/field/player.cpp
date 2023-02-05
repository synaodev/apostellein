#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./player.hpp"
#include "./environment.hpp"
#include "./camera.hpp"
#include "../ai/particles.hpp"
#include "../ai/friends.hpp"
#include "../ai/weapons.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/collision.hpp"
#include "../ecs/kinematics.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/health.hpp"
#include "../ecs/liquid.hpp"
#include "../ctrl/controller.hpp"
#include "../ctrl/kernel.hpp"
#include "../menu/headsup.hpp"
#include "../util/buttons.hpp"
#include "../util/id-table.hpp"
#include "../hw/audio.hpp"

namespace {
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
	constexpr rect LOCATION_HITBOX {
		konst::TILE<r32>() / 4.0f,
		0.0f,
		konst::TILE<r32>() / 2.0f,
		konst::TILE<r32>()
	};
	constexpr r32 INCR_OFFSET_X = 1.0f;
	constexpr r32 FAR_OFFSET_X = 48.0f;
	constexpr r32 INCR_OFFSET_Y1 = 2.0f;
	constexpr r32 INCR_OFFSET_Y2 = 6.0f;
	constexpr r32 GROUND_OFFSET_Y = -8.0f;
	constexpr r32 AIR_OFFSET_Y = 80.0f;
	constexpr i32 DEFAULT_SPRITE_LAYER = 1;
	constexpr i32 ABSOLUTE_STARTING_BARRIER = 2;
	constexpr i32 ABSOLUTE_MAXIMUM_BARRIER = 16;
	constexpr i32 ABSOLUTE_MAXIMUM_POISON = 999;
	constexpr i32 ABSOLUTE_MAXIMUM_OXYGEN = 1500;
	constexpr i32 OXYGEN_TIMER_FACTOR = 15;
	constexpr i32 BARRIER_BROKEN_TIMER = 360;
	constexpr i32 INVINCIBILITY_TIMER = 60;
	constexpr i32 POISON_TIMER = 180;
	constexpr i32 CHARGING_TIMER = 180;
	constexpr i32 WALL_JUMP_TIMER = 10;
	constexpr i32 FREE_FALL_TIMER = 60;
	constexpr i32 FLASH_TIMER = 60;
	constexpr i32 POISON_LEVEL_ONE = 350;
	constexpr i32 POISON_LEVEL_TWO = 650;
	constexpr char BARRIER_ENTRY[] = "Barrier";
	constexpr char POISON_ENTRY[] = "Poison";
	constexpr char EQUIPMENT_ENTRY[] = "Equipment";
	constexpr char POSITION_ENTRY[] = "Position";
}

namespace player_anim {
	constexpr udx IDLE = 0;
	constexpr udx WALKING = 1;
	constexpr udx JUMPING = 2;
	constexpr udx IDLE_FIRING = 3;
	constexpr udx WALKING_FIRING = 4;
	constexpr udx JUMPING_FIRING = 5;
	constexpr udx TIRED = 6;
	constexpr udx INTERACTING = 7;
	constexpr udx DAMAGED = 8;
	constexpr udx WALL_JUMPING = 9;
	constexpr udx ATTACKING = 10;
	constexpr udx CHARGING = 11;
	constexpr udx DASHING_FLOOR = 12;
	constexpr udx DASHING_WALLS = 13;
	constexpr udx DASHING_CEILING = 14;
	constexpr udx KILLED = 15;
	// constexpr udx SLEEPING = 16;
	// constexpr udx AWAKENING = 17;
	// constexpr udx STANDING_UP = 18;
	constexpr udx BLINKING = 19;
}

namespace player_weapon {
	constexpr i32 KANNON_TYPE = 1;
	constexpr i32 KANNON_RELOAD = 60;
	constexpr i32 FRONTIER_TYPE = 2;
	constexpr i32 FRONTIER_RELOAD = 6;
	constexpr i32 WOLF_VULCAN_TYPE = 3;
	constexpr i32 WOLF_VULCAN_RELOAD = 30;
	constexpr i32 NAIL_RAY_TYPE = 4;
	constexpr i32 NAIL_RAY_RELOAD = 20;
	constexpr i32 AUSTERE_TYPE = 5;
	constexpr i32 AUSTERE_RELOAD = 30;
}

bool player::build(environment& env) {
	auto s = env.allocate();
	if (s != this->entity()) {
		spdlog::critical("Player entity must always be zero!");
		return false;
	}

	auto& loc = env.emplace<ecs::location>(s);
	loc.hitbox = LOCATION_HITBOX;

	auto& kin = env.emplace<ecs::kinematics>(s);
	kin.hitbox = COMPOSITE_COLLISION_RECT;

	auto& spt = env.emplace<ecs::sprite>(s, anim::Naomi);
	spt.layer = DEFAULT_SPRITE_LAYER;

	auto& hel = env.emplace<ecs::health>(s);
	hel.current = ABSOLUTE_STARTING_BARRIER;
	hel.maximum = ABSOLUTE_STARTING_BARRIER;

	auto& blk = env.emplace<ecs::blinker>(s);
	blk.blink_state = player_anim::BLINKING;
	blk.return_state = player_anim::IDLE;

	auto& sub = env.emplace<ecs::submersible>(s);
	sub.particle = ai::splash;
	sub.sound = sfx::Splash;

	return true;
}

void player::clear(environment& env) {
	auto s = this->entity();
	auto [kin, spt, hel, sub] = env.get<
		ecs::kinematics,
		ecs::sprite,
		ecs::health,
		ecs::submersible
	>(s);

	kin.clear();
	kin.flags.bottom = true;
	spt.clear();
	spt.layer = DEFAULT_SPRITE_LAYER;
	hel.clear();
	hel.current = ABSOLUTE_STARTING_BARRIER;
	hel.maximum = ABSOLUTE_STARTING_BARRIER;
	hel.poison = 0;
	sub.entity = entt::null;

	flags_._raw = {};
	timers_.clear();
	equips_._raw = {};
	physics_.clear();
	dir_ = {
		player_direction::hori::right,
		player_direction::vert::none
	};
	previous_dir_ = {
		player_direction::hori::none,
		player_direction::vert::none
	};
	riding_ = {};
	viewpoint_ = {};
}

void player::read(const nlohmann::json& data, environment& env) {
	auto s = this->entity();
	auto [loc, kin, spt, hel, sub] = env.get<
		ecs::location,
		ecs::kinematics,
		ecs::sprite,
		ecs::health,
		ecs::submersible
	>(s);

	if (
		data.contains(BARRIER_ENTRY) and
		data[BARRIER_ENTRY].is_array() and
		data[BARRIER_ENTRY].size() == 2 and
		data[BARRIER_ENTRY][0].is_number_unsigned() and
		data[BARRIER_ENTRY][1].is_number_unsigned()
	) {
		hel.maximum = glm::clamp(
			data[BARRIER_ENTRY][0].get<i32>(),
			ABSOLUTE_STARTING_BARRIER,
			ABSOLUTE_MAXIMUM_BARRIER
		);
		hel.current = glm::clamp(
			data[BARRIER_ENTRY][1].get<i32>(),
			0, hel.maximum
		);
	} else {
		hel.maximum = ABSOLUTE_STARTING_BARRIER;
		hel.current = ABSOLUTE_STARTING_BARRIER;
	}
	if (
		data.contains(POISON_ENTRY) and
		data[POISON_ENTRY].is_number_unsigned()
	) {
		hel.poison = glm::clamp(
			data[POISON_ENTRY].get<i32>(),
			0, ABSOLUTE_MAXIMUM_POISON
		);
	} else {
		hel.poison = 0;
	}
	if (
		data.contains(POSITION_ENTRY) and
		data[POSITION_ENTRY].is_array() and
		data[POSITION_ENTRY].size() == 2 and
		data[POSITION_ENTRY][0].is_number() and
		data[POSITION_ENTRY][1].is_number()
	) {
		loc.position = {
			data[POSITION_ENTRY][0].get<r32>() * konst::TILE<r32>(),
			data[POSITION_ENTRY][1].get<r32>() * konst::TILE<r32>()
		};
	} else {
		loc.position = {};
	}
	if (
		data.contains(EQUIPMENT_ENTRY) and
		data[EQUIPMENT_ENTRY].is_string()
	) {
		const std::string hex = data[EQUIPMENT_ENTRY].get<std::string>();
		equips_._raw = as<u32>(std::strtoul(hex.c_str(), nullptr, 0));
	} else {
		equips_._raw = {};
	}
	kin.clear();
	kin.flags.bottom = true;
	spt.clear();
	spt.layer = DEFAULT_SPRITE_LAYER;
	hel.flags._raw = {};
	sub.entity = entt::null;
	flags_._raw = {};
	timers_.clear();
	physics_.clear();
	dir_ = {
		player_direction::hori::right,
		player_direction::vert::none
	};
	previous_dir_ = {
		player_direction::hori::none,
		player_direction::vert::none
	};
	riding_ = {};
	viewpoint_ = {};
}

void player::write(nlohmann::json& data, const environment& env) const {
	auto s = this->entity();
	auto& hel = env.get<ecs::health>(s);
	data[BARRIER_ENTRY] = nlohmann::json::array({
		hel.maximum,
		hel.current
	});
	data[POISON_ENTRY] = hel.poison;
	auto& loc = env.get<ecs::location>(s);
	data[POSITION_ENTRY] = nlohmann::json::array({
		loc.position.x / konst::TILE<r32>(),
		loc.position.y / konst::TILE<r32>()
	});
	data[EQUIPMENT_ENTRY] = fmt::format(
		"{:#010x}", // 0x00000000
		equips_._raw.value()
	);
}

void player::transfer(i32 id, camera& cam, environment& env) {
	auto s = this->entity();
	auto [loc, kin, spt, sub] = env.get<
		ecs::location,
		ecs::kinematics,
		ecs::sprite,
		ecs::submersible
	>(s);
	if (auto e = env.search(id); e != entt::null) {
		loc.position = env.get<ecs::location>(e).position;
		spt.state(player_anim::IDLE);
	}
	cam.focus_on(loc.center());
	kin.flags.will_drop = false;
	kin.flags.no_clip = false;
	kin.flags.out_of_bounds = false;
	kin.flags.bottom = true;
	ecs::liquid::handle(env, loc, sub);
	if (sub.entity != entt::null) {
		timers_.oxygen = ABSOLUTE_MAXIMUM_OXYGEN;
		physics_.water();
	} else {
		physics_.clear();
	}
	flags_._raw = {};
	dir_ = {
		player_direction::hori::right,
		player_direction::vert::none
	};
	previous_dir_ = {
		player_direction::hori::none,
		player_direction::vert::none
	};
	if (equips_.carry_shoshi) {
		env.spawn(ai::carried_shoshi, loc.position);
	} else if (equips_.accompany_shoshi) {
		env.spawn(ai::accompanied_shoshi, loc.position);
	}
}

void player::damage(entt::entity o, environment& env) {
	static constexpr i32 SMOKE_COUNT = 8;
	static constexpr i32 SHRAPNEL_COUNT = 6;
	static constexpr r32 REPEL_X_VELOCITY = 2.0f;
	static constexpr r32 REPEL_Y_VELOCITY = 4.0f;

	if (!flags_.invincible and !flags_.damaged) {
		auto s = this->entity();
		auto [s_loc, s_hel] = env.get<ecs::location, ecs::health>(s);
		auto [o_loc, o_hel] = env.get<ecs::location, ecs::health>(o);
		// deal damage to naomi
		if (ecs::health::attack(o_loc, s_loc, s_hel, o_hel)) {
			dir_.v = player_direction::vert::none;
			auto& s_kin = env.get<ecs::kinematics>(s);
			s_kin.flags.bottom = false;
			s_kin.velocity.y = -REPEL_Y_VELOCITY;
			if (env.has<ecs::kinematics>(o)) {
				auto& o_kin = env.get<ecs::kinematics>(o);
				if (o_kin.velocity.x == 0.0f) {
					const glm::vec2 s_center = s_loc.center();
					const glm::vec2 o_center = o_loc.center();
					if (s_center.x < o_center.x) {
						dir_.h = player_direction::hori::right;
						s_kin.velocity.x = -REPEL_X_VELOCITY;
					} else {
						dir_.h = player_direction::hori::left;
						s_kin.velocity.x = REPEL_X_VELOCITY;
					}
				} else {
					s_kin.velocity.x = o_kin.velocity.x;
				}
			} else {
				const glm::vec2 s_center = s_loc.center();
				const glm::vec2 o_center = o_loc.center();
				if (s_center.x < o_center.x) {
					dir_.h = player_direction::hori::right;
					s_kin.velocity.x = -REPEL_X_VELOCITY;
				} else {
					dir_.h = player_direction::hori::left;
					s_kin.velocity.x = REPEL_X_VELOCITY;
				}
			}
			// determine if naomi is dead or hurt
			if (s_hel.maximum <= 0) {
				flags_.killed = true;
				flags_.decimated = true;
				dir_.v = player_direction::vert::up;
				env.smoke(s_loc.center(), SMOKE_COUNT);
				env.shrapnel(s_loc.center(), SHRAPNEL_COUNT);
			} else if (s_hel.current <= 0) {
				if (flags_.broken) {
					flags_.killed = true;
					dir_.v = player_direction::vert::up;
					env.smoke(s_loc.center(), SMOKE_COUNT);
				} else {
					flags_.broken = true;
					timers_.barrier = BARRIER_BROKEN_TIMER;
					audio::play(sfx::BrokenBarrier, 0);
					env.spawn(ai::barrier, s_loc.center());
				}
			} else {
				audio::play(sfx::Damage, 0);
				env.spawn(ai::barrier, s_loc.center());
			}
		}
	}
}

void player::solid(entt::entity o, environment& env, const tile_map& map) {
	static constexpr r32 MUSHY_MARGIN = 3.0f;
	static constexpr r32 STAND_MARGIN = 1.0f;
	static constexpr r32 REPEL_VELOCITY = 1.0f;

	auto s = this->entity();
	auto [s_loc, s_kin] = env.get<ecs::location, ecs::kinematics>(s);
	const rect s_hitbox = s_kin.hitbox.bounds();
	const auto s_left = s_hitbox.x;
	const auto s_top = s_hitbox.y;
	const auto s_right = s_hitbox.right();
	const auto s_bottom = s_hitbox.bottom();

	auto [o_loc, o_kin] = env.get<ecs::location, ecs::kinematics>(o);
	const rect o_hitbox = o_kin.hitbox.bounds();
	const auto o_left = o_hitbox.x;
	const auto o_top = o_hitbox.y;
	const auto o_right = o_hitbox.right();
	const auto o_bottom = o_hitbox.bottom();
	const glm::vec2 o_center = o_hitbox.center();

	// handle sides of object
	if ((s_top < o_bottom - MUSHY_MARGIN) and (s_bottom > o_top + MUSHY_MARGIN)) {
		// left side of object
		if (s_right > o_left and s_right < o_center.x and !s_kin.flags.left) {
			if (s_kin.velocity.x > -REPEL_VELOCITY) {
				s_kin.flags.right = true;
				s_kin.velocity.x -= REPEL_VELOCITY;
			}
		}
		// right side of object
		if (s_left < o_right and s_left > o_center.x and !s_kin.flags.right) {
			if (s_kin.velocity.x < REPEL_VELOCITY) {
				s_kin.flags.left = true;
				s_kin.velocity.x += REPEL_VELOCITY;
			}
		}
	}
	// have we fallen off?
	if (!(s_left > o_right - STAND_MARGIN or s_right < o_left + STAND_MARGIN)) {
		// standing on object
		if (s_bottom >= o_top and s_bottom <= o_center.y and !s_kin.flags.top) {
			s_loc.position.y = o_hitbox.side(side_type::top) - s_hitbox.side(side_type::bottom);
			s_kin.velocity.y = 0.0f;
			s_kin.extend(s_loc, map, o_kin.velocity);
			s_kin.flags.bottom = true;

			if (o_kin.velocity.x != 0.0f and riding_.x == 0.0f) {
				s_loc.position.x -= o_kin.velocity.x;
				s_kin.velocity.x = 0.0f;
			}
			if (o_kin.velocity.y != 0.0f and riding_.y == 0.0f) {
				s_loc.position.y -= o_kin.velocity.y;
				s_kin.velocity.y = 0.0f;
			}
			riding_ = o_kin.velocity;
		} else if (s_top < o_bottom and s_top > o_center.y and !s_kin.flags.bottom) {
			s_loc.position.y = o_hitbox.side(side_type::bottom) - s_hitbox.side(side_type::top);
			s_kin.velocity.y = o_kin.velocity.y > 0.0f ?
				o_kin.velocity.y :
				0.0f;
		}
	}
}

void player::handle(
	const buttons& bts,
	controller& ctl,
	kernel& knl,
	headsup& hud,
	environment& env,
	const tile_map& map
) {
	auto s = this->entity();
	auto [loc, kin, spt, hel, sub] = env.get<
		ecs::location,
		ecs::kinematics,
		ecs::sprite,
		ecs::health,
		ecs::submersible
	>(s);

	this->do_begin_(kin);
	if (flags_.killed) {
		this->do_killed_(kin);
	} else if (!ctl.state().locked) {
		flags_.event_animation = false;
		if (flags_.damaged) {
			this->do_recovery_(kin);
		} else {
			this->do_poison_(hel);
			this->do_invincible_(spt);
			this->do_barrier_(env, loc, hel);
			this->do_strafe_(bts);
			this->do_grapple_(bts, loc, kin);
			this->do_fire_(bts, ctl, env, loc, kin, spt);
			this->do_attack_(bts, env, loc, kin);
			this->do_move_(bts, kin, false);
			this->do_look_(bts, kin);
			this->do_jump_(bts, kin, false);
			this->do_step_(spt);
			this->do_interact_(bts, knl, env, loc, kin);
			this->do_dash_(bts, env, loc, kin, spt);
			this->do_wall_jump_(bts, map, loc, kin);
		}
	} else {
		this->do_move_(bts, kin, true);
		this->do_jump_(bts, kin, true);
		this->do_step_(spt);
	}
	this->do_physics_(kin);
	this->do_water_(env, sub);
	this->do_camera_(kin);
	this->do_death_(knl, kin, hel, sub);
	this->do_animate_(spt, hel);
	this->do_headsup_(hud, hel, sub);
}

void player::visible(environment& env, bool value) {
	auto s = this->entity();
	auto& spt = env.get<ecs::sprite>(s);

	spt.color = value ? 0xFFU : 0x00U;
}

void player::animate(environment& env, udx state, u32 facing) {
	constexpr u32 FACING_LEFT = 0x1;
	constexpr u32 FACING_UP = 0x2;
	constexpr u32 FACING_DOWN = 0x4;
	constexpr u32 FACING_NEUTRAL = 0x8;

	flags_.event_animation = true;

	auto s = this->entity();
	auto& spt = env.get<ecs::sprite>(s);
	spt.state(state);

	// vertical
	if (facing & FACING_DOWN) {
		spt.variation = 2;
	} else if (facing & FACING_UP) {
		spt.variation = 1;
	} else if (facing & FACING_NEUTRAL) {
		spt.variation = 0;
	}

	// horizontal
	if (!(facing & FACING_NEUTRAL)) {
		spt.mirror.horizontally = facing & FACING_LEFT;
	}
}

void player::teleport(environment& env, const glm::vec2& position) {
	auto s = this->entity();

	auto& loc = env.get<ecs::location>(s);
	loc.position = position * konst::TILE<r32>();

	auto& spt = env.get<ecs::sprite>(s);
	spt.prepare(loc.position);
}

void player::add_barrier(environment& env, i32 amount) {
	auto s = this->entity();
	auto& hel = env.get<ecs::health>(s);

	hel.current = glm::clamp(hel.current + amount, 0, hel.maximum);
	flags_.broken = false;
	flags_.healed = true;
	timers_.barrier = 0;
}

void player::boost_barrier(environment& env, i32 amount) {
	auto s = this->entity();
	auto& hel = env.get<ecs::health>(s);

	hel.maximum = glm::clamp(hel.maximum + amount, 0, ABSOLUTE_MAXIMUM_BARRIER);
}

void player::change_poison(environment& env, i32 amount) {
	auto s = this->entity();
	auto& hel = env.get<ecs::health>(s);

	hel.poison = glm::clamp(hel.poison + amount, 0, ABSOLUTE_MAXIMUM_POISON);
}

void player::change_equipment(u32 id, bool value) {
	constexpr u32 MAXIMUM_BITS = as<u32>(sizeof(u32) * 8);

	if (id < MAXIMUM_BITS) {
		equips_._raw.set(id, value);
	} else {
		spdlog::error("Equip ID cannot go higher than {}!", MAXIMUM_BITS - 1);
	}
}

bool player::visible(const environment& env) const {
	auto s = this->entity();
	auto& spt = env.get<ecs::sprite>(s);
	return spt.color.a > 0x00U;
}

glm::vec2 player::viewpoint(const environment& env) const {
	auto s = this->entity();
	return viewpoint_ + env.get<ecs::location>(s).center();
}

u32 player::death_type(const ecs::kinematics& kin, const ecs::health& hel, const ecs::submersible& sub) const {
	if (flags_.decimated) {
		return 1;
	}
	if (flags_.killed) {
		return 2;
	}
	if (kin.flags.out_of_bounds) {
		return 3;
	}
	if (!equips_.oxygen and timers_.oxygen <= 0 and sub.entity != entt::null) {
		return 4;
	}
	if (hel.poison >= ABSOLUTE_MAXIMUM_POISON) {
		return 5;
	}
	return 0;
}

i32 player::indicator_state(const headsup_params& params, udx original) const {
	static constexpr udx INDICATOR_GOOD = 0;
	static constexpr udx INDICATOR_OKAY = 1;
	static constexpr udx INDICATOR_SICK = 2;
	static constexpr udx INDICATOR_HEAL = 3;

	i32 result = INDICATOR_GOOD;
	if (original != INDICATOR_HEAL) {
		if (flags_.healed) {
			result = INDICATOR_HEAL;
		} else if (params.poison >= POISON_LEVEL_TWO) {
			result = INDICATOR_SICK;
		} else if (params.poison >= POISON_LEVEL_ONE) {
			result = INDICATOR_OKAY;
		}
	}
	return result;
}

void player::do_begin_(ecs::kinematics& kin) {
	flags_.interacting = false;
	flags_.healed = false;
	flags_.skidding = false;
	if (kin.flags.bottom) {
		if (flags_.airbourne) {
			flags_.airbourne = false;
			audio::play(sfx::Landing, 0);
		}
	} else {
		flags_.airbourne = true;
	}
}

void player::do_killed_(ecs::kinematics& kin) {
	if (kin.flags.bottom) {
		dir_.v = player_direction::vert::none;
		kin.velocity = {};
		flags_.death_animation = true;
	}
}

void player::do_recovery_(ecs::kinematics& kin) {
	if (kin.flags.bottom) {
		flags_.damaged = false;
		flags_.invincible = true;
		timers_.invincible = INVINCIBILITY_TIMER;
	}
}

void player::do_poison_(ecs::health& hel) {
	if (hel.poison > 0) {
		if (--timers_.poison; timers_.poison <= 0) {
			timers_.poison = POISON_TIMER;
			--hel.poison;
		}
	} else {
		timers_.poison = POISON_TIMER;
	}
}

void player::do_invincible_(ecs::sprite& spt) {
	if (flags_.invincible) {
		if (--timers_.invincible; timers_.invincible <= 0) {
			timers_.invincible = 0;
			flags_.invincible = false;
			spt.color.a = 0xFFU;
		} else {
			spt.color.a = spt.color.a == 0.0f ?
				0x00F :
				0x00U;
		}
	}
}

void player::do_barrier_(environment& env, const ecs::location& loc, ecs::health& hel) {
	if (flags_.broken) {
		if (--timers_.barrier; timers_.barrier <= 0) {
			timers_.barrier = 0;
			flags_.broken = false;
			hel.current = 1;
			env.spawn(ai::barrier, loc.center());
		}
	}
}

void player::do_strafe_(const buttons& bts) {
	if (!bts.holding.apostle and bts.pressed.strafe) {
		flags_.strafing = !flags_.strafing;
		audio::play(sfx::Inven, 7);
	}
}

void player::do_grapple_(const buttons&, const ecs::location&, ecs::kinematics&) {
	if (equips_.grapple) {
		// TODO
	}
}

void player::do_fire_(const buttons& bts, controller& ctl, environment& env, const ecs::location& loc, ecs::kinematics& kin, const ecs::sprite& spt) {
	flags_.firing = false;
	if (timers_.reload > 0) {
		--timers_.reload;
	}
	if (bts.holding.item and timers_.reload == 0 and ctl.provision_weapon()) {
		auto ptr = ctl.provision_ptr();
		if (ptr and ptr->type > 0 and ptr->count > 0) {
			--ptr->count;
			flags_.firing = true;

			const auto point = spt.action_point(
				player_anim::JUMPING_FIRING,
				loc.position
			);
			switch (ptr->type) {
				case player_weapon::KANNON_TYPE: {
					timers_.reload = player_weapon::KANNON_RELOAD;
					audio::play(sfx::Kannon, 4);
					env.spawn(ai::kannon, point, kin.velocity);
					break;
				}
				case player_weapon::FRONTIER_TYPE: {
					timers_.reload = player_weapon::FRONTIER_RELOAD;
					audio::play(sfx::Spark, 4);
					env.spawn(ai::frontier, point);
					if (dir_.v == player_direction::vert::down) {
						kin.accel_y(physics_.hover, physics_.max_hover);
					}
					break;
				}
				case player_weapon::WOLF_VULCAN_TYPE: {
					timers_.reload = player_weapon::WOLF_VULCAN_RELOAD;
					audio::play(sfx::Fan, 4);
					env.spawn(ai::wolf_vulcan, point);
					break;
				}
				case player_weapon::NAIL_RAY_TYPE: {
					timers_.reload = player_weapon::NAIL_RAY_RELOAD;
					audio::play(sfx::Razor, 4);
					env.spawn(ai::frontier, point);
					env.spawn(ai::small_blast, point);
					break;
				}
				case player_weapon::AUSTERE_TYPE: {
					timers_.reload = player_weapon::AUSTERE_RELOAD;
					audio::play(sfx::Fan, 4);
					env.spawn(ai::frontier, point);
					break;
				}
				default:
					break;
			}
		}
	}
}

void player::do_attack_(const buttons& bts, environment& env, const ecs::location& loc, ecs::kinematics& kin) {
	if (equips_.weak_arms or equips_.strong_arms) {
		if (bts.pressed.arms) {
			flags_.will_attack = true;
		}
		if (flags_.attacking) {
			if (--timers_.attack; timers_.attack <= 0) {
				timers_.attack = 0;
				flags_.attacking = false;
			}
		} else if (flags_.will_attack) {
			if (kin.flags.bottom) {
				kin.velocity.x = 0.0f;
			}
			flags_.attacking = true;
			flags_.will_attack = false;
			flags_.firing = false;
			auto& type = equips_.strong_arms ?
				ai::strong_hammer :
				ai::weak_hammer;
			env.spawn(type, loc.center(), kin.velocity);
			audio::play(sfx::Blade, 4);
		}
	}
}

void player::do_move_(const buttons& bts, ecs::kinematics& kin, bool locked) {
	if (
		!flags_.wall_jumping and
		!flags_.will_wall_jump and
		!flags_.charging and
		!flags_.dashing
	) {
		bool right = locked ? false : bts.holding.right.value();
		bool left = locked ? false : bts.holding.left.value();
		if ((right and left) or !(right or left)) {
			flags_.moving = false;
			if (kin.flags.bottom) {
				kin.decel_x(physics_.decel);
			}
		} else {
			flags_.moving = true;
			kin.accel_x(
				right ? physics_.accel : -physics_.accel,
				physics_.max_speed.x
			);
			if (!flags_.strafing) {
				dir_.h = right ?
					player_direction::hori::right :
					player_direction::hori::left;

				if (kin.flags.bottom) {
					if (kin.velocity.x > 0.0f and left) {
						flags_.skidding = true;
					} else if (kin.velocity.x < 0.0f and right) {
						flags_.skidding = true;
					}
				}
			}
		}
	}
}

void player::do_look_(const buttons& bts, const ecs::kinematics& kin) {
	if (!flags_.dashing) {
		bool up = bts.holding.up;
		bool down = bts.holding.down;
		if ((up and down) or !(up or down)) {
			dir_.v = player_direction::vert::none;
		} else if (up) {
			dir_.v = player_direction::vert::up;
		} else if (down) {
			dir_.v = kin.flags.bottom ?
				player_direction::vert::none :
				player_direction::vert::down;
		}
	}
}

void player::do_step_(const ecs::sprite& spt) {
	if (auto state = spt.state();
		state == player_anim::WALKING or
		state == player_anim::WALKING_FIRING
	) {
		if ((spt.frame % 2) != 0) {
			if (!flags_.stepping) {
				flags_.stepping = true;
				audio::play(sfx::Walk, 1);
			}
		} else {
			flags_.stepping = false;
		}
	} else {
		flags_.stepping = false;
	}
}

void player::do_jump_(const buttons& bts, ecs::kinematics& kin, bool locked) {
	if (!kin.flags.bottom) {
		kin.velocity.x += riding_.x;
		riding_ = {};
		if (
			!locked and
			!flags_.will_wall_jump and
			bts.holding.jump and
			kin.velocity.y < 0.0f
		) {
			kin.velocity.y -= physics_.jump_hold;
		}
	} else if (!locked and bts.pressed.jump) {
		if (kin.flags.fall_through and bts.holding.down) {
			kin.flags.fall_through = false;
		} else {
			kin.velocity.y = -physics_.jump_power;
			kin.velocity += riding_;
			riding_ = {};
			audio::play(sfx::Jump, 0);
		}
	}
}

void player::do_interact_(const buttons& bts, kernel& knl, environment& env, const ecs::location& loc, ecs::kinematics& kin) {
	if (kin.flags.bottom and bts.pressed.down) {
		if (
			!flags_.moving and
			!flags_.dashing and
			!flags_.charging and
			!flags_.attacking and
			!flags_.firing
		) {
			flags_.interacting = true;
			kin.velocity.x = 0.0f;
			const rect hitbox = loc.bounds();
			env.slice<ecs::location, ecs::trigger>().each(
			[&knl, &hitbox](entt::entity, const ecs::location& that_loc, const ecs::trigger& trg) {
				if (trg.flags.interaction and that_loc.overlaps(hitbox)) {
					knl.run_event(trg.id);
				}
			});
		}
	}
}

void player::do_dash_(const buttons& bts, environment& env, const ecs::location& loc, ecs::kinematics& kin, ecs::sprite& spt) {
	if (equips_.dash) {
		if (flags_.dashing) {
			if (--timers_.flash; timers_.flash <= 0) {
				timers_.flash = FLASH_TIMER;
				env.spawn(ai::dash_flash, loc.center());
			}
			if (flags_.ceiling_dashing) {
				const auto speed = dir_.h == player_direction::hori::right ?
					physics_.dash_speed :
					-physics_.dash_speed;
				if (kin.flags.right or kin.flags.left) {
					dir_.h = dir_.h == player_direction::hori::right ?
						player_direction::hori::left :
						player_direction::hori::right;
					dir_.v = player_direction::vert::up;
					flags_.wall_dashing = true;
					flags_.ceiling_dashing = false;
					flags_.zero_gravity = true;
					if (dir_.h == player_direction::hori::left) {
						spt.mirror.horizontally = false;
						spt.mirror.vertically = true;
					} else {
						spt.mirror.horizontally = true;
						spt.mirror.vertically = true;
					}
					if (kin.flags.right) {
						spt.variation = 0;
					} else if (kin.flags.left) {
						spt.variation = 1;
					}
					kin.flags.right = false;
					kin.flags.left = false;
					kin.flags.top = false;
					kin.flags.bottom = false;
					kin.velocity = {};
				} else {
					kin.velocity.x = speed;
				}
			} else if (flags_.wall_dashing) {
				const auto speed = dir_.v == player_direction::vert::up ?
					physics_.dash_speed :
					-physics_.dash_speed;
				if (kin.flags.top or kin.flags.bottom) {
					if (kin.flags.top) {
						dir_.h = dir_.h == player_direction::hori::right ?
							player_direction::hori::left :
							player_direction::hori::right;
					}
					dir_.v = player_direction::vert::none;
					flags_.ceiling_dashing = kin.flags.top;
					flags_.wall_dashing = false;
					flags_.zero_gravity = false;
					if (dir_.h == player_direction::hori::left) {
						spt.mirror.horizontally = true;
						spt.mirror.vertically = false;
					} else {
						spt.mirror.horizontally = true;
						spt.mirror.vertically = true;
					}
					spt.variation = 0;
					kin.flags.right = false;
					kin.flags.left = false;
					kin.flags.top = false;
					kin.flags.bottom = false;
					kin.velocity = {};
				} else {
					kin.velocity = { 0.0f, speed };
				}
			} else {
				const auto speed = dir_.h == player_direction::hori::right ?
					physics_.dash_speed :
					-physics_.dash_speed;
				if (kin.flags.right or kin.flags.left) {
					flags_.wall_dashing = true;
					flags_.zero_gravity = true;
					dir_.h = kin.flags.right ?
						player_direction::hori::right :
						player_direction::hori::left;
					dir_.v = player_direction::vert::none;
					if (dir_.h == player_direction::hori::left) {
						spt.mirror.horizontally = true;
						spt.mirror.vertically = false;
						spt.variation = 1;
					} else {
						spt.mirror.horizontally = false;
						spt.mirror.vertically = false;
						spt.variation = 0;
					}
					kin.flags.right = false;
					kin.flags.left = false;
					kin.flags.top = false;
					kin.flags.bottom = false;
					kin.velocity = {};
				} else {
					kin.velocity.x = speed;
				}
			}
			if (bts.pressed.apostle) {
				flags_.dashing = false;
				flags_.wall_dashing = false;
				flags_.ceiling_dashing = false;
				flags_.zero_gravity = false;
				timers_.flash = 0;
				spt.variation = 0;
			}
		} else if (!flags_.charging) {
			if (kin.flags.bottom) {
				if (bts.holding.apostle and bts.pressed.strafe) {
					flags_.charging = true;
					timers_.charging = CHARGING_TIMER;
					kin.velocity.x = 0.0f;
				}
			}
		} else if (timers_.charging > 0) {
			if (--timers_.charging; timers_.charging == 0) {
				timers_.flash = FLASH_TIMER;
				flags_.charging = false;
				flags_.dashing = true;
				kin.flags.right = false;
				kin.flags.left = false;
				kin.flags.top = false;
				kin.flags.bottom = false;
				kin.velocity.x = 0.0f;
			}
		}
	}
}

void player::do_wall_jump_(const buttons& bts, const tile_map& map, const ecs::location& loc, ecs::kinematics& kin) {
	if (equips_.wall_jump) {
		if (!kin.flags.bottom) {
			if (flags_.wall_jumping) {
				kin.velocity.x = dir_.h == player_direction::hori::right ?
					physics_.max_speed.x :
					-physics_.max_speed.x;
				if (--timers_.wall_jump; timers_.wall_jump <= 0) {
					timers_.wall_jump = 0;
					flags_.wall_jumping = false;
				}
			}
			if (flags_.will_wall_jump) {
				if (--timers_.wall_jump; timers_.wall_jump <= 0) {
					flags_.will_wall_jump = false;
					flags_.wall_jumping = true;
					flags_.zero_gravity = false;
					timers_.wall_jump = WALL_JUMP_TIMER;
					kin.velocity.y = -physics_.jump_power;
					audio::play(sfx::Jump, 0);
				}
			} else if (kin.hori_sides()) {
				if (bts.pressed.jump) {
					const auto delta = dir_.h == player_direction::hori::right ?
						physics_.max_speed.x :
						-physics_.max_speed.x;
					const side_type side = dir_.h == player_direction::hori::right ?
						side_type::right :
						side_type::left;
					const rect test = kin.hitbox.side(side, loc.position, delta);
					const auto result = collision::check(map, kin, test, side);
					if (result) {
						kin.velocity.y = 0.0f;
						flags_.will_wall_jump = true;
						flags_.zero_gravity = true;
						timers_.wall_jump = WALL_JUMP_TIMER;
						dir_.h = dir_.h == player_direction::hori::right ?
							player_direction::hori::left :
							player_direction::hori::right;
					}
				}
			}
		}
	}
}

void player::do_camera_(const ecs::kinematics& kin) {
	if (dir_.h == player_direction::hori::left) {
		viewpoint_.x = glm::max(viewpoint_.x - INCR_OFFSET_X, -FAR_OFFSET_X);
	} else {
		viewpoint_.x = glm::min(viewpoint_.x + INCR_OFFSET_X, FAR_OFFSET_X);
	}
	if (!flags_.airbourne) {
		timers_.free_fall = FREE_FALL_TIMER;
		viewpoint_.y = glm::max(viewpoint_.y - INCR_OFFSET_Y1, GROUND_OFFSET_Y);
	} else if (kin.velocity.y >= physics_.max_speed.y) {
		if (--timers_.free_fall; timers_.free_fall <= 0) {
			viewpoint_.y = glm::min(viewpoint_.y + INCR_OFFSET_Y2, AIR_OFFSET_Y);
		}
	}
}

void player::do_physics_(ecs::kinematics& kin) {
	if (!flags_.zero_gravity) {
		kin.accel_y(
			flags_.ceiling_dashing ?
				-physics_.gravity :
				physics_.gravity,
			physics_.max_speed.y
		);
	}
}

void player::do_water_(const environment& env, ecs::submersible& sub) {
	if (sub.entity != entt::null and env.valid(sub.entity)) {
		physics_.water();
		if (!equips_.oxygen) {
			--timers_.oxygen;
		}
	} else {
		physics_.clear();
	}
}

void player::do_animate_(ecs::sprite& spt, const ecs::health& hel) {
	if (!flags_.event_animation) {
		auto state = spt.state();
		if (flags_.death_animation) {
			state = player_anim::KILLED;
		} else if (flags_.damaged) {
			state = player_anim::DAMAGED;
		} else if (flags_.will_wall_jump) {
			state = player_anim::WALL_JUMPING;
		} else if (flags_.charging or flags_.skidding) {
			state = player_anim::CHARGING;
		} else if (flags_.wall_dashing) {
			state = player_anim::DASHING_WALLS;
		} else if (flags_.ceiling_dashing) {
			state = player_anim::DASHING_CEILING;
		} else if (flags_.dashing) {
			state = player_anim::DASHING_FLOOR;
		} else if (flags_.attacking) {
			state = player_anim::ATTACKING;
		} else if (flags_.airbourne) {
			if (flags_.strafing or flags_.firing) {
				state = player_anim::JUMPING_FIRING;
			} else {
				state = player_anim::JUMPING;
			}
		} else if (flags_.moving) {
			if (flags_.strafing or flags_.firing) {
				state = player_anim::WALKING_FIRING;
			} else {
				state = player_anim::WALKING;
			}
		} else if (flags_.interacting or state == player_anim::INTERACTING) {
			dir_.v = player_direction::vert::none;
			state = player_anim::INTERACTING;
		} else if (flags_.strafing or flags_.firing) {
			state = player_anim::IDLE_FIRING;
		} else if (hel.poison >= POISON_LEVEL_ONE) {
			state = player_anim::TIRED;
		} else if (state != player_anim::BLINKING) {
			state = player_anim::IDLE;
		}
		spt.state(state);
		if (dir_ != previous_dir_ and !flags_.dashing) {
			previous_dir_ = dir_;
			spt.mirror.horizontally = dir_.h == player_direction::hori::left;
			switch (dir_.v) {
			case player_direction::vert::down:
				spt.variation = 2;
				break;
			case player_direction::vert::up:
				spt.variation = 1;
				break;
			default:
				spt.variation = 0;
				break;
			}
		}
	}
}

void player::do_death_(kernel& knl, const ecs::kinematics& kin, const ecs::health& hel, const ecs::submersible& sub) {
	if (!knl.running()) {
		if (const auto death = this->death_type(kin, hel, sub); death > 0) {
			knl.run_death(death);
		}
	}
}

void player::do_headsup_(headsup& hud, const ecs::health& hel, const ecs::submersible& sub) {
	headsup_params params {};
	params.strafing = flags_.strafing;
	params.indication = this->indicator_state(params, hud.indication());
	params.current_barrier = hel.current;
	params.maximum_barrier = hel.maximum;
	params.poison = hel.poison;
	params.maximum_oxygen = ABSOLUTE_MAXIMUM_OXYGEN / OXYGEN_TIMER_FACTOR;
	if (!equips_.oxygen and sub.entity != entt::null) {
		params.current_oxygen = timers_.oxygen / OXYGEN_TIMER_FACTOR;
	} else {
		params.current_oxygen = params.maximum_oxygen;
	}
	hud.set(params);
}
