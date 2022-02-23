#include <apostellein/konst.hpp>

#include "./aktor.hpp"
#include "../field/environment.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr chroma HITBOX_TINT { 0xFFU, 0xFFU, 0xFFU, 0x7FU };
}

void ecs::location::render(const rect& view, renderer& rdr, const environment& env) {
	auto& list = rdr.query(
		priority_type::automatic,
		blending_type::add,
		pipeline_type::blank
	);
	env.slice<ecs::location>().each(
	[&list, &view](entt::entity, const ecs::location& loc) {
		const rect bounds = loc.bounds();
		if (bounds.overlaps(view)) {
			list.batch_blank(bounds, HITBOX_TINT);
		}
	});
}
