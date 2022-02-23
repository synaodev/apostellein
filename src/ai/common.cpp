#include "./common.hpp"
#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/kinematics.hpp"
#include "../ecs/health.hpp"
#include "../ctrl/kernel.hpp"
#include "../util/id-table.hpp"

namespace {
	constexpr rect HORIZONAL_TRIGGER { 0.0f, -48.0f, 16.0f, 112.0f };
	constexpr rect VERTICAL_TRIGGER { -48.0f, 0.0f, 112.0f, 16.0f };
}

static void null_ctor(entt::entity, environment&) {}

static void hv_trigger_tick(entt::entity s, kernel& knl, camera&, player& plr, environment& env) {
	auto& trg = env.get<ecs::trigger>(s);
	if (trg.flags.interaction) {
		auto& s_loc = env.get<ecs::location>(s);
		auto& p_loc = env.get<ecs::location>(plr.entity());
		if (s_loc.overlaps(p_loc)) {
			knl.run_event(trg.id);
			env.dispose(s);
		}
	}
}

static void hv_trigger_ctor(entt::entity s, environment& env) {
	auto [loc, trg] = env.get<ecs::location, ecs::trigger>(s);
	if (trg.flags.facing_left) {
		loc.hitbox = VERTICAL_TRIGGER;
	} else {
		loc.hitbox = HORIZONAL_TRIGGER;
	}
	env.emplace<ecs::thinker>(s, hv_trigger_tick);
}

static void full_chest_ctor(entt::entity s, environment& env) {
	env.emplace<ecs::sprite>(s, anim::Chest);
}

static void empty_chest_ctor(entt::entity s, environment& env) {
	auto& spt = env.emplace<ecs::sprite>(s, anim::Chest);
	spt.variation = 1;
}

static void door_ctor(entt::entity s, environment& env) {
	env.emplace<ecs::sprite>(s, anim::Door);
}

static void spikes_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 4.0f, 4.0f, 24.0f, 12.0f };

	env.emplace<ecs::sprite>(s, anim::Death);

	auto& hel = env.emplace<ecs::health>(s);
	hel.flags.deadly = true;
}

static void bed_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 8.0f, 0.0f, 16.0f, 16.0f };

	env.emplace<ecs::sprite>(s, anim::Helpful);
}

static void ammo_station_ctor(entt::entity s, environment& env) {
	auto& spt = env.emplace<ecs::sprite>(s, anim::Helpful);
	spt.state(1);
}

static void computer_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.position.x -= 2.0f;
	loc.hitbox = { 5.0f, 0.0f, 16.0f, 16.0f };

	env.emplace<ecs::sprite>(s, anim::Computer);
}

static void fire_ctor(entt::entity s, environment& env) {
	env.emplace<ecs::sprite>(s, anim::Fireplace);
}

APOSTELLEIN_THINKER_TABLE(common) {
	APOSTELLEIN_THINKER_ENTRY(ai::null, null_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::hv_trigger, hv_trigger_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::full_chest, full_chest_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::empty_chest, empty_chest_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::door, door_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::spikes, spikes_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::bed, bed_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::ammo_station, ammo_station_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::computer, computer_ctor);
	APOSTELLEIN_THINKER_ENTRY(ai::fire, fire_ctor);
}
