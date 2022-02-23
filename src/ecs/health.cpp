#include "./health.hpp"
#include "./aktor.hpp"
#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../ctrl/kernel.hpp"
#include "../menu/headsup.hpp"

bool ecs::health::attack(
	const ecs::location& this_location,
	ecs::location& that_location,
	const ecs::health& this_health,
	ecs::health& that_health
) {
	if (this_health.flags.attacking) {
		if (this_location.overlaps(that_location)) {
			if (this_health.flags.deadly) {
				that_health.maximum = 0;
			} else if (!that_health.flags.once_more and that_health.maximum < this_health.damage) {
				that_health.maximum = 0;
			}
			that_health.current -= this_health.damage;
			that_health.poison += this_health.poison;
			return true;
		}
	}
	return false;
}

void ecs::health::handle(kernel& knl, headsup& hud, player& plr, environment& env) {
	env.slice<ecs::aktor, ecs::health>().each(
	[&knl, &hud, &plr, &env](entt::entity e, const ecs::aktor&, ecs::health& hel) {
		if (hel.current <= 0) {
			if (env.has<ecs::trigger>(e)) {
				auto& trg = env.get<ecs::trigger>(e);
				if (trg.flags.destruction) {
					trg.flags.destruction = false;
					knl.run_event(trg.id);
				}
			}
			if (hel.flags.boss_fight) {
				hel.clear();
				hud.meter(0, 0);
			} else {
				env.dispose(e);
			}
		} else if ((hel.flags.attacking and hel.damage > 0) or hel.flags.deadly) {
			plr.damage(e, env);
		} else if (hel.flags.boss_fight) {
			hud.meter(hel.current, hel.maximum);
		}
	});
}
