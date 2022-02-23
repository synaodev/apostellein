#include <apostellein/konst.hpp>

#include "./liquid.hpp"
#include "./aktor.hpp"
#include "../field/environment.hpp"
#include "../hw/audio.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr chroma WATER_TINT { 0x00U, 0x3FU, 0x7FU, 0x7FU };
}

void ecs::liquid::handle(environment& env, const ecs::location& loc, ecs::submersible& sub) {
	auto check_validity = [&loc, &sub](entt::entity e, const ecs::liquid& liq) {
		if (sub.entity == entt::null and loc.overlaps(liq.hitbox)) {
			sub.entity = e;
		}
	};
	auto spawner = [&env](const ecs::location& that_loc, const ecs::submersible& that_sub, const rect& bounds) {
		if (that_sub.sound.value()) {
			audio::play(that_sub.sound, 11);
		}
		if (that_sub.particle.value()) {
			const glm::vec2 position { that_loc.center().x, bounds.y };
			env.spawn(that_sub.particle, position);
		}
	};
	if (sub.entity == entt::null or !env.valid(sub.entity)) {
		sub.entity = entt::null;
		env.slice<ecs::liquid>().each(check_validity);
		if (sub.entity != entt::null) {
			spawner(loc, sub, env.get<ecs::liquid>(sub.entity).hitbox);
		}
	} else if (!loc.overlaps(env.get<ecs::liquid>(sub.entity).hitbox)) {
		sub.entity = entt::null;
		env.slice<ecs::liquid>().each(check_validity);
		if (sub.entity == entt::null) {
			spawner(loc, sub, env.get<ecs::liquid>(sub.entity).hitbox);
		}
	}
}

void ecs::liquid::handle(environment& env) {
	env.slice<ecs::location, ecs::submersible>().each(
	[&env](entt::entity, const ecs::location& loc, ecs::submersible& sub) {
		ecs::liquid::handle(env, loc, sub);
	});
}

void ecs::liquid::render(const rect& view, renderer& rdr, const environment& env) {
	auto slice = env.slice<ecs::liquid>();
	if (!slice.empty()) {
		auto& list = rdr.query(
			priority_type::automatic,
			blending_type::add,
			pipeline_type::blank
		);
		slice.each([&list, &view](entt::entity, const ecs::liquid& liq) {
			if (liq.hitbox.overlaps(view)) {
				list.batch_blank(liq.hitbox, WATER_TINT);
			}
		});
	}
}
