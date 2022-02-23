#pragma once

#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>
#include <apostellein/struct.hpp>

struct buttons;
struct tile_map;
struct headsup;
struct headsup_params;
struct environment;
struct camera;
struct kernel;
struct controller;

namespace ecs {
	struct location;
	struct kinematics;
	struct anchor;
	struct health;
	struct liquid;
	struct submersible;
	struct sprite;
}

union player_flags {
	bitfield_raw<u32> _raw {};
	bitfield_index<u32, 0> moving;
	bitfield_index<u32, 1> stepping;
	bitfield_index<u32, 2> firing;
	bitfield_index<u32, 3> attacking;
	bitfield_index<u32, 4> strafing;
	bitfield_index<u32, 5> interacting;
	bitfield_index<u32, 6> charging;
	bitfield_index<u32, 7> dashing;
	bitfield_index<u32, 8> wall_jumping;
	bitfield_index<u32, 9> wall_dashing;
	bitfield_index<u32, 10> ceiling_dashing;
	bitfield_index<u32, 11> airbourne;
	bitfield_index<u32, 12> zero_gravity;
	bitfield_index<u32, 13> will_attack;
	bitfield_index<u32, 14> will_wall_jump;
	bitfield_index<u32, 15> invincible;
	bitfield_index<u32, 16> healed;
	bitfield_index<u32, 17> damaged;
	bitfield_index<u32, 18> broken;
	bitfield_index<u32, 19> killed;
	bitfield_index<u32, 20> decimated;
	bitfield_index<u32, 21> event_animation;
	bitfield_index<u32, 22> death_animation;
};

union player_equips {
	bitfield_raw<u32> _raw {};
	bitfield_index<u32, 0> weak_arms;
	bitfield_index<u32, 1> wall_jump;
	bitfield_index<u32, 2> grapple;
	bitfield_index<u32, 3> dash;
	bitfield_index<u32, 4> oxygen;
	bitfield_index<u32, 5> once_more;
	bitfield_index<u32, 6> strong_arms;
	bitfield_index<u32, 7> carry_shoshi;
	bitfield_index<u32, 8> accompany_shoshi;
};

struct player_timers {
	i32 flash {};
	i32 attack {};
	i32 reload {};
	i32 poison {};
	i32 oxygen {};
	i32 invincible {};
	i32 barrier {};
	i32 wall_jump {};
	i32 charging {};
	i32 free_fall {};
public:
	void clear() {
		flash = 0;
		attack = 0;
		reload = 0;
		poison = 0;
		oxygen = 0;
		invincible = 0;
		barrier = 0;
		wall_jump = 0;
		charging = 0;
		free_fall = 0;
	}
};

struct player_physics {
	glm::vec2 max_speed {};
	r32 accel {};
	r32 decel {};
	r32 jump_power {};
	r32 jump_hold {};
	r32 hover {};
	r32 max_hover {};
	r32 gravity {};
	r32 dash_speed {};
public:
	void clear() {
		max_speed = { 2.0f, 6.0f };
		accel = 0.1f;
		decel = 0.24f;
		jump_power = 3.7f;
		jump_hold = 0.16f;
		hover = 0.01f;
		max_hover = 0.3f;
		gravity = 0.28f;
		dash_speed = 5.0f;
	}
	void water() {
		max_speed = { 1.0f, 3.0f };
		accel = 0.05f;
		decel = 0.12f;
		jump_power = 1.85f;
		jump_hold = 0.08f;
		hover = 0.01f;
		max_hover = 0.3f;
		gravity = 0.14f;
		dash_speed = 2.5f;
	}
};

struct player_direction {
	enum class hori {
		none,
		right,
		left
	} h { hori::right };
	enum class vert {
		none,
		up,
		down
	} v { vert::none };
public:
	bool operator==(const player_direction& that) const {
		return (
			this->h == that.h and
			this->v == that.v
		);
	}
	bool operator!=(const player_direction& that) const {
		return !(*this == that);
	}
};

struct player {
public:
	bool build(environment& env);
	void clear(environment& env);
	void read(const nlohmann::json& data, environment& env);
	void write(nlohmann::json& data, const environment& env) const;
	void transfer(i32 id, camera& cam, environment& env);
	void damage(entt::entity o, environment& env);
	void solid(entt::entity o, environment& env, const tile_map& map);
	void handle(
		const buttons& bts,
		controller& ctl,
		kernel& knl,
		headsup& hud,
		environment& env,
		const tile_map& map
	);
	void visible(environment& env, bool value);
	void animate(environment& env, udx state, u32 facing);
	void teleport(environment& env, const glm::vec2& position);
	void add_barrier(environment& env, i32 amount);
	void boost_barrier(environment& env, i32 amount);
	void change_poison(environment& env, i32 amount);
	void change_equipment(u32 id, bool value);
	entt::entity entity() const { return entt::entity{ 0 }; }
	bool visible(const environment& env) const;
	bool interacting() const { return flags_.interacting; }
	glm::vec2 viewpoint(const environment& env) const;
	u32 death_type(
		const ecs::kinematics& kin,
		const ecs::health& hel,
		const ecs::submersible& sub
	) const;
	i32 indicator_state(const headsup_params& params, udx original) const;
private:
	void do_begin_(ecs::kinematics& kin);
	void do_killed_(ecs::kinematics& kin);
	void do_recovery_(ecs::kinematics& kin);
	void do_poison_(ecs::health& hel);
	void do_invincible_(ecs::sprite& spt);
	void do_barrier_(environment& env, const ecs::location& loc, ecs::health& hel);
	void do_strafe_(const buttons& bts);
	void do_grapple_(const buttons& bts, const ecs::location& loc, ecs::kinematics& kin);
	void do_fire_(const buttons& bts, controller& ctl, environment& env, const ecs::location& loc, ecs::kinematics& kin, const ecs::sprite& spt);
	void do_attack_(const buttons& bts, environment& env, const ecs::location& loc, ecs::kinematics& kin);
	void do_move_(const buttons& bts, ecs::kinematics& kin, bool locked);
	void do_look_(const buttons& bts, const ecs::kinematics& kin);
	void do_step_(const ecs::sprite& spt);
	void do_jump_(const buttons& bts, ecs::kinematics& kin, bool locked);
	void do_interact_(const buttons& bts, kernel& knl, environment& env, const ecs::location& loc, ecs::kinematics& kin);
	void do_dash_(const buttons& bts, environment& env, const ecs::location& loc, ecs::kinematics& kin, ecs::sprite& spt);
	void do_wall_jump_(const buttons& bts, const tile_map& map, const ecs::location& loc, ecs::kinematics& kin);
	void do_camera_(const ecs::kinematics& kin);
	void do_physics_(ecs::kinematics& kin);
	void do_water_(const environment& env, ecs::submersible& sub);
	void do_animate_(ecs::sprite& spt, const ecs::health& hel);
	void do_death_(kernel& knl, const ecs::kinematics& kin, const ecs::health& hel, const ecs::submersible& sub);
	void do_headsup_(headsup& hud, const ecs::health& hel, const ecs::submersible& sub);
	player_flags flags_ {};
	player_equips equips_ {};
	player_timers timers_ {};
	player_physics physics_ {};
	player_direction dir_ {};
	player_direction previous_dir_ {};
	glm::vec2 riding_ {};
	glm::vec2 viewpoint_ {};
};
