#include "./thinker.hpp"
#include "../field/environment.hpp"

void ecs::thinker::handle(kernel& knl, camera& cam, player& plr, environment& env) {
	env.slice<ecs::thinker>().each(
	[&knl, &cam, &plr, &env](entt::entity s, const ecs::thinker& thk) {
		if (thk.func) {
			thk.func(s, knl, cam, plr, env);
		}
	});
}
