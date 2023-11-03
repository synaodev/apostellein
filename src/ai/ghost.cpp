#include "./ghost.hpp"
#include "../field/environment.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/health.hpp"
#include "../ecs/sprite.hpp"
#include "../util/id-table.hpp"

static void ghost_tick(entt::entity s, kernel&, camera&, player&, environment& env) {

}

static void ghost_ctor(entt::entity s, environment& env) {
	auto& loc = env.get<ecs::location>(s);
	loc.hitbox = { 4.0f, 0.0f, 8.0f, 16.0f };

	auto& spr = env.emplace<ecs::sprite>(s, anim::Ghost);
	spr.pivot = { 8.0f, 16.0f };

	auto& hel = env.emplace<ecs::health>(s);
	hel.current = 8;
	hel.maximum = 8;
	hel.damage = 1;
	hel.poison = 1;
	hel.flags.apostolic = true;

	env.emplace<ecs::thinker>(s, ghost_tick);
}

// Tables

APOSTELLEIN_THINKER_TABLE(ghost) {
	APOSTELLEIN_THINKER_ENTRY(ai::ghost, ghost_ctor);
}
